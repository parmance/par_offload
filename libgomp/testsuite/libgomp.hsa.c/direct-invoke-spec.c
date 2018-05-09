/* Test for __builtin_GOMP_direct_invoke_spmd_kernel()'s function callee
   specialization feature. */
/* { dg-additional-options "-fdirecthsa" } */

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define VERIFY(CONDITION)						\
  do {									\
    if (!(CONDITION))							\
      {									\
	fprintf (stderr, #CONDITION " failed in line %d.", __LINE__);	\
	abort ();							\
      }									\
  } while (0)

__attribute__((hsa_placeholder))
int __hsa_callee_placeholder_bin_operation0(int a, int b);

__attribute__((hsa_placeholder))
int __hsa_callee_placeholder_bin_operation1(int a, int b);

__attribute__((hsa_placeholder))
int __hsa_callee_placeholder_bin_operation2(int a, int b);

__attribute__((hsa_placeholder))
int __hsa_callee_placeholder_bin_operation3(int a, int b);

__attribute__((hsa_universal))
int adder (int a, int b)
{
  return a + b;
}

__attribute__((hsa_universal))
int subtractor (int a, int b)
{
  return a - b;
}
 __attribute__((hsa_kernel))
void
test_kernel (void *args)
{
  int *vals = *(int**)args;
  size_t i = __builtin_hsa_workitemflatabsid ();

  vals[i] = __hsa_callee_placeholder_bin_operation0(i, 1);
  vals[i] = __hsa_callee_placeholder_bin_operation1(vals[i], 1);
  vals[i] = __hsa_callee_placeholder_bin_operation2(vals[i], 1);
  vals[i] = __hsa_callee_placeholder_bin_operation3(vals[i], 1);
}

int
main(int argc, char* argv[])
{

#define DIM 10
#define NWORKITEMS (DIM*DIM*DIM)

  int buf[NWORKITEMS + 1];
  buf[NWORKITEMS] = 99;

  int *arg = &buf[0];

  void *adder_dev_handle
    = __builtin_GOMP_host_to_device_fptr (0, adder);
  VERIFY ((uint64_t)adder_dev_handle != UINT64_MAX);

  void *subtractor_dev_handle
    = __builtin_GOMP_host_to_device_fptr (0, subtractor);
  VERIFY ((uint64_t)subtractor_dev_handle != UINT64_MAX);

  int status = __builtin_GOMP_direct_invoke_spmd_kernel
    (0, (void*)test_kernel, DIM, 0, DIM, 0, DIM, 0, &arg,
     (void*)__hsa_callee_placeholder_bin_operation0,
     adder_dev_handle,
     (void*)__hsa_callee_placeholder_bin_operation1,
     subtractor_dev_handle,
     (void*)__hsa_callee_placeholder_bin_operation2,
     adder_dev_handle,
     (void*)__hsa_callee_placeholder_bin_operation3,
     adder_dev_handle);

  VERIFY (status == 0);

  /* Check that we can call universal functions from the host.  */
  VERIFY (adder (1, 2) == 3);

  for (size_t i = 0; i < NWORKITEMS; ++i)
    {
      if (buf[i] != i + 2)
	fprintf (stderr, "buf[%d] != %d like expected, but %d\n", i, i + 2,
		 buf[i]);
      VERIFY (buf[i] == i + 2);
    }
}
