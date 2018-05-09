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

// Test cases for transform par_offload binary operation use cases that are
// expected to get offloaded.

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

static char
dec(char X, signed int D)
{
  if (X >= 97 && X <= 122)
    return X - D;
  else
    return X;
}

static char
inc(char X, unsigned int D)
{
  if (X >= 65 && X <= 90)
    return X + D;
  else
    return X;
}

bool &offloaded = std::experimental::execution::__pstl_was_offloaded;
using std::experimental::execution::par_offload;

void
test_c_function_pointers()
{
  std::vector<char> vals(50, 32);
  std::string s("c function pointers"); //...
  std::transform(par_offload, s.begin(), s.end(), vals.begin(), s.begin(),
		 dec);
  VERIFY(s == "C FUNCTION POINTERS");
  VERIFY(offloaded);

  auto f_ptr = inc;
  std::transform(par_offload, s.begin(), s.end(), vals.begin(), s.begin(),
		 f_ptr);
  VERIFY(s == "c function pointers");
  VERIFY(offloaded);
}

void
test_std_functions()
{
  std::vector<char> vals(50, 32);
  std::string s2("std::functions");
  std::function<char(char, signed)> K = dec;
  std::transform(par_offload, s2.begin(), s2.end(), vals.begin(),
		 s2.begin(), K);
  VERIFY(s2 == "STD::FUNCTIONS");
  VERIFY(offloaded);

  // TODO: test std::functions wrapping a member function.
}

void
test_non_capturing_lambdas()
{
  std::vector<char> vals(50, 'm');
  std::string s3("non-capturing lambdas");

  // Note that the argument and result types do not match the element
  // type (char in this case). This tests the conversion code generation
  // via the templating mechanism.  */
  std::transform(par_offload, s3.begin(), s3.end(), vals.begin(), s3.begin(),
		 [](unsigned c, char r) -> int {
		   if (c == 'n')
		     return r;
		   else if (c == r)
		     return 'n';
		   else
		     return c;
		 });

  VERIFY(s3 == "mom-capturimg lanbdas");
  VERIFY(offloaded);

  std::transform(par_offload, s3.begin(), s3.end(), vals.begin(), s3.begin(),
		 [](unsigned char c, char r) -> char {
		   if (c == r)
		     return 'n';
		   else if (c == 'n')
		     return r;
		   else
		     return c;
		 });

  VERIFY(s3 == "non-capturing lambdas");
  VERIFY(offloaded);
}

void
test_capturing_lambdas()
{
  std::vector<char> vals(50, 1);
  int counter = 1000;
  int a = 1;
  std::string s4("capturing lambdas");
  auto l = [&counter, a](unsigned c, char k) -> int {
    // TODO: This has a race.  Convert to an atomic when they work.
    ++counter;
    if (c != ' ')
      return c + a + k;
    else
      return c;
  };
  std::transform(par_offload, s4.begin(), s4.end(), vals.begin(),
		 s4.begin(), l);

  VERIFY(s4 == "ecrvwtkpi ncodfcu");
  VERIFY(offloaded);
  VERIFY(counter > 1000);

  a = -1;

  std::transform(par_offload, s4.begin(), s4.end(), vals.begin(),
		 s4.begin(),
		 [&](unsigned c, char k) -> char {
		   // TODO: This has a race.
		   // Convert to an atomic when they work in HSABE.
		   --counter;
		   if (c != ' ')
		     return c + a - k;
		   else
		     return c;
		 });
  VERIFY(s4 == "capturing lambdas");
  VERIFY(offloaded);
  VERIFY(counter >= 1000);
}

void
test_local_custom_datatype()
{
  struct LocalType
  {
    long memb = 0;
    struct SecondType {
      long foo;
      long bar;
      long more;
      long to_force;
      long the_result_passed_as_a_reference;
    } x;
    LocalType() : memb(0) {}
    LocalType(long m) : memb(m) {}
    bool operator < (const LocalType & other) const {
      return (this->memb < other.memb ?
	      true : this->x.bar < other.x.bar);
    }
  };

  std::vector<LocalType> v1_in(16, LocalType(1));
  std::vector<LocalType> v2_in(16, LocalType(2));

  std::vector<LocalType> v1_out(16);

  std::transform(par_offload, v1_in.begin(), v1_in.end(), v2_in.begin(),
		 v1_out.begin(),
		 [](const LocalType& a, const LocalType& b) {
		   LocalType c;
		   if (a < b)
		     c.memb = a.memb + b.memb;
		   else
		     c.memb = 99;
		   return c;
		 });
  for (auto& a : v1_out)
    VERIFY(a.memb == 3);
  VERIFY(offloaded);
}

int main() {

  test_c_function_pointers();
  test_std_functions();
  test_non_capturing_lambdas();
  test_capturing_lambdas();
  test_local_custom_datatype();
  return 0;
}
