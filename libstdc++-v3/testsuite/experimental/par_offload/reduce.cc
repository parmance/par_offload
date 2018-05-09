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

// Test cases for reduce par_offload use cases that are expected to get
// offloaded.

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
test_corner_cases()
{
  std::vector<int> v1(4);
  for (size_t i = 0; i < 4; ++i) {
    v1[i] = i;
  }
  int result1 =
    std::reduce(par_offload, v1.cbegin(), v1.cend(), 0, std::plus<int>());
  if (result1 != 6) {
    std::cout << "result1 == " << result1 << std::endl;
    VERIFY(result1 == 6);
  }
  VERIFY(!offloaded);

  int result2 =
    std::reduce(par_offload, v1.begin(), v1.end(), 4, std::plus<int>());
  VERIFY(result2 == result1 + 4);
  VERIFY(!offloaded);

  std::vector<int> v2(3);
  for (size_t i = 0; i < 3; ++i) {
    v2[i] = i;
  }
  int result3 =
    std::reduce(par_offload, v2.cbegin(), v2.cend(), 0, std::plus<int>());
  VERIFY(result3 == 3);
  VERIFY(!offloaded);

  int result4 =
    std::reduce(par_offload, v2.begin(), v2.end(), -4, std::plus<int>());
  VERIFY(result4 == result3 - 4);
  VERIFY(!offloaded);

  std::vector<int> v3(2);
  for (size_t i = 0; i < 2; ++i) {
    v3[i] = i;
  }
  int result5 =
    std::reduce(par_offload, v3.cbegin(), v3.cend(), 0, std::plus<int>());
  VERIFY(result5 == 1);
  VERIFY(!offloaded);

  int result6 =
    std::reduce(par_offload, v3.begin(), v3.end(), -4, std::plus<int>());
  VERIFY(result6 == result5 - 4);
  VERIFY(!offloaded);

  std::vector<int> v4(1);
  v4[0] = 1;
  int result7 =
    std::reduce(par_offload, v4.cbegin(), v4.cend(), 0, std::plus<int>());
  VERIFY(result7 == 1);
  VERIFY(!offloaded);

  int result8 =
    std::reduce(par_offload, v4.begin(), v4.end(), -4, std::plus<int>());
  VERIFY(result8 == result7 - 4);
  VERIFY(!offloaded);

  std::vector<int> empty;
  int result9 =
    std::reduce(par_offload, empty.begin(), empty.end(), 25, std::plus<int>());
  VERIFY(result9 == 25);
  VERIFY(!offloaded);

  // The first offloaded case.
  std::vector<int> voff1(1001);
  for (size_t i = 0; i < 1001; ++i) {
    voff1[i] = i;
  }
  int result10 =
    std::reduce(par_offload, voff1.cbegin(), voff1.cend(), 0,
		std::plus<int>());
  VERIFY(result10 == 500500);
  VERIFY(offloaded);

  int result11 =
    std::reduce(par_offload, voff1.begin(), voff1.end(), -4,
		std::plus<int>());
  VERIFY(result11 == result10 - 4);
  VERIFY(offloaded);

  // 2nd uneven size offloaded case.
  std::vector<int> voff2(1003);
  int expected = 0;
  for (size_t i = 0; i < 1003; ++i) {
    voff2[i] = i;
    expected += i;
  }
  int result12 =
    std::reduce(par_offload, voff2.cbegin(), voff2.cend(), 0,
		std::plus<int>());
  VERIFY(result12 == expected);
  VERIFY(offloaded);

  int result13 =
    std::reduce(par_offload, voff2.begin(), voff2.end(), -4,
		std::plus<int>());
  VERIFY(result13 == result12 - 4);
  VERIFY(offloaded);
}

int main() {
  test_corner_cases();

  return 0;
}

