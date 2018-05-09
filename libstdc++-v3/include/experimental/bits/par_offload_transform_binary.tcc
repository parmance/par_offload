// Offloading 'transform' (binary_op) algorithm -*- C++ -*-

// Copyright (C) 2018 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file experimental/bits/par_offload_transform_binary.tcc
 *  This is an internal header file included by the par_offload header.
 *  Do not attempt to use it directly.
 *
 *  This header has an implementation of experimental 'par_offload'
 *  execution policy for the 'transform' on top of HSA Runtime.
 */

#ifndef _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_TRANSFORM_BINARY_TCC
#define _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_TRANSFORM_BINARY_TCC 1

#include <algorithm>
#include <iostream>
#include <iterator>
#include <experimental/type_traits>
#include <experimental/bits/par_offload_helpers.tcc>
#include <experimental/bits/par_offload_placeholders.tcc>

#pragma GCC system_header

/// HSA kernel for array containers.
template<typename _Element1Type, typename _Element2Type, typename _ResultType,
	 typename _FuncRetType, typename _FuncArg1Type, typename _FuncArg2Type,
	 bool _NeedsThisPtr>
  static void __attribute__((hsa_kernel))
  __hsa_transform_binary_array(void *__args) {
    uint32_t __i = __builtin_hsa_workitemabsid (0);

    void* __this = *((void**)__args + 0);
    _ResultType* __result = *((_ResultType**)__args + 1);
    _Element1Type* __first1 = *((_Element1Type**)__args + 2);
    _Element2Type* __first2 = *((_Element2Type**)__args + 3);

    __result[__i] =
      _NeedsThisPtr ?
      _PSTL_HSA_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER_M <_FuncRetType,
						     _FuncArg1Type,
						     _FuncArg2Type>
      (__this, __first1[__i], __first2[__i]) :
      _PSTL_HSA_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER <_FuncRetType,
						   _FuncArg1Type,
						   _FuncArg2Type>
      (__first1[__i], __first2[__i]);
  }

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

namespace experimental
{
namespace execution
{
  /// Offloading 'transform' binary op implementation using HSA.
  template<
    class _ExecutionPolicy,
    class _ForwardIt1,
    class _ForwardIt2,
    class _ForwardIt,
    class _BinaryOperation,
    class _Enabler = typename std::enable_if<
      __par_offload_ok<_ExecutionPolicy, _ForwardIt>>::type
    >
    _ForwardIt
    transform(_ExecutionPolicy&& __policy,
	      _ForwardIt1 __first1, _ForwardIt1 __last1,
	      _ForwardIt2 __first2, _ForwardIt __result,
	      _BinaryOperation __binary_op) {

       size_t __grid_x_size = std::distance (__first1, __last1);
       typedef typename iterator_traits<_ForwardIt1>::value_type
	 _Src1ValueType;
       typedef typename iterator_traits<_ForwardIt2>::value_type
	 _Src2ValueType;
       typedef typename iterator_traits<_ForwardIt>::value_type
	 _DstValueType;

       using _InvocationType =
	 std::experimental::raw_invocation_type<
	   _BinaryOperation(_Src1ValueType, _Src2ValueType)>;
       typedef typename _InvocationType::template arg<0>::type
       _UserFuncArg1Type;
       typedef typename _InvocationType::template arg<1>::type
       _UserFuncArg2Type;
       typedef typename _InvocationType::ret_type _UserFuncRetType;

       void* __host_function_address;
       void* __device_function_address;

       static_assert (sizeof(void*) == 8);

       __host_function_address =
	 __to_c_function_addr <decltype (__binary_op),
			       _UserFuncRetType,
			       _UserFuncArg1Type,
			       _UserFuncArg2Type>(__binary_op);

       __device_function_address =
	 __builtin_GOMP_host_to_device_fptr (0, __host_function_address);

#ifdef _TESTING_PAR_OFFLOAD
       __pstl_was_offloaded = false;
#endif
       // In case there was no device version of the UnaryFunction
       // available, fallback to host execution.
       if ((uint64_t)__device_function_address == UINT64_MAX)
	 return std::transform(__first1, __last1, __first2, __result,
			       __binary_op);

       constexpr bool __needs_this_ptr =
	 __call_needs_this_ptr<
	    decltype (__binary_op), _UserFuncRetType, _UserFuncArg1Type,
	    _UserFuncArg2Type>();

       int __status = 0;
       void* __arg_data[4];
       __arg_data[0] = __needs_this_ptr ? &__binary_op : nullptr;
       __arg_data[1] = &(*__result);
       __arg_data[2] = const_cast<_Src1ValueType*>(&(*__first1));
       __arg_data[3] = const_cast<_Src2ValueType*>(&(*__first2));
       __status =
	 __builtin_GOMP_direct_invoke_spmd_kernel
	 (0, (void*)__hsa_transform_binary_array<
	  _Src1ValueType, _Src2ValueType, _DstValueType, _UserFuncRetType,
	  _UserFuncArg1Type, _UserFuncArg2Type, __needs_this_ptr>,
	  __grid_x_size, 0, 0, 0, 0, 0, &__arg_data,
	  __needs_this_ptr ?
	  (void*) _PSTL_HSA_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER_M<
	  _UserFuncRetType, _UserFuncArg1Type, _UserFuncArg2Type> :
	  (void*) _PSTL_HSA_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER<
	  _UserFuncRetType, _UserFuncArg1Type, _UserFuncArg2Type>,
	  __device_function_address, NULL, NULL, NULL, NULL, NULL, NULL);

       if (__status == 1)
	 return std::transform(__first1, __last1, __first2, __result,
			       __binary_op);
#ifdef _TESTING_PAR_OFFLOAD
       __pstl_was_offloaded = true;
#endif
       return __result + __grid_x_size;
  }
} // namespace execution
} // namespace experimental
  using std::experimental::execution::transform;
} // namespace std

#endif
