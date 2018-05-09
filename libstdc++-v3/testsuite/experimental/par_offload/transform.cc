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

// Test cases for transform par_offload use cases that are expected to get
// offloaded.

// This gives us std::experimental::execution::__pstl_was_offloaded;
#define _TESTING_PAR_OFFLOAD

#include <experimental/par_offload>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <atomic>
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

bool &offloaded = std::experimental::execution::__pstl_was_offloaded;
using std::experimental::execution::par_offload;

void
test_c_function_pointers()
{
  std::string s("c function pointers");
  std::transform(par_offload, s.begin(), s.end(), s.begin(), toup);
  VERIFY(s == "C FUNCTION POINTERS");
  VERIFY(offloaded);

  auto f_ptr = tolow;
  std::transform(par_offload, s.begin(), s.end(), s.begin(), f_ptr);
  VERIFY(s == "c function pointers");
  VERIFY(offloaded);
}

void
test_std_functions()
{
  std::string s2("std::functions");
  std::function<char(char)> K = toup;
  std::transform(par_offload, s2.begin(), s2.end(), s2.begin(), K);
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
  std::transform(par_offload, s3.begin(), s3.end(), s3.begin(),
		 [](unsigned c) -> int {
		   if (c == 'n')
		     return 'm';
		   else if (c == 'm')
		     return 'n';
		   else
		     return c;
		 });

  VERIFY(s3 == "mom-capturimg lanbdas");
  VERIFY(offloaded);

  std::transform(par_offload, s3.begin(), s3.end(), s3.begin(),
		 [](unsigned char c) -> char {
		   if (c == 'm')
		     return 'n';
		   else if (c == 'n')
		     return 'm';
		   else
		     return c;
		 });

  VERIFY(s3 == "non-capturing lambdas");
  VERIFY(offloaded);
}

void
test_capturing_lambdas()
{
  std::atomic<int> counter(1000);
  int a = 2;
  std::string s4("capturing lambdas");
  auto l = [&counter, a](unsigned c) -> int {
    // TODO: This has a race.  Convert to an atomic when they work.
    ++counter;
    if (c != ' ')
      return c + a;
    else
      return c;
  };
  std::transform(par_offload, s4.begin(), s4.end(), s4.begin(), l);

  VERIFY(s4 == "ecrvwtkpi ncodfcu");
  VERIFY(offloaded);
  VERIFY(counter > 1000);

  a = -2;

  std::transform(par_offload, s4.begin(), s4.end(), s4.begin(),
		 [&](unsigned c) -> char {
		   counter -= 2;
		   if (c != ' ')
		     return c + a;
		   else
		     return c;
		 });
  VERIFY(s4 == "capturing lambdas");
  VERIFY(offloaded);
  VERIFY(counter == 1000 - s4.size());
}

#if 0
// Currently the iterator validity is checked at compile time,
// if it's moved to a runtime check with a fallback, enable this
// test.
void
test_unsupported_containers()
{
  std::set<int> in;
  std::set<int> out;

  std::transform(par_offload, in.begin(), in.end(), out.begin(),
		 [](int a) -> int {
		   return a + 1;
		 });
  VERIFY(!offloaded);
}
#endif

void
test_custom_datatypes()
{
  struct LocalType
  {
    int memb = 0;
  };

  std::vector<LocalType> v1_in(16);
  std::vector<LocalType> v1_out(16);

  std::transform(par_offload, v1_in.begin(), v1_in.end(), v1_out.begin(),
		 [](LocalType a) -> LocalType {
		   a.memb = 1;
		   return a;
		 });

  for (auto& a : v1_out)
    VERIFY(a.memb == 1);

  VERIFY(offloaded);
}

// Member function calls to non-virtual functions should work.
void
test_member_function_calls()
{
  struct LocalType {
    void accumulate(int a) { memb += a; }
    int memb = 0;
  };

  std::vector<LocalType> v1_in(16);
  std::vector<LocalType> v1_out(16);

  std::transform(par_offload, v1_in.begin(), v1_in.end(), v1_out.begin(),
		 [](LocalType a) -> LocalType {
		   a.accumulate(11);
		   return a;
		 });

  for (auto& a : v1_out)
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
  std::vector<LocalType> v1_out(16);

  std::transform(par_offload, v1_in.begin(), v1_in.end(), v1_out.begin(),
		 [](LocalType a) -> LocalType {
		   LocalType::accumulate_static(a, 10);
		   return a;
		 });

  for (auto& a : v1_out)
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
