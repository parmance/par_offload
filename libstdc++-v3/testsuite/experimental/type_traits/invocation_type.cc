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

// test cases for invocation_type use cases that are expected to work
// see also invocation_type_neg.cc and invocation_type_todo.cc

#include <experimental/type_traits>
#include <string>

using std::experimental::is_same_v;

//// Single argument function objects.

auto lambda_cap = [&](unsigned char c) -> char {
  return c + 1;
};

typedef char (decltype(lambda_cap)::*expected1)(unsigned char) const;

static_assert (is_same_v<std::experimental::raw_invocation_type<
	         decltype(lambda_cap)(unsigned char)>::type, expected1>,
	       "Lambda (w/ capture): no implicit conversions");


static_assert (is_same_v<std::experimental::raw_invocation_type<
	         decltype(lambda_cap)(int)>::type, expected1>,
	       "Lambda (w/ capture): implicit arg conversion");


auto lambda_cap_ref_arg = [&](unsigned char& c) -> void {
  c += 1;
};

typedef void (decltype(lambda_cap_ref_arg)::*expected_ref_arg)(unsigned char&) const;

static_assert (is_same_v<std::experimental::raw_invocation_type<
	         decltype(lambda_cap_ref_arg)(unsigned char&)>::type,
	         expected_ref_arg>,
	       "Lambda (w/ capture): ref arg");

auto lambda_nocap = [](std::string str) -> unsigned char {
  return str[0];
};

typedef unsigned char (decltype(lambda_nocap)::*expected2)(std::string) const;

static_assert (is_same_v<std::experimental::raw_invocation_type<
	         decltype(lambda_nocap)(std::string)>::type, expected2>,
	       "Lambda (w/o capture): no implicit conversions");

static_assert (is_same_v<std::experimental::raw_invocation_type<
  	         decltype(lambda_nocap)(const char*)>::type, expected2>,
	       "Lambda (w/o capture): implicit arg conversion");


auto lambda_nocap_ref_arg = [](std::string& str) -> unsigned char {
  return str[0];
};

typedef unsigned char (decltype(lambda_nocap_ref_arg)::*expected2_ref_arg)
   (std::string&) const;

static_assert (is_same_v<std::experimental::raw_invocation_type<
	         decltype(lambda_nocap_ref_arg)(std::string&)>::type,
	       expected2_ref_arg>,
	       "Lambda (w/o capture): reference arg");

class ClassA {
public:
  virtual char operator()(unsigned char a) { return a + 2; };
};

class ClassB : public ClassA {
public:
};

typedef char (ClassA::*expected3)(unsigned char);

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       ClassA(unsigned char)>::arg<0>::type, unsigned char>,
	       "Functor: single arg, no implicit conversions (arg0)");

static_assert (is_same_v<std::experimental::raw_invocation_type<
	         ClassA(unsigned char)>::ret_type, char>,
	       "Functor: single arg, no implicit conversions (return value)");

static_assert (is_same_v<std::experimental::raw_invocation_type<
	         ClassA(unsigned char)>::type, expected3>,
	       "Functor: single arg, no implicit conversions");

static_assert (is_same_v<std::experimental::raw_invocation_type<
  	         ClassA(int)>::type, expected3>,
	       "Functor: single arg, with implicit conversions");

typedef char (ClassA::*expected4)(unsigned char);

static_assert (is_same_v<std::experimental::raw_invocation_type<
  	         ClassB(unsigned char)>::type, expected4>,
	       "Functor: single arg, derived, no implicit conversions");

static_assert (is_same_v<std::experimental::raw_invocation_type<
  	         ClassB(int)>::type, expected4>,
	       "Functor: single arg, derived, with implicit conversions");

class ClassC {
public:
  virtual char operator()(unsigned char a) const { return a + 2; };
};

typedef char (ClassC::*expected5)(unsigned char) const;

static_assert (is_same_v<std::experimental::raw_invocation_type<
  	         ClassC(unsigned char)>::type, expected5>,
	       "Functor with a const operator()");

class ClassCNoConst {
public:
  virtual char operator()(unsigned char a) { return a + 2; };
};

typedef char (ClassCNoConst::*expected6)(unsigned char);

static_assert (is_same_v<std::experimental::raw_invocation_type<
  	         ClassCNoConst(unsigned char)>::type, expected6>,
	       "Functor with a const operator()");

class ClassD {
public:
  virtual char operator()(unsigned char a) volatile { return a + 2; };
};

typedef char (ClassD::*expected7)(unsigned char) volatile;

static_assert (is_same_v<std::experimental::raw_invocation_type<
  	         ClassD(unsigned char)>::type, expected7>,
	       "Functor with a volatile operator()");

class ClassE {
public:
  virtual char operator()(unsigned char a) const volatile { return a + 2; };
};

typedef char (ClassE::*expected8)(unsigned char) const volatile;

static_assert (is_same_v<std::experimental::raw_invocation_type<
  	         ClassE(unsigned char)>::type, expected8>,
	       "Functor with a volatile const operator()");

