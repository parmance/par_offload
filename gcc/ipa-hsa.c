/* Callgraph based analysis of static variables.
   Copyright (C) 2015-2018 Free Software Foundation, Inc.
   Contributed by Martin Liska <mliska@suse.cz>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* Interprocedural HSA pass is responsible for creation of HSA clones.
   For all these HSA clones, we emit HSAIL instructions and pass processing
   is terminated.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "is-a.h"
#include "hash-set.h"
#include "vec.h"
#include "tree.h"
#include "tree-pass.h"
#include "function.h"
#include "basic-block.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "dumpfile.h"
#include "gimple-pretty-print.h"
#include "tree-streamer.h"
#include "stringpool.h"
#include "cgraph.h"
#include "print-tree.h"
#include "symbol-summary.h"
#include "hsa-common.h"
#include "cfg.h"
#include "attribs.h"
#include "tree-cfg.h"

namespace {

/* If NODE is not versionable, warn about not emiting HSAIL and return false.
   Otherwise return true.  */

static bool
check_warn_node_versionable (cgraph_node *node)
{
  if (!node->local.versionable)
    {
      warning_at (EXPR_LOCATION (node->decl), OPT_Whsa,
		  "could not emit HSAIL for function %s: function cannot be "
		  "cloned", node->name ());
      return false;
    }
  return true;
}

/* Search the callgraph for any function that might be a candidate for
   offloading via parallel STL calls and marks them with the "hsa_universal"
   attribute.  */

