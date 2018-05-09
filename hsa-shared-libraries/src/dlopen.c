#include <dlfcn.h>
#include <stdio.h>

main()
{
  void * f = dlopen ("build/test.so", RTLD_NOW);
  void * obj = dlsym (f, "__brig_start");
  void * obj2 = dlsym (f, "__brig_end");

  fprintf (stderr, "start: %p, end: %p, size: %u\n", obj, obj2, obj2 - obj);
}
