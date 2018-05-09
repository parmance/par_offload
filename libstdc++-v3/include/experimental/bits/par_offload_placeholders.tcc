// Offloading ParallelSTL HSA function placeholders -*- C++ -*-

// Copyright (C) 2017-2018 Free Software Foundation, Inc.
// Contributed by Pekka Jaaskelainen <pekka.jaaskelainen@parmance.com>
// for General Processor Tech.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files
// (the "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

/** @file experimental/bits/par_offload_placeholders.tcc
 *  This is an internal header file included by the various par_offload
 *  implementations for the placeholder functions used by plugin-hsa
 *  to specialize the kernel launches to call targeted functors directly.
 *  The placeholders are collected to this header because it's an ugliness
 *  that hurts eyes and distracts while reading the meaningful implementation
 *  parts.
 */
#ifndef _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_PLACEHOLDERS_TCC
#define _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_PLACEHOLDERS_TCC 1

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Whsa"

// The placeholder declarations are used to refer to the specialized
// function before the functor is known from the kernel. Their name
// is ugly and magic because the specialization to the concrete called
// function is done by string replacing and we must ensure there's enough
// space.

// par_offload_foreach.tcc placeholder functions:

#define _PSTL_HSA_FOR_EACH_FUNCTOR_PLACEHOLDER				\
  __hsa_callee_placeholder_for_each_____________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________


template<typename _RetType, typename _ArgType>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_FOR_EACH_FUNCTOR_PLACEHOLDER (_ArgType);

#define _PSTL_HSA_FOR_EACH_FUNCTOR_PLACEHOLDER_M			\
  __hsa_callee_placeholder_m_for_each___________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________


template<typename _RetType, typename _ArgType>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_FOR_EACH_FUNCTOR_PLACEHOLDER_M (void *, _ArgType);

// par_offload_max_element.tcc placeholder functions:

#define _PSTL_HSA_MAX_ELEMENT_CMP_PLACEHOLDER				\
  __hsa_callee_placeholder_max_element_cmp______________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________

template<typename _RetType, typename _ArgType>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_MAX_ELEMENT_CMP_PLACEHOLDER (_ArgType, _ArgType);

#define _PSTL_HSA_MAX_ELEMENT_CMP_PLACEHOLDER_M				\
  __hsa_callee_placeholder_max_element_cmp_m____________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________


template<typename _RetType, typename _ArgType>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_MAX_ELEMENT_CMP_PLACEHOLDER_M (void *, _ArgType,
							 _ArgType);

// par_offload_minmax_element.tcc placeholder functions:

#define _PSTL_HSA_MINMAX_ELEMENT_CMP_PLACEHOLDER			\
  __hsa_callee_placeholder_minmax_element_cmp___________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________

template<typename _RetType, typename _ArgType>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_MINMAX_ELEMENT_CMP_PLACEHOLDER (_ArgType, _ArgType);

#define _PSTL_HSA_MINMAX_ELEMENT_CMP_PLACEHOLDER_M			\
  __hsa_callee_placeholder_minmax_element_cmp_m_________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________


template<typename _RetType, typename _ArgType>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_MINMAX_ELEMENT_CMP_PLACEHOLDER_M (void *, _ArgType,
							    _ArgType);

// par_offload_transform.tcc placeholder functions:

#define _PSTL_HSA_TRANSFORM_FUNCTOR_PLACEHOLDER				\
  __hsa_callee_placeholder_transform____________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________

template<typename _RetType, typename _ArgType>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_TRANSFORM_FUNCTOR_PLACEHOLDER (_ArgType);

#define _PSTL_HSA_TRANSFORM_FUNCTOR_PLACEHOLDER_M			\
  __hsa_callee_placeholder_m_transform__________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________

template<typename _RetType, typename _ArgType>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_TRANSFORM_FUNCTOR_PLACEHOLDER_M (void *, _ArgType);

// par_offload_transform_binary.tcc placeholder functions:

#define _PSTL_HSA_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER			\
  __hsa_callee_placeholder_transform_bin________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________

template<typename _RetType, typename _Arg1Type, typename _Arg2Type>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER (_Arg1Type,
							     _Arg2Type);

#define _PSTL_HSA_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER_M			\
  __hsa_callee_placeholder_m_transform_bin______________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________

template<typename _RetType, typename _Arg1Type, typename _Arg2Type>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER_M (void*,
							       _Arg1Type,
							       _Arg2Type);

// par_offload_transform_reduce.tcc placeholder functions:

#define _PSTL_HSA_TR_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER			\
  __hsa_callee_placeholder_tr_transform_bin_____________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________

template<typename _RetType, typename _Arg1Type, typename _Arg2Type>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_TR_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER (_Arg1Type,
								_Arg2Type);

#define _PSTL_HSA_TR_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER_M		\
  __hsa_callee_placeholder_m_tr_transform_bin___________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________

template<typename _RetType, typename _Arg1Type, typename _Arg2Type>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_TR_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER_M (void*,
								  _Arg1Type,
								  _Arg2Type);


#define _PSTL_HSA_TR_REDUCE_FUNCTOR_PLACEHOLDER				\
  __hsa_callee_placeholder_tr_reduce____________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________

template<typename _RetType, typename _Arg1Type, typename _Arg2Type>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_TR_REDUCE_FUNCTOR_PLACEHOLDER (_Arg1Type,
							 _Arg2Type);

#define _PSTL_HSA_TR_REDUCE_FUNCTOR_PLACEHOLDER_M			\
  __hsa_callee_placeholder_m_tr_reduce__________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________\
________________________________________________________________________

template<typename _RetType, typename _Arg1Type, typename _Arg2Type>
__attribute__((hsa_function,hsa_placeholder))
extern _RetType _PSTL_HSA_TR_REDUCE_FUNCTOR_PLACEHOLDER_M (void*,
							   _Arg1Type,
							   _Arg2Type);

// par_offload_transform_reduce_binary.tcc placeholder functions:

#pragma GCC diagnostic pop

#endif
