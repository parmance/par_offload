// Offloading 'max_element' algorithm -*- C++ -*-

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

/** @file experimental/bits/par_offload_max_element.tcc
 *  This is an internal header file included by the par_offload header.
 *  Do not attempt to use it directly.
 *
 *  This header has an implementation of experimental 'par_offload'
 *  execution policy for 'max_element' on top of HSA Runtime.
 */

#ifndef _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_MAX_ELEMENT_TCC
#define _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_MAX_ELEMENT_TCC 1

#include <algorithm>
#include <vector>
#include <numeric>
#include <iostream>
#include <cassert>
#include <experimental/type_traits>
#include <iterator>
#include <numeric>
#include <experimental/bits/par_offload_helpers.tcc>
#include <experimental/bits/par_offload_placeholders.tcc>

#pragma GCC system_header

template<typename _ValueType>
struct _PartialMaxT {
  size_t __index;
  _ValueType __val;
};

/// HSA max_element kernel for random accessible containers.
template<typename _ElementType, typename _FuncRetType, typename _FuncArgType,
	 bool _NeedsThisPtr>
  static void __attribute__((hsa_kernel))
  __hsa_max_element(void* __args) {

    size_t __id = __builtin_hsa_workitemflatabsid ();

    void* __this = *((void**)__args + 0);
    _PartialMaxT<_ElementType>* __result =
      *((_PartialMaxT<_ElementType>**)__args + 1);
    __result += __id;
    _ElementType* __search_pos =  *((_ElementType**)__args + 2);
    size_t __chunk_size = **((size_t**)__args + 3);

    size_t __i = __id * __chunk_size;

    __result->__index = __i;
    __search_pos += __i;
    __result->__val = *__search_pos;
    __i++;
    __search_pos++;

    for (size_t __p = 1; __p < __chunk_size; ++__p, ++__search_pos, ++__i) {
      _ElementType __val = *__search_pos;
      bool __cmp_res =
	_NeedsThisPtr ?
	_PSTL_HSA_MAX_ELEMENT_CMP_PLACEHOLDER_M<_FuncRetType, _FuncArgType>
	(__this, __result->__val, __val) :
	_PSTL_HSA_MAX_ELEMENT_CMP_PLACEHOLDER<_FuncRetType, _FuncArgType>
	(__result->__val, __val);
      if (__cmp_res && __result->__val != __val) {
	__result->__index = __i;
	__result->__val = __val;
      }
    }
  }

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

