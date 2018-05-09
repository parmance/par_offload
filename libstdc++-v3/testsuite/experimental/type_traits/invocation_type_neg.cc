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

// { dg-do compile { target c++14 } }

// test cases for invocation_type use cases that are expected to cause
// compile time errors

#include <experimental/type_traits>

using std::experimental::is_same_v;

auto lambda_cap = [&](unsigned char c) -> char {
  return c + 1;
};

static_assert (is_same_v<std::experimental::raw_invocation_type< // { dg-error "template argument 1 is invalid" }
	       decltype(lambda_cap)(std::string)>::type,
	       char(unsigned char)>,
	       "Lambda: (w/ capture) illegal implicit arg conversion");

auto lambda_nocap = [](std::string str) -> unsigned char {
  return str[0];
};

static_assert (is_same_v<std::experimental::raw_invocation_type< // { dg-error "template argument 1 is invalid" }
	       decltype(lambda_nocap)(int)>::type,
	       unsigned char(std::string)>,
	       "Lambda (w/o capture): illegal implicit arg conversion");


// { dg-prune-output "no type named" }