static void
mark_possibly_pstl_offloaded_functions ()
{
  vec<cgraph_node *> nodes_to_remove;
  struct cgraph_node *node;
  FOR_EACH_DEFINED_FUNCTION(node)
  {
    hsa_function_summary *s = hsa_summaries->get (node);
    tree decl = node->decl;

    if (dump_file)
      fprintf (dump_file, "CONSIDERING function '%s' for HSA offloading.\n",
	       get_name (decl));

    bool is_kernel = lookup_attribute ("hsa_kernel", DECL_ATTRIBUTES (decl));

    /* A linked function is skipped.  */
    if (s->m_bound_function != NULL)
      continue;

    /* Skip known functions which are known to cause problems when compiling
       to HSAIL. */

    /* In case a hsa_kernel is unsupported (e.g. due to return
       slot optimized calls inside it), ensure it won't get called
       or compiled to host ISA by changing it to hsa_function.  */
#define UNSUPPORTED_HSA_F(__PRED, __REASON)				\
    {									\
      if (__PRED)							\
	{								\
	  if (dump_enabled_p())						\
	    dump_printf_loc						\
	      (MSG_MISSED_OPTIMIZATION, DECL_SOURCE_LOCATION (decl),	\
	       "Unable to prepare '%s' for HSA offloading (" __REASON ").\n", \
	       get_name (decl));					\
	  DECL_ATTRIBUTES (decl)					\
	    = remove_attribute ("omp declare target",			\
				DECL_ATTRIBUTES (decl));		\
	  if (lookup_attribute ("hsa_kernel", DECL_ATTRIBUTES(decl)))	\
	    nodes_to_remove.safe_push (node);				\
	  continue;							\
	}								\
    } do {} while (0)

    UNSUPPORTED_HSA_F (DECL_VIRTUAL_P (decl), "virtual function");
    UNSUPPORTED_HSA_F (MAIN_NAME_P (DECL_NAME (decl)), "main function");
    UNSUPPORTED_HSA_F (node->only_called_directly_or_aliased_p (),
		       "indirect function");
    UNSUPPORTED_HSA_F (node->indirect_calls, "has indirect calls");
    UNSUPPORTED_HSA_F (DECL_STATIC_CONSTRUCTOR (decl) ||
		       DECL_STATIC_DESTRUCTOR (decl),
		       "static object constructor or destructor");

    bool calls_throwing_functions = false;
    bool calls_functions_with_indirect_calls = false;
    for (cgraph_edge *e = node->callees; e; e = e->next_callee)
      {

	/* Treat kernels as a special case here because they might
	   call the empty placeholder declarations which _might_
	   throw in the point of view of cgraph analysis.  */

	if (!is_kernel && e->can_throw_external)
	  {
	    calls_throwing_functions = !is_kernel;
	    break;
	  }
	if (e->callee->indirect_calls)
	  {
	    calls_functions_with_indirect_calls = true;
	    break;
	  }
      }

    UNSUPPORTED_HSA_F (calls_throwing_functions,
		       "calls to functions that throw exceptions");
    UNSUPPORTED_HSA_F (calls_functions_with_indirect_calls,
		       "calls to functions with indirect calls");

    /* Some of the internal identifiers from the C++ frontend must be skipped.
       We can detect these from names that have a trailing space.  */
    const char *name = get_name (decl);
    UNSUPPORTED_HSA_F (name[strlen (name) - 1] == ' ',
		       "C++ frontend internal function");

    /* Scan the instructions and look for expressions not supported by
       HSAIL or hsa-gen.c.  */

    bool takes_address_of_function = false,
      takes_address_of_unsupported_global_var = false,
      calls_with_return_slot_opt = false,
      known_bad = false;

    function *func = node->get_fun ();
    basic_block bb;
    FOR_EACH_BB_FN (bb, func)
      {
	gimple_stmt_iterator gsi;

	for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi) && !known_bad;
	     gsi_next (&gsi))
	  {
	    gimple *stmt = gsi_stmt (gsi);

	    if (is_gimple_debug (stmt) || !gimple_has_ops (stmt))
	      continue;

	    const bool is_call = is_gimple_call (stmt);

	    if (is_call && gimple_call_return_slot_opt_p ((gcall*)(stmt)))
	      {
		calls_with_return_slot_opt = true;
		known_bad = true;
		break;
	      }

	    /* In case of calls, we need to check the call arguments.
	       The function target operand is already checked for
	       indirectness earlier.  */
	    const unsigned num_ops
	      = is_call
	      ? gimple_call_num_args ((const gcall*)(stmt))
	      : gimple_num_ops (stmt);

	    for (unsigned opr = 0;
		 gimple_has_ops (stmt) && opr < num_ops && !known_bad; ++opr)
	      {
		tree operand
		  = !is_call ? gimple_op (stmt, opr)
		  : gimple_call_arg ((const gcall*)(stmt), opr);

		if (operand == NULL_TREE)
		  break;

		/* Taking the address of a function is not supported.   */
		/* For now forbid also all kinds of global variable
		   references as the mechanism to mark host-defined global
		   variables that were not in modules built with -fpar_offload
		   is not supported.   */
		if (TREE_CODE (operand) == ADDR_EXPR)
		  {
		    if (TREE_CODE (TREE_OPERAND (operand, 0)) == FUNCTION_DECL)
		      {
			known_bad = true;
			takes_address_of_function = true;
			break;
		      }
		    else if (TREE_CODE (TREE_OPERAND (operand, 0)) == VAR_DECL
			     && is_global_var (TREE_OPERAND (operand, 0)))
		      {
			known_bad = true;
			takes_address_of_unsupported_global_var = true;
			/*print_gimple_stmt (stdout, stmt, TDF_VOPS|TDF_MEMSYMS|TDF_ADDRESS);*/
			break;
		      }
		  }
	      }
	  }
      }

    UNSUPPORTED_HSA_F (takes_address_of_function,
		       "takes a function's address");

    UNSUPPORTED_HSA_F (takes_address_of_unsupported_global_var,
		       "refers to an unsupported global variable");

    UNSUPPORTED_HSA_F (calls_with_return_slot_opt,
		       "calls with return slot optimization");

    if (!lookup_attribute ("hsa_universal", DECL_ATTRIBUTES (decl)))
	DECL_ATTRIBUTES (decl) = tree_cons (get_identifier ("hsa_universal"),
					    NULL_TREE, DECL_ATTRIBUTES (decl));

    if (dump_file)
      fprintf (dump_file, "MARKED function '%s' for HSA offloading.\n",
	       get_name (decl));
    /*dump_function_to_file (decl, stderr, TDF_VOPS|TDF_MEMSYMS|TDF_ADDRESS); */
  }

  for (size_t i = 0; i < nodes_to_remove.length (); ++i)
    nodes_to_remove[i]->remove();
}

