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

// XFAIL test cases for invocation_type use cases that are yet to
// implement.

#include <experimental/type_traits>
#include <string>

using std::experimental::is_same_v;

class ClassA {
public:
  char operator()(unsigned char a) const { return a + 2; };
  char operator()() const { return 'a'; }
};

static_assert (is_same_v<std::experimental::raw_invocation_type< // { dg-error "template argument 1 is invalid" }
  	         ClassA(unsigned char)>::type, char(unsigned char)>,
	       "Functor: single arg, overloaded operator(), diff arg count");

class ClassB {
public:
  char operator()(unsigned char a) const { return a + 2; };
  char operator()(std::string s) const { return s[0]; }
};

static_assert (is_same_v<std::experimental::raw_invocation_type < // { dg-error "template argument 1 is invalid" }
  	         ClassB(unsigned char)>::type, char(unsigned char)>,
	       "Functor: single arg, overloaded operator(), same arg count");

// { dg-prune-output "decltype cannot resolve address of overloaded function" }
