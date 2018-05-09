/* Basic test for __builtin_GOMP_direct_invoke_spmd_kernel() */
/* { dg-additional-options "-fdirecthsa" } */


#include <stdlib.h>
#include <stddef.h>

#define VERIFY(CONDITION) \
  do { if (!CONDITION) abort (); } while (0)

/* Test kernel that writes the flatabsid to the flatabsid
   index in the array.  */
static void __attribute__((hsa_kernel))
test_kernel_get_ids (void *__args)
{
  int *vals = *(int**)__args;
  size_t i = __builtin_hsa_workitemflatabsid ();

  vals[i] = i + 1;
}

int
main(int argc, char* argv[])
{

#define DIM 10
#define NWORKITEMS (DIM*DIM*DIM)

  int buf[NWORKITEMS + 1];
  buf[NWORKITEMS] = 99;

  int *arg = &buf[0];

  int status = __builtin_GOMP_direct_invoke_spmd_kernel
    (0, (void*)test_kernel_get_ids, DIM, 0, DIM, 0, DIM, 0, &arg,
     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

  VERIFY (status == 0);

  for (size_t i = 0; i < NWORKITEMS; ++i)
    VERIFY (buf[i] == i + 1);
}
