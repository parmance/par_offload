#include <stdlib.h>
#include <stdint.h>

#pragma GCC optimize ("no-tree-loop-distribute-patterns")
#pragma omp declare target

void *
memcpy (void *dest, const void *src, size_t n)
{
  char *dest_char_ptr = (char *) dest;
  const char *src_char_ptr = (const char *) src;

  while (n)
    {
      *(dest_char_ptr++) = *(src_char_ptr++);
      n--;
    }

  return dest;
}

void *
mempcpy (void *dest, const void *src, size_t n)
{
  memcpy (dest, src, n);

  return dest + n;
}

void *
memset (void *s, int c, size_t n)
{
  char c_value = (char)c;

  char *dest_char_ptr = (char *) s;
  while (n)
    {
      *(dest_char_ptr++) = c_value;
      n--;
    }

  return s;
}

void
bzero (void *s, size_t n)
{
  memset (s, 0, n);
}

#pragma omp end declare target