/* The function creates HSA clones for all functions that were either
   marked as HSA kernels or are callable HSA functions.  Apart from that,
   we redirect all edges that come from an HSA clone and end in another
   HSA clone to connect these two functions.  */

static unsigned int
process_hsa_functions (void)
{
  struct cgraph_node *node;

  if (hsa_summaries == NULL)
    hsa_summaries = new hsa_summary_t (symtab);

  if (flag_par_offload)
    mark_possibly_pstl_offloaded_functions ();

  FOR_EACH_FUNCTION (node)
    {
      hsa_function_summary *s = hsa_summaries->get (node);

      /* A linked function is skipped.  */
      if (s->m_bound_function != NULL)
	continue;

      if (lookup_attribute ("hsa_placeholder", DECL_ATTRIBUTES (node->decl)))
	{
	  /* The placeholder functions are treated as artificial functions,
	     but they need to end up in the BRIG to enable the specialization.
	     Ensure this is the case by forcing them public.  */
	  TREE_PUBLIC (node->decl) = true;
	}
      else if (lookup_attribute ("hsa_kernel", DECL_ATTRIBUTES (node->decl)))
	hsa_summaries->mark_hsa_only_implementation (node, HSA_KERNEL);
      else if (lookup_attribute ("hsa_function", DECL_ATTRIBUTES (node->decl))
	       || s->m_kind != HSA_NONE)
	{
	  if (lookup_attribute ("hsa_function", DECL_ATTRIBUTES (node->decl)))
	    hsa_summaries->mark_hsa_only_implementation (node, HSA_FUNCTION);
	  if (!check_warn_node_versionable (node))
	    continue;
	  cgraph_node *clone
	    = node->create_virtual_clone (vec <cgraph_edge *> (),
					  NULL, NULL, "hsa");
	  TREE_PUBLIC (clone->decl) = TREE_PUBLIC (node->decl);
	  clone->externally_visible = node->externally_visible;

	  clone->force_output = true;
	  hsa_summaries->link_functions (clone, node, s->m_kind, false);

	  if (dump_file)
	    fprintf (dump_file, "Created a new HSA clone: %s, type: %s " \
		     "gridified %d\n", clone->name (),
		     s->m_kind == HSA_KERNEL ? "kernel" : "function",
		     s->m_gridified_kernel_p);
	}
      else if (lookup_attribute ("hsa_universal",
				 DECL_ATTRIBUTES (node->decl)))
	{
	  /* hsa_universal functions are compiled both to host ISA and HSAIL.
	     Thus, they should not contain HSA-specific builtin calls.  They
	     also get indexed, meaning one can query a device's function handle
	     via the GOMP_host_to_device_fptr() API.  */
	  if (!lookup_attribute ("omp declare target",
				 DECL_ATTRIBUTES (node->decl)))
	    DECL_ATTRIBUTES (node->decl)
	      = tree_cons (get_identifier ("omp declare target"),
			   NULL_TREE, DECL_ATTRIBUTES (node->decl));

	  if (!check_warn_node_versionable (node))
            continue;
          cgraph_node *clone
            = node->create_virtual_clone (vec <cgraph_edge *> (),
                                          NULL, NULL, "hsa");

	  TREE_PUBLIC (clone->decl) = TREE_PUBLIC (node->decl);

	  if (flag_par_offload)
	    /* The potential function pointer targets must be 'prog' scope
	       to match the placeholder decl and be visible to the HSA
	       runtime to query their symbol address.  */
	    TREE_PUBLIC (clone->decl) = true;
	  else
	    TREE_PUBLIC (clone->decl) = TREE_PUBLIC (node->decl);

          clone->externally_visible = node->externally_visible;

          if (!cgraph_local_p (node))
            clone->force_output = true;
          hsa_summaries->link_functions (clone, node, HSA_FUNCTION, false);

          if (dump_file)
            fprintf (dump_file,
		     "Created a new hsa_universal function clone: %s\n",
                     clone->name ());

	}
      else if (hsa_callable_function_p (node->decl)
	       /* At this point, this is enough to identify clones for
		  parallel, which for HSA would need to be kernels anyway.  */
	       && !DECL_ARTIFICIAL (node->decl))
	{
	  if (!check_warn_node_versionable (node))
	    continue;
	  cgraph_node *clone
	    = node->create_virtual_clone (vec <cgraph_edge *> (),
					  NULL, NULL, "hsa");

	  TREE_PUBLIC (clone->decl) = TREE_PUBLIC (node->decl);

	  clone->externally_visible = node->externally_visible;

	  if (!cgraph_local_p (node))
	    clone->force_output = true;

	  hsa_summaries->link_functions (clone, node, HSA_KERNEL, false);

	  if (dump_file)
	    fprintf (dump_file, "Created a new HSA function clone: %s\n",
		     clone->name ());
	}
    }

  /* Redirect all edges that are between HSA clones.  */
  FOR_EACH_DEFINED_FUNCTION (node)
    {
      cgraph_edge *e = node->callees;

      while (e)
	{
	  hsa_function_summary *src = hsa_summaries->get (node);
	  if (src->m_kind != HSA_NONE && src->m_hsa_implementation_p)
	    {
	      hsa_function_summary *dst = hsa_summaries->get (e->callee);
	      if (dst->m_kind != HSA_NONE && !dst->m_hsa_implementation_p)
		{
		  e->redirect_callee (dst->m_bound_function);
		  if (dump_file)
		    fprintf (dump_file,
			     "Redirecting edge to HSA function: %s->%s\n",
			     xstrdup_for_dump (e->caller->name ()),
			     xstrdup_for_dump (e->callee->name ()));
		}
	    }

	  e = e->next_callee;
	}
    }

  return 0;
}