char basic_c_func(std::string arg) { return arg[0]; }

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       decltype(&basic_c_func)(std::string)>::type, char(std::string)>,
	       "Basic C function, no implicit conversions");

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       decltype(&basic_c_func)(const char *)>::type, char(std::string)>,
	       "Basic C function, with implicit conversions");


//// Two argument function objects.

auto lambda_cap_2arg = [&](unsigned char c, int b) -> char {
  return c + b;
};

typedef char (decltype(lambda_cap_2arg)::*expected1_2arg)
             (unsigned char, int) const;

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       decltype(lambda_cap_2arg)(unsigned char, int)>::type,
	       expected1_2arg>,
	       "2-arg lambda (w/ capture): no implicit conversions");

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       decltype(lambda_cap_2arg)(int, int)>::type, expected1_2arg>,
	       "2-arg lambda (w/ capture): implicit arg conversion");

auto lambda_nocap_2arg = [](std::string str, int i) -> unsigned char {
  return str[i];
};

typedef unsigned char (decltype(lambda_nocap_2arg)::*expected2_2arg)
                      (std::string, int) const;

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       decltype(lambda_nocap_2arg)(std::string, int)>::type,
	         expected2_2arg>,
	       "2-arg lambda (w/o capture): no implicit conversions");

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       decltype(lambda_nocap_2arg)(const char*, char)>::type,
	         expected2_2arg>,
	       "2-arg lambda (w/o capture): implicit arg conversion");

class ClassA2 {
public:
  virtual char operator()(unsigned char a, int i) { return a + i; };
};

class ClassB2 : public ClassA2 {
public:
};

typedef char (ClassA2::*expected3_2arg)(unsigned char, int);

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       ClassA2(unsigned char, int)>::arg<0>::type, unsigned char>,
	       "2-arg functor: single arg, no implicit conversions (arg0)");

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       ClassA2(unsigned char, int)>::arg<1>::type, int>,
	       "2-arg functor: single arg, no implicit conversions (arg1)");

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       ClassA2(unsigned char, int)>::ret_type, char>,
	       "2-arg functor: single arg, no implicit conversions "
	       "(return value)");

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       ClassA2(unsigned char, int)>::type, expected3_2arg>,
	       "2-arg functor: single arg, no implicit conversions");

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       ClassA2(char, unsigned)>::type, expected3_2arg>,
	       "2-arg functor: single arg, with implicit conversions");

typedef char (ClassA2::*expected4_2arg)(unsigned char, int);

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       ClassB2(unsigned char, int)>::type, expected4_2arg>,
	       "2-arg functor: single arg, derived, no implicit conversions");

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       ClassB2(int, int)>::type, expected4_2arg>,
	       "2-arg functor: single arg, derived, with implicit conversions");

class ClassC2 {
public:
  virtual char operator()(unsigned char a, int i) const { return a + i; };
};

typedef char (ClassC2::*expected5_2arg)(unsigned char, int) const;

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       ClassC2(unsigned char, int)>::type, expected5_2arg>,
	       "2-arg functor with a const operator()");

class ClassCNoConst2 {
public:
  virtual char operator()(unsigned char a, int i) { return a + i; };
};

typedef char (ClassCNoConst2::*expected6_2arg)(unsigned char, int);

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       ClassCNoConst2(unsigned char, int)>::type, expected6_2arg>,
	       "2-arg functor with a const operator()");

class ClassD2 {
public:
  virtual char operator()(unsigned char a, int i) volatile
  { return a + i; };
};

typedef char (ClassD2::*expected7_2arg)(unsigned char, int) volatile;

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       ClassD2(unsigned char, int)>::type, expected7_2arg>,
	       "2-arg functor with a volatile operator()");

class ClassE2 {
public:
  virtual char operator()(unsigned char a, int i) const volatile
  { return a + i; };
};

typedef char (ClassE2::*expected8_2arg)(unsigned char, int) const volatile;

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       ClassE2(unsigned char, int)>::type, expected8_2arg>,
	       "2-arg functor with a volatile const operator()");

char basic_c_func_2(std::string arg, int i) { return arg[i]; }

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       decltype(&basic_c_func_2)(std::string, int)>::type,
	       char(std::string, int)>,
	       "basic 2-arg C function, no implicit conversions");

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       decltype(&basic_c_func_2)(const char *, char)>::type,
	       char(std::string, int)>,
	       "basic 2-arg C function, with implicit conversions");

const char& basic_c_func_with_ref_ret_val(const std::string& arg, int i) {
  return arg[i];
}

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       decltype(&basic_c_func_with_ref_ret_val)(std::string, int)>::type,
	       const char&(const std::string&, int)>,
	       "function with a reference return value");

struct CustomType {
  long a, b, c, d, e;
};

auto lambda_custom_type = [](const CustomType& a) {
  CustomType x;
  x.a = 3;
  return x;
};

typedef CustomType (decltype(lambda_custom_type)::*expected_custom)(const CustomType&) const;

static_assert (is_same_v<std::experimental::raw_invocation_type<
	       decltype(lambda_custom_type)(CustomType)>::type,
	       expected_custom>,
	       "lambda with a struct return value");
