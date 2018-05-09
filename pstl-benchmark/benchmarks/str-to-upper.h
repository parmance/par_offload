/*
  Converts a huge string to upper case using transform.

  The toup function has a simple branch inside, thus is not completely
  trivial to execute in SIMD fashion.

  (c) 2017-2018 Pekka Jääskeläinen / Parmance
*/

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

// User defined function that should be converted to "universal functions"
// automatically.

void toup(char& X) {
  if (X >= 97 && X <= 122)
    X -= 32;
}

template <typename ExecutionPolicy>
Duration
huge_string_to_upper() {

#define HUGE_STRING_SIZE (100*1024*1024)

#ifndef HSA_BASE_PROFILE
  char *huge_string = new char[HUGE_STRING_SIZE];
#else
  char *huge_string = (char*)hsa_alloc_shared(HUGE_STRING_SIZE);
  assert(huge_string != nullptr);
#endif

  for (size_t i = 0; i < HUGE_STRING_SIZE; ++i) {
    huge_string[i] = 'a';
  }

  auto start_t = std::chrono::high_resolution_clock::now();

  ExecutionPolicy exec;
  std::for_each(exec, huge_string, huge_string + HUGE_STRING_SIZE, toup);
  auto end_t = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<Duration>(end_t - start_t);

  if constexpr (VERIFY_RESULTS) {
    for (size_t i = 0; i < HUGE_STRING_SIZE; ++i) {
      if (huge_string[i] != 'A') {
	std::cerr << "Wrong data at " << i << ": " << huge_string[i]
		  << std::endl;
	abort();
      }
    }
  }
#ifdef HSA_BASE_PROFILE
  hsa_memory_free(huge_string);
#else
  delete[] huge_string;
#endif
  return duration;
}
