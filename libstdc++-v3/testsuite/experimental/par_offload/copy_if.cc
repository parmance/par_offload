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

// { dg-do run { target c++14 } }
// { dg-options "-fpar-offload -Wno-hsa" }
// { dg-require-par-offload "" }

// Test cases for copy_if par_offload exec mode.

// This gives us std::experimental::execution::__pstl_was_offloaded;
#define _TESTING_PAR_OFFLOAD

#include <experimental/par_offload>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <testsuite_hooks.h>

bool &offloaded = std::experimental::execution::__pstl_was_offloaded;
using std::experimental::execution::par_offload;

void
test_basic() {

  std::vector<int> src(500000);
  for (size_t i = 0; i < src.size(); ++i) {
    src[i] = i;
  }

  std::vector<int> even(500000);
#if 0
  // TO FIX: This lambda generates wrong HSA code, it ends up
  // with a negation (not) of the whole input converted to a char,
  // which is of course wrong.
  auto last =
    std::copy_if(par_offload, src.begin(), src.end(), even.begin(),
		 [](const int& e) {
		   return (e % 2) == 0;
		 });
#elif 0
  // This fails similarly. Produces constant 0 as the return value.
  auto last =
    std::copy_if(par_offload, src.begin(), src.end(), even.begin(),
		 [](const int& e) {
		   return (e & 0x01) == 0;
		 });
#else
  // This works. Perhaps there's something broken with constant
  // handling.
  int mask = 0x01;
  auto last =
    std::copy_if(par_offload, src.begin(), src.end(), even.begin(),
		 [mask](const int& e) {
		   return (e & mask) == 0;
		 });
#endif

  VERIFY(offloaded);

  size_t res_size = std::distance(even.begin(), last);

  VERIFY(res_size == src.size() / 2);

  last--;
  VERIFY(*last == 499998);

  for (size_t i = 0; i < res_size; ++i) {
    VERIFY(even[i] == i*2);
  }
}

int main() {
  test_basic();

  return 0;
}

