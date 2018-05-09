// Offloading 'reduce' algorithm -*- C++ -*-

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

/** @file experimental/bits/par_offload_reduce.tcc
 *  This is an internal header file included by the par_offload header.
 *  Do not attempt to use it directly.
 *
 *  This header has an implementation of experimental 'par_offload'
 *  execution policy for the 'reduce' on top of HSA Runtime.
 */

#ifndef _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_REDUCE_TCC
#define _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_REDUCE_TCC 1

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
  /// Offloading 'reduce' implementation using HSA.
  template<
    class _ExecutionPolicy,
    class _ForwardIt,
    class _T,
    class _BinaryOperation,
    class _Enabler = typename std::enable_if<
      __par_offload_ok<_ExecutionPolicy, _ForwardIt>>::type
    >
    _T
    reduce(_ExecutionPolicy&& __policy,
	   _ForwardIt __first, _ForwardIt __last,
	   _T __init, _BinaryOperation __reduce_op)
    {
      typedef typename iterator_traits<_ForwardIt>::value_type
	_SrcValueType;

      using _TransformInvocationType =
	std::experimental::raw_invocation_type<
	  _BinaryOperation(_SrcValueType, _SrcValueType)>;

      typedef typename _TransformInvocationType::ret_type _ResultType;

      size_t __data_size = std::distance(__first, __last);

      if (__data_size == 0) {
	return __init;
      } else if (__data_size == 1) {
	return __reduce_op(*__first, __init);
      } else if (__data_size == 2) {
	return __reduce_op(__reduce_op(*__first, *(__first + 1)), __init);
      }
      /* This implementation is not memory optimized, but aims to launch
	 as simple kernels as possible for improved performance portability.
	 It relies on HW-based cache coherence logic for data locality and
	 assumes kernel launches are cheap.

	 TO OPTIMIZE:
	 - utilize local (scratchpad) memory, if any, by computing the
	   reduce inside the local memory like the transform_reduce impl. does
      */

      std::vector<_ResultType> __reduce_results(__data_size / 2);
      _T __result = __init;

      /* Delegate to transform by repeatedly splitting the data to two parts
	 and executing the reduction to pairs formed from the first and the
	 last parts. TO OPTIMIZE: this has awful data locality if used in
	 combination with transform.  */
      const _ResultType* __chunk1 = &*(__first);
      const _ResultType* __chunk1_last = __chunk1 + __data_size / 2;
      const _ResultType* __chunk2 = __chunk1_last;
      size_t __to_reduce = __data_size;

      constexpr size_t _OFFLOAD_THRESHOLD = 1000;

      // Offload only N reduction chunks.
      while (__to_reduce >= _OFFLOAD_THRESHOLD) {

	if (__to_reduce % 2 == 1) {
	  // Ensure the size of the container is divisible by two.
	  __result = __reduce_op(*(__chunk1 + __to_reduce - 1), __result);
	  --__to_reduce;
	}

	std::experimental::execution::transform(
  	  __policy, __chunk1, __chunk1_last, __chunk2,
	  &__reduce_results[0], __reduce_op);

	__to_reduce /= 2;
	__chunk1 = &__reduce_results[0];
	__chunk1_last = __chunk1 + __to_reduce / 2;
	__chunk2 = __chunk1_last;
      }

      // Perform the leftover reductions serially.
      while (__to_reduce > 0) {
	__result = __reduce_op(*__chunk1, __result);
	++__chunk1;
	--__to_reduce;
      }
      return __result;
    }
} // namespace execution
} // namespace experimental
  using std::experimental::execution::reduce;
} // namespace std

#endif