namespace experimental
{
namespace execution
{
  /// Offloading 'max_element' implementation using HSA.
  template<
    class _ExecutionPolicy,
    class _InputIt,
    class _Comparator,
    class _Enabler = typename std::enable_if<
      __par_offload_ok<_ExecutionPolicy, _InputIt>>::type
    >
    _InputIt
    max_element(_ExecutionPolicy&& __policy, _InputIt __first,
		_InputIt __last, _Comparator __cmp) {

#ifdef _TESTING_PAR_OFFLOAD
      __pstl_was_offloaded = false;
#endif

      size_t __data_size = std::distance(__first, __last);

      if (__data_size == 0)
	return __last;

      typedef typename iterator_traits<_InputIt>::value_type _SrcValueType;

      typedef typename std::add_lvalue_reference<_SrcValueType>::type
	_CallArgType;

      using _InvocationType =
	std::experimental::raw_invocation_type<
	  _Comparator(_CallArgType, _CallArgType)>;
      typedef typename _InvocationType::template arg<0>::type _UserFuncArgType;
      typedef typename _InvocationType::ret_type _UserFuncRetType;

      void* __host_function_address;
      void* __device_function_address;

      static_assert (sizeof(void*) == 8);

      __host_function_address =
	__to_c_function_addr <decltype (__cmp), _UserFuncRetType,
			      _UserFuncArgType, _UserFuncArgType>(__cmp);

      __device_function_address =
	__builtin_GOMP_host_to_device_fptr (0, __host_function_address);

      if ((uint64_t)__device_function_address == UINT64_MAX)
	return std::max_element(__first, __last, __cmp);

      constexpr bool __needs_this_ptr =
	__call_needs_this_ptr<
	  decltype (__cmp), _UserFuncRetType, _UserFuncArgType,
	  _UserFuncArgType>();

      constexpr size_t __max_parallel_reduces = 1024;
      const size_t __parallel_reduces =
	std::min(__data_size, __max_parallel_reduces);
      const size_t __data_per_wi = __data_size / __parallel_reduces;
      const size_t __offloaded_data = __data_per_wi * __parallel_reduces;
      std::vector<_PartialMaxT<_SrcValueType> >
	__partial_results(__parallel_reduces);

      int __status = 0;
      void* __arg_data[4];
      __arg_data[0] = __needs_this_ptr ? &__cmp : nullptr;
      __arg_data[1] = &__partial_results[0];
      __arg_data[2] = (void*)&(*__first);
      __arg_data[3] = (void*)(&__data_per_wi);
      __status =
	__builtin_GOMP_direct_invoke_spmd_kernel
	(0, (void*)__hsa_max_element<
	 _SrcValueType, _UserFuncRetType, _UserFuncArgType, __needs_this_ptr>,
	 __parallel_reduces, 0, 0, 0, 0, 0, &__arg_data,
	 __needs_this_ptr ?
	 (void*) _PSTL_HSA_MAX_ELEMENT_CMP_PLACEHOLDER_M <
	 _UserFuncRetType, _UserFuncArgType> :
	 (void*) _PSTL_HSA_MAX_ELEMENT_CMP_PLACEHOLDER <
	 _UserFuncRetType, _UserFuncArgType>,
	 __device_function_address, NULL, NULL, NULL, NULL, NULL, NULL);

      if (__status == 1)
	return std::max_element(__first, __last, __cmp);

      const size_t __leftover_data =
	__data_size - __offloaded_data;

      _PartialMaxT<_SrcValueType> __best_so_far;
      if (__leftover_data > 0) {
	const size_t __first_leftover = __data_per_wi * __parallel_reduces;
	__best_so_far.__index = __first_leftover;
	__best_so_far.__val = *(__first + __first_leftover);

	_InputIt __it = __first + __first_leftover + 1;
	for (size_t __i = 1; __i < __leftover_data; ++__i, ++__it) {
	  if (__cmp(__best_so_far.__val, *__it) &&
	      __best_so_far.__val != *__it) {
	    __best_so_far.__index = __first_leftover + __i;
	    __best_so_far.__val = *__it;
	  }
	}
      } else {
	__best_so_far.__index = 0;
	__best_so_far.__val = __partial_results[0].__val;
      }

      auto __reducer =
	[&__cmp](const _PartialMaxT<_SrcValueType>& __a,
		 const _PartialMaxT<_SrcValueType>& __b) {
	if (__cmp(__b.__val, __a.__val) ||
	    (__a.__val == __b.__val && __a.__index < __b.__index))
	  return __a;
	else
	  return __b;
      };

      const _PartialMaxT<_SrcValueType> __the_best =
	std::reduce(__policy, &__partial_results[0],
		    &__partial_results[0] + __parallel_reduces,
		    __best_so_far, __reducer);
#ifdef _TESTING_PAR_OFFLOAD
      __pstl_was_offloaded = true;
#endif
      return __first + __the_best.__index;
    }

  template<
    class _ExecutionPolicy,
    class _InputIt,
    class _Enabler = typename std::enable_if<
      __par_offload_ok<_ExecutionPolicy, _InputIt>>::type
    >
    _InputIt
      max_element(_ExecutionPolicy&& __policy, _InputIt __first,
		  _InputIt __last) {
         typedef typename iterator_traits<_InputIt>::value_type _ValueType;
         return max_element(__policy, __first, __last, std::less<_ValueType>());
      }
} // namespace execution
} // namespace experimental
  using std::experimental::execution::max_element;
} // namespace std

#endif
