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

// Test cases for for_each par_offload use cases that are expected to get
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

static void
toup(char& X)
{
  if (X >= 97 && X <= 122)
    X -= 32;
}

static void
tolow(char& X)
{
  if (X >= 65 && X <= 90)
    X += 32;
}

bool &offloaded = std::experimental::execution::__pstl_was_offloaded;
using std::experimental::execution::par_offload;

void
test_c_function_pointers()
{
  std::string s("c function pointers");
  std::for_each(par_offload, s.begin(), s.end(), toup);
  VERIFY(s == "C FUNCTION POINTERS");
  VERIFY(offloaded);

  auto f_ptr = tolow;
  std::for_each(par_offload, s.begin(), s.end(), f_ptr);
  VERIFY(s == "c function pointers");
  VERIFY(offloaded);
}

void
test_std_functions()
{
  std::string s2("std::functions");
  std::function<void(char&)> K = toup;
  std::for_each(par_offload, s2.begin(), s2.end(), K);
  VERIFY(s2 == "STD::FUNCTIONS");
  VERIFY(offloaded);

  // TODO: test std::functions wrapping a member function.
}

void
test_non_capturing_lambdas()
{
  std::string s3("non-capturing lambdas");

  // Note that the argument and result types do not match the element
  // type (char in this case). This tests the conversion code generation
  // via the templating mechanism.  */
  std::for_each(par_offload, s3.begin(), s3.end(),
		 [](char& c) -> void {
		   if (c == 'n')
		     c = 'm';
		   else if (c == 'm')
		     c = 'n';
		 });

  VERIFY(s3 == "mom-capturimg lanbdas");
  VERIFY(offloaded);

  std::for_each(par_offload, s3.begin(), s3.end(),
		 [](char& c) -> void {
		   if (c == 'm')
		     c = 'n';
		   else if (c == 'n')
		     c = 'm';
		 });

  VERIFY(s3 == "non-capturing lambdas");
  VERIFY(offloaded);
}

void
test_capturing_lambdas()
{
  int counter = 1000;
  int a = 2;
  std::string s4("capturing lambdas");
  auto l = [&counter, a](char& c) -> void {
    // TODO: This has a race.  Convert to an atomic when they work.
    ++counter;
    if (c != ' ')
      c = c + a;
  };
  std::for_each(par_offload, s4.begin(), s4.end(), l);

  VERIFY(s4 == "ecrvwtkpi ncodfcu");
  VERIFY(offloaded);
  // Due to the race we cannot assume more than this (not even this
  // in reality, but it should hold usually).
  VERIFY(counter > 1000);

  a = -2;

  std::for_each(par_offload, s4.begin(), s4.end(),
		 [&](char& c) -> void {
		   // TODO: This has a race.  Convert to an atomic when they work.
		   --counter;
		   if (c != ' ')
		     c = c + a;
		 });
  VERIFY(s4 == "capturing lambdas");
  VERIFY(offloaded);
  VERIFY(counter >= 1000);
}

void
test_custom_datatypes()
{
  struct LocalType
  {
    int memb = 0;
  };

  std::vector<LocalType> v1_in(16);

  std::for_each(par_offload, v1_in.begin(), v1_in.end(),
		 [](LocalType& a) -> void {
		    a.memb = 1;
		 });

  VERIFY(offloaded);

  for (auto& a : v1_in) {
    VERIFY(a.memb == 1);
  }
}

// Member function calls to non-virtual functions.
void
test_member_function_calls()
{
  struct LocalType {
    void accumulate(int a) { memb += a; }
    int memb = 0;
  };

  std::vector<LocalType> v1_in(16);

  std::for_each(par_offload, v1_in.begin(), v1_in.end(),
		 [](LocalType& a) -> void {
		   a.accumulate(11);
		 });

  for (auto& a : v1_in)
    VERIFY(a.memb == 11);

  VERIFY(offloaded);
}

void
test_static_functions()
{
  struct LocalType {
    static void accumulate_static(LocalType& t, int a) { t.memb += a; }
    int memb = 0;
  };

  std::vector<LocalType> v1_in(16);

  std::for_each(par_offload, v1_in.begin(), v1_in.end(),
		 [](LocalType& a) -> void {
		   LocalType::accumulate_static(a, 10);
		 });

  for (auto& a : v1_in)
    VERIFY(a.memb == 10);

  VERIFY(offloaded);
}

int main() {

  test_c_function_pointers();
  test_std_functions();
  test_non_capturing_lambdas();
  test_capturing_lambdas();
  test_custom_datatypes();
  test_static_functions();
  test_member_function_calls();

  return 0;
}
