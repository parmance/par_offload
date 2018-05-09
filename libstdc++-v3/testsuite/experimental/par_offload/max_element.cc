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

// Test cases for max_element with par_offload exec policy.

// This gives us std::experimental::execution::__pstl_was_offloaded;
#define _TESTING_PAR_OFFLOAD

#include <experimental/par_offload>
#include <vector>
#include <functional>
#include <testsuite_hooks.h>

bool &offloaded = std::experimental::execution::__pstl_was_offloaded;
using std::experimental::execution::par_offload;

void
test_basic() {

  std::vector<int> src(500000);
  for (size_t i = 0; i < src.size(); ++i) {
    src[i] = i;
  }

  auto maxres1 =
    std::max_element(par_offload, src.begin(), src.end());

  VERIFY(offloaded);
  VERIFY(maxres1 == src.end() - 1);

  src[5] = 1000000;

  auto maxres2 =
    std::max_element(par_offload, src.begin(), src.end());

  VERIFY(offloaded);
  VERIFY(maxres2 == src.begin() + 5);

  src[0] = 1;
  src[100000] = 1;
  src[499999] = 1;
  auto minres1 =
    std::max_element(par_offload, src.begin(), src.end(), std::greater<int>());

  VERIFY(offloaded);
  // It should return the iterator to the first '1', the smallest element at
  // the smallest index 0.
  VERIFY(minres1 == src.begin());
}

void
test_corner_cases() {

  std::vector<int> src(1025);
  for (size_t i = 0; i < src.size(); ++i) {
    src[i] = i;
  }

  // Zero items.
  auto res1 =
    std::max_element(par_offload, src.begin(), src.begin());

  VERIFY(!offloaded);
  VERIFY(res1 == src.begin());

  // 1 item.
  auto res2 =
    std::max_element(par_offload, src.begin(), src.begin() + 1);
  VERIFY(offloaded);
  VERIFY(res2 == src.begin());

  // Less than 1024 parallel reduces.
  auto res3 =
    std::max_element(par_offload, src.begin(), src.end() - 3);
  VERIFY(offloaded);
  VERIFY(res3 == src.end() - 4);

  // 1 leftover item (over 1024).
  auto res4 =
    std::max_element(par_offload, src.begin(), src.end());
  VERIFY(offloaded);
  VERIFY(res4 == src.end() - 1);
}

int main() {
  test_basic();
  test_corner_cases();
  return 0;
}