/* Iterate all HSA functions and stream out HSA function summary.  */

static void
ipa_hsa_write_summary (void)
{
  struct bitpack_d bp;
  struct cgraph_node *node;
  struct output_block *ob;
  unsigned int count = 0;
  lto_symtab_encoder_iterator lsei;
  lto_symtab_encoder_t encoder;

  if (!hsa_summaries)
    return;

  ob = create_output_block (LTO_section_ipa_hsa);
  encoder = ob->decl_state->symtab_node_encoder;
  ob->symbol = NULL;
  for (lsei = lsei_start_function_in_partition (encoder); !lsei_end_p (lsei);
       lsei_next_function_in_partition (&lsei))
    {
      node = lsei_cgraph_node (lsei);
      hsa_function_summary *s = hsa_summaries->get (node);

      if (s->m_kind != HSA_NONE)
	count++;
    }

  streamer_write_uhwi (ob, count);

  /* Process all of the functions.  */
  for (lsei = lsei_start_function_in_partition (encoder); !lsei_end_p (lsei);
       lsei_next_function_in_partition (&lsei))
    {
      node = lsei_cgraph_node (lsei);
      hsa_function_summary *s = hsa_summaries->get (node);

      if (s->m_kind != HSA_NONE)
	{
	  encoder = ob->decl_state->symtab_node_encoder;
	  int node_ref = lto_symtab_encoder_encode (encoder, node);
	  streamer_write_uhwi (ob, node_ref);

	  bp = bitpack_create (ob->main_stream);
	  bp_pack_value (&bp, s->m_kind, 2);
	  bp_pack_value (&bp, s->m_hsa_implementation_p, 1);
	  bp_pack_value (&bp, s->m_bound_function != NULL, 1);
	  streamer_write_bitpack (&bp);
	  if (s->m_bound_function)
	    stream_write_tree (ob, s->m_bound_function->decl, true);
	}
    }

  streamer_write_char_stream (ob->main_stream, 0);
  produce_asm (ob, NULL);
  destroy_output_block (ob);
}

