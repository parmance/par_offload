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
// { dg-options "-fpar-offload" }
// { dg-require-par-offload "" }

// Test cases for transform_reduce par_offload binary operation use cases
// that are expected to get offloaded.

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

int plus_c(int a, int b) {
  return a + b;
}

int mul_c(int a, int b) {
  return a * b;
}

void
test_dot_product()
{
  std::vector<int> vals1(1000001);
  std::vector<int> vals2(1000001);

  for (size_t i = 0; i < vals1.size(); ++i) {
    vals1[i] = i;
    vals2[i] = vals1.size() - i;
  }

  int par_offload_result =
    std::transform_reduce(par_offload, vals1.begin(), vals1.end(), vals2.begin(),
			  0, std::plus<int>(), std::multiplies<int>());

  int par_offload_c_func_result =
    std::transform_reduce(par_offload, vals1.begin(), vals1.end(), vals2.begin(),
			  0, plus_c, mul_c);

  int serial_result =
    std::transform_reduce(vals1.begin(), vals1.end(), vals2.begin(),
			  0, std::plus<int>(), std::multiplies<int>());

  if (par_offload_result != serial_result)
    {
      std::cerr << par_offload_result << " is not " << serial_result
		<< std::endl;
      VERIFY(par_offload_result == serial_result);
    }
  VERIFY(par_offload_result == serial_result);
  VERIFY(par_offload_c_func_result == serial_result);
  VERIFY(offloaded);
}

int main() {

  test_dot_product();
  return 0;
}
