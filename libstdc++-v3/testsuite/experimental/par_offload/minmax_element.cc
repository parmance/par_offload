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

// Test cases for minmax_element with par_offload exec policy.

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

  auto res1 =
    std::minmax_element(par_offload, src.begin(), src.end());

  VERIFY(offloaded);
  VERIFY(res1.first == src.begin());
  VERIFY(res1.second == src.end() - 1);

  src[5] = 500000;

  auto res2 =
    std::minmax_element(par_offload, src.begin(), src.end());

  VERIFY(offloaded);
  VERIFY(res2.first == src.begin());
  VERIFY(res2.second == src.begin() + 5);

  src[0] = 1;
  src[100000] = 1;
  src[100001] = 500001;
  src[200000] = 500001;
  src[499999] = 1;

  auto res3 =
    std::minmax_element(par_offload, src.begin(), src.end());

  VERIFY(offloaded);
  // Should return the iterator to the first '1', the smallest element at
  // the smallest index 0.
  VERIFY(res3.first == src.begin());
  // The last biggest element is at 200000, not at 100001.
  VERIFY(res3.second == src.begin() + 200000);
}

void
test_corner_cases() {

  std::vector<int> src(1025);
  for (size_t i = 0; i < src.size(); ++i) {
    src[i] = i;
  }

  // Zero items.
  auto res1 =
    std::minmax_element(par_offload, src.begin(), src.begin());

  VERIFY(!offloaded);
  VERIFY(res1.first == src.begin());
  VERIFY(res1.second == src.begin());

  // 1 item.
  auto res2 =
    std::minmax_element(par_offload, src.begin(), src.begin() + 1);
  VERIFY(offloaded);
  VERIFY(res2.first == src.begin());
  VERIFY(res2.second == src.begin());

  // Less than 1024 parallel reduces.
  auto res3 =
    std::minmax_element(par_offload, src.begin(), src.end() - 3);
  VERIFY(offloaded);
  VERIFY(res3.first == src.begin());
  VERIFY(res3.second == src.end() - 4);

  src[0] = 2;
  // 1 leftover item (over 1024).
  auto res4 =
    std::minmax_element(par_offload, src.begin(), src.end());
  VERIFY(offloaded);
  // The smallest value is now '1' at 1.
  VERIFY(res4.first == src.begin() + 1);
  VERIFY(res4.second == src.end() - 1);
}


struct GlobalType
{
  long memb = 0;
  struct SecondType {
    long foo;
    long bar;
    long more;
    long to_force;
    long the_result_passed_as_a_reference;
    long right;
    long now;
  } x;
  GlobalType() : memb(0) {}
  GlobalType(long m) : memb(m) {}
  bool operator < (const GlobalType & other) const {
    return (this->memb < other.memb ?
	    true : this->x.bar < other.x.bar);
  }
  bool operator == (const GlobalType & other) const {
    return (this->memb == other.memb);
  }

};

static auto
sub_function(std::vector<GlobalType>& v1_in, std::vector<GlobalType>& v2_in) {

  return std::minmax_element(par_offload, v1_in.begin(), v1_in.end());
}

void
test_global_custom_datatype()
{

  std::vector<GlobalType> v1_in(16, GlobalType(1));
  std::vector<GlobalType> v2_in(16, GlobalType(2));

  auto res = sub_function(v1_in, v2_in);
#if 0
  for (auto& a : out)
    VERIFY(a.memb == 3);
#endif
  VERIFY(offloaded);
}



int main() {
  test_basic();
  test_corner_cases();
  return 0;
}