/* Read section in file FILE_DATA of length LEN with data DATA.  */

static void
ipa_hsa_read_section (struct lto_file_decl_data *file_data, const char *data,
		       size_t len)
{
  const struct lto_function_header *header
    = (const struct lto_function_header *) data;
  const int cfg_offset = sizeof (struct lto_function_header);
  const int main_offset = cfg_offset + header->cfg_size;
  const int string_offset = main_offset + header->main_size;
  struct data_in *data_in;
  unsigned int i;
  unsigned int count;

  lto_input_block ib_main ((const char *) data + main_offset,
			   header->main_size, file_data->mode_table);

  data_in
    = lto_data_in_create (file_data, (const char *) data + string_offset,
			  header->string_size, vNULL);
  count = streamer_read_uhwi (&ib_main);

  for (i = 0; i < count; i++)
    {
      unsigned int index;
      struct cgraph_node *node;
      lto_symtab_encoder_t encoder;

      index = streamer_read_uhwi (&ib_main);
      encoder = file_data->symtab_node_encoder;
      node = dyn_cast<cgraph_node *> (lto_symtab_encoder_deref (encoder,
								index));
      gcc_assert (node->definition);
      hsa_function_summary *s = hsa_summaries->get (node);

      struct bitpack_d bp = streamer_read_bitpack (&ib_main);
      s->m_kind = (hsa_function_kind) bp_unpack_value (&bp, 2);
      s->m_hsa_implementation_p = bp_unpack_value (&bp, 1);
      bool has_tree = bp_unpack_value (&bp, 1);

      if (has_tree)
	{
	  tree decl = stream_read_tree (&ib_main, data_in);
	  s->m_bound_function = cgraph_node::get_create (decl);
	}
    }
  lto_free_section_data (file_data, LTO_section_ipa_hsa, NULL, data,
			 len);
  lto_data_in_delete (data_in);
}

/* Load streamed HSA functions summary and assign the summary to a function.  */

static void
ipa_hsa_read_summary (void)
{
  struct lto_file_decl_data **file_data_vec = lto_get_file_decl_data ();
  struct lto_file_decl_data *file_data;
  unsigned int j = 0;

  if (hsa_summaries == NULL)
    hsa_summaries = new hsa_summary_t (symtab);

  while ((file_data = file_data_vec[j++]))
    {
      size_t len;
      const char *data = lto_get_section_data (file_data, LTO_section_ipa_hsa,
					       NULL, &len);

      if (data)
	ipa_hsa_read_section (file_data, data, len);
    }
}

const pass_data pass_data_ipa_hsa =
{
  IPA_PASS, /* type */
  "hsa", /* name */
  OPTGROUP_OMP, /* optinfo_flags */
  TV_IPA_HSA, /* tv_id */
  0, /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  TODO_dump_symtab, /* todo_flags_finish */
};

class pass_ipa_hsa : public ipa_opt_pass_d
{
public:
  pass_ipa_hsa (gcc::context *ctxt)
    : ipa_opt_pass_d (pass_data_ipa_hsa, ctxt,
		      NULL, /* generate_summary */
		      ipa_hsa_write_summary, /* write_summary */
		      ipa_hsa_read_summary, /* read_summary */
		      ipa_hsa_write_summary, /* write_optimization_summary */
		      ipa_hsa_read_summary, /* read_optimization_summary */
		      NULL, /* stmt_fixup */
		      0, /* function_transform_todo_flags_start */
		      NULL, /* function_transform */
		      NULL) /* variable_transform */
    {}

  /* opt_pass methods: */
  virtual bool gate (function *);

  virtual unsigned int execute (function *) { return process_hsa_functions (); }

}; // class pass_ipa_reference

bool
pass_ipa_hsa::gate (function *)
{
  return hsa_gen_requested_p ();
}

} // anon namespace

ipa_opt_pass_d *
make_pass_ipa_hsa (gcc::context *ctxt)
{
  return new pass_ipa_hsa (ctxt);
}
