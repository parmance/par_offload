// Offloading 'copy_if' algorithm -*- C++ -*-

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

/** @file experimental/bits/par_offload_copy_if.tcc
 *  This is an internal header file included by the par_offload header.
 *  Do not attempt to use it directly.
 *
 *  This header has an implementation of experimental 'par_offload'
 *  execution policy for 'copy_if' on top of HSA Runtime.
 */

#ifndef _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_COPY_IF_TCC
#define _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_COPY_IF_TCC 1

#include <algorithm>
#include <vector>
#include <numeric>
#include <iostream>
#include <cassert>
#include <experimental/type_traits>
#include <iterator>
#include <experimental/bits/par_offload_helpers.tcc>

#pragma GCC system_header

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

namespace experimental
{
namespace execution
{
  /// Offloading 'copy_if' implementation using HSA.
  template<
    class _ExecutionPolicy,
    class _InputIt,
    class _OutputIt,
    class _UnaryPredicate,
    class _Enabler = typename std::enable_if<
      __par_offload_ok<_ExecutionPolicy, _InputIt, _OutputIt>>::type
    >
    _OutputIt
    copy_if(_ExecutionPolicy&& __policy,
	    _InputIt __first, _InputIt __last,
	    _OutputIt __d_first, _UnaryPredicate __pred) {

       size_t __data_size = std::distance(__first, __last);
       std::vector<char> __copy_mask(__data_size);

       if (__data_size > 1024)
	 std::experimental::execution::transform(
	   __policy, __first, __last, __copy_mask.begin(), __pred);
       else
	 std::transform(__first, __last, __copy_mask.begin(), __pred);

       _OutputIt __d = __d_first;
       for (size_t __i = 0; __i < __copy_mask.size(); ++__i, ++__first) {
	 if (__copy_mask[__i])
	   *(__d++) = *__first;
       }
       return __d;
    }
} // namespace execution
} // namespace experimental
  using std::experimental::execution::copy_if;
} // namespace std

#endif
