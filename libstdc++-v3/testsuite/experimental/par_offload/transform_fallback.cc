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

// Test cases for transform par_offload use cases that are expected to
// fallback to host execution.

// This gives us std::experimental::execution::__pstl_was_offloaded;
#define _TESTING_PAR_OFFLOAD

#include <experimental/par_offload>

bool &offloaded = std::experimental::execution::__pstl_was_offloaded;
using std::experimental::execution::par_offload;

#include <cctype>
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <testsuite_hooks.h>

static char
toup(char X)
{
  if (X >= 97 && X <= 122)
    return X - 32;
  else
    return X;
}

static char
tolow(char X)
{
  if (X >= 65 && X <= 90)
    return X + 32;
  else
    return X;
}

// Indirect function calls inside the functor are not
// offloaded at the moment.
void
test_indirect_function_calls()
{
  std::string s5("fallback to host 1");
  auto fptr = toup;
  std::transform(par_offload, s5.begin(), s5.end(), s5.begin(),
		 [fptr](char c) -> char {
		   return fptr(c);
		 });

  VERIFY(s5 == "FALLBACK TO HOST 1");
  VERIFY(!offloaded);
}

int main() {

  test_indirect_function_calls();

  return EXIT_SUCCESS;
}
