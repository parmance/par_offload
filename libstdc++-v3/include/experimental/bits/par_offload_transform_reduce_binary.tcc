// Offloading 'transform_reduce' (binary_op) algorithm -*- C++ -*-

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

/** @file experimental/bits/par_offload_transform_reduce_binary.tcc
 *  This is an internal header file included by the par_offload header.
 *  Do not attempt to use it directly.
 *
 *  This header has an implementation of experimental 'par_offload'
 *  execution policy for the 'transform_reduce' on top of HSA Runtime.
 */

#ifndef _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_TRANSFORM_REDUCE_BINARY_TCC
#define _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_TRANSFORM_REDUCE_BINARY_TCC 1

#include <algorithm>
#include <vector>
#include <iostream>
#include <numeric>
#include <iterator>
#include <memory>
#include <experimental/type_traits>
#include <experimental/bits/par_offload_helpers.tcc>
#include <experimental/bits/par_offload_placeholders.tcc>

#pragma GCC system_header

/// HSA kernel for array containers.
template<typename _Element1Type, typename _Element2Type,
	 typename _ResultType, typename _TrFuncRetType,
	 typename _TrFuncArg1Type, typename _TrFuncArg2Type,
	 typename _RedFuncRetType, typename _RedFuncArg1Type,
	 typename _RedFuncArg2Type, int _NPairs, int _WGSize,
	 bool _TrNeedsThisPtr, bool _RedNeedsThisPtr>
  static void __attribute__((hsa_kernel))
  __hsa_transform_reduce_binary_array(void *__args)
  {
    uint32_t __i = __builtin_hsa_workitemabsid (0);

    void* __this_tr = *((void**)__args + 0);
    void* __this_red = *((void**)__args + 1);

    _ResultType* __result = *((_ResultType**)__args + 2);
    _Element1Type* __first1 = *((_Element1Type**)__args + 3);
    _Element2Type* __first2 = *((_Element2Type**)__args + 4);

    uint32_t __transform_i = __i * _NPairs;
    _TrFuncRetType __tr_results[_NPairs];

    for (uint32_t __ti = 0; __ti < _NPairs; ++__ti, ++__transform_i)
      __tr_results[__ti] =
	_TrNeedsThisPtr ?
	_PSTL_HSA_TR_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER_M <_TrFuncRetType,
							  _TrFuncArg1Type,
							  _TrFuncArg2Type>
	(__this_tr, __first1[__transform_i], __first2[__transform_i]) :
	_PSTL_HSA_TR_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER <_TrFuncRetType,
							_TrFuncArg1Type,
							_TrFuncArg2Type>
	(__first1[__transform_i], __first2[__transform_i]);

    _RedFuncRetType __red_results[_NPairs / 2];

    for (uint32_t __ri = 0; __ri < _NPairs / 2; ++__ri)
      __red_results[__ri] =
	_RedNeedsThisPtr ?
	_PSTL_HSA_TR_REDUCE_FUNCTOR_PLACEHOLDER_M <_RedFuncRetType,
						   _RedFuncArg1Type,
						   _RedFuncArg2Type>
	(__this_red, __tr_results[__ri], __tr_results[__ri + _NPairs / 2]) :
	_PSTL_HSA_TR_REDUCE_FUNCTOR_PLACEHOLDER <_RedFuncRetType,
						 _RedFuncArg1Type,
						 _RedFuncArg2Type>
	(__tr_results[__ri], __tr_results[__ri + _NPairs / 2]);

    for (uint32_t __ri1 = _NPairs / 2; __ri1 > 1; __ri1 /= 2)
      for (uint32_t __ri2 = 0, __split = __ri1 / 2; __ri2 < __split;
	   ++__ri2)
	__red_results[__ri2] =
	  _RedNeedsThisPtr ?
	  _PSTL_HSA_TR_REDUCE_FUNCTOR_PLACEHOLDER_M <_RedFuncRetType,
						     _RedFuncArg1Type,
						     _RedFuncArg2Type>
	  (__this_red, __red_results[__ri2], __red_results[__ri2 + __split]) :
	  _PSTL_HSA_TR_REDUCE_FUNCTOR_PLACEHOLDER <_RedFuncRetType,
						   _RedFuncArg1Type,
						   _RedFuncArg2Type>
	  (__red_results[__ri2], __red_results[__ri2 + __split]);


    static _RedFuncRetType __attribute__((hsa_group_segment))
      __shared_red_results[_WGSize];

    uint32_t __lid = __builtin_hsa_workitemid(0);

    __shared_red_results[__lid] = __red_results[0];
    __builtin_hsa_barrier_all();

    for (uint32_t __split = _WGSize / 2; __split > 0; __split /= 2) {
      if (__lid < __split) {
	__shared_red_results[__lid] =
	  _RedNeedsThisPtr ?
	  _PSTL_HSA_TR_REDUCE_FUNCTOR_PLACEHOLDER_M <_RedFuncRetType,
						     _RedFuncArg1Type,
						     _RedFuncArg2Type>
	(__this_red, __shared_red_results[__lid],
	 __shared_red_results[__lid + __split]) :
	  _PSTL_HSA_TR_REDUCE_FUNCTOR_PLACEHOLDER <_RedFuncRetType,
						   _RedFuncArg1Type,
						   _RedFuncArg2Type>
	(__shared_red_results[__lid], __shared_red_results[__lid + __split]);
      }
      __builtin_hsa_barrier_all();
    }

    if (__lid == 0)
      __result[__builtin_hsa_workgroupid(0)] = __shared_red_results[0];
  }


namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

namespace experimental
{
namespace execution
{
  /// Serial fallback implementation.
  template<
    class _ForwardIt1,
    class _ForwardIt2,
    class _T,
    class _BinaryOperation1,
    class _BinaryOperation2
    >
    _T
    transform_reduce(_ForwardIt1 __first1, _ForwardIt1 __last1,
		     _ForwardIt2 __first2, _T __init,
		     _BinaryOperation1 __reduce_op,
		     _BinaryOperation2 __transform_op)
    {
      size_t __data_size = std::distance(__first1, __last1);
      std::vector<_T> __transform_results(__data_size);
      std::transform(__first1, __last1, __first2, __transform_results.begin(),
		     __transform_op);

      _T __result = __init;
      for (_T __d : __transform_results)
	__result = __reduce_op (__d, __result);

      return __result;
    }

  /// Offloading 'transform_reduce' binary op implementation using HSA.
  template<
    class _ExecutionPolicy,
    class _ForwardIt1,
    class _ForwardIt2,
    class _T,
    class _BinaryOperation1,
    class _BinaryOperation2,
    class _Enabler = typename std::enable_if<
      __par_offload_ok<_ExecutionPolicy, _ForwardIt1, _ForwardIt2>>::type
    >
    _T
    transform_reduce(_ExecutionPolicy&& __policy,
		     _ForwardIt1 __first1, _ForwardIt1 __last1,
		     _ForwardIt2 __first2, _T __init,
		     _BinaryOperation1 __reduce_op,
		     _BinaryOperation2 __transform_op)
    {
      typedef typename iterator_traits<_ForwardIt1>::value_type _Src1ValueType;
      typedef typename iterator_traits<_ForwardIt2>::value_type _Src2ValueType;

      using _TransformInvocationType =
	std::experimental::raw_invocation_type<
	  _BinaryOperation2(_Src1ValueType, _Src2ValueType)>;
      typedef typename _TransformInvocationType::template arg<0>::type
       _TransformArg1Type;
      typedef typename _TransformInvocationType::template arg<1>::type
       _TransformArg2Type;
      typedef typename _TransformInvocationType::ret_type _TransformRetType;

      using _ReduceInvocationType =
	std::experimental::raw_invocation_type<
	  _BinaryOperation1(_Src1ValueType, _Src2ValueType)>;
      typedef typename _ReduceInvocationType::template arg<0>::type
       _ReduceArg1Type;
      typedef typename _ReduceInvocationType::template arg<1>::type
       _ReduceArg2Type;
      typedef typename _ReduceInvocationType::ret_type _ReduceRetType;

      size_t __data_size = std::distance(__first1, __last1);
      constexpr int __pairs_per_wi = 8;
      constexpr int __wg_size = 1024;

      if ((__data_size / __pairs_per_wi) / __wg_size < 16)
	return transform_reduce(__first1, __last1, __first2,
				__init, __reduce_op, __transform_op);

#ifdef _TESTING_PAR_OFFLOAD
      __pstl_was_offloaded = false;
#endif

       void* __tr_host_function_address;
       void* __tr_device_function_address;

       static_assert (sizeof(void*) == 8);

       __tr_host_function_address =
	 __to_c_function_addr <decltype (__transform_op),
			       _TransformRetType,
			       _TransformArg1Type,
			       _TransformArg2Type>(__transform_op);

       __tr_device_function_address =
	 __builtin_GOMP_host_to_device_fptr (0, __tr_host_function_address);

       void* __red_host_function_address;
       void* __red_device_function_address;

       static_assert (sizeof(void*) == 8);

       __red_host_function_address =
	 __to_c_function_addr <decltype (__reduce_op),
			       _ReduceRetType,
			       _ReduceArg1Type,
			       _ReduceArg2Type>(__reduce_op);

       __red_device_function_address =
	 __builtin_GOMP_host_to_device_fptr (0, __red_host_function_address);

       if ((uint64_t)__tr_device_function_address == UINT64_MAX ||
	   (uint64_t)__red_device_function_address == UINT64_MAX)
	 return transform_reduce(__first1, __last1, __first2,
				 __init, __reduce_op, __transform_op);

       constexpr bool __tr_needs_this_ptr =
	 __call_needs_this_ptr<
	    decltype (__transform_op), _TransformRetType,
	    _TransformArg1Type, _TransformArg2Type>();

       constexpr bool __red_needs_this_ptr =
	 __call_needs_this_ptr<
	    decltype (__reduce_op), _ReduceRetType, _ReduceArg1Type,
	    _ReduceArg2Type>();

       size_t __grid_x_size =
	 ((__data_size / __pairs_per_wi) / __wg_size) * __wg_size;
       size_t __leftover = __data_size - (__grid_x_size * __pairs_per_wi);

       // Store the intermediate reduce results to a temporary array.  Leave
       // room for leftover data in case the size is not divisible by the
       // number of pairs we process per WI or the WG size.
       size_t __result_size = __grid_x_size / __wg_size + __leftover;
#ifdef HSA_BASE_PROFILE
       std::unique_ptr<_T[], std::function<void(_T*)>> __result
	 ((_T*)hsa_alloc_shared(__result_size * sizeof(_T)),
	  [](_T* ptr) { hsa_memory_free(ptr); });
#else
       std::unique_ptr<_T[]> __result(new _T[__result_size]);
#endif
       int __status = 1;
       void* __arg_data[5];
       __arg_data[0] = __tr_needs_this_ptr ? &__transform_op : nullptr;
       __arg_data[1] = __red_needs_this_ptr ? &__reduce_op : nullptr;
       __arg_data[2] = __result.get();
       __arg_data[3] = const_cast<_Src1ValueType*>(&(*__first1));
       __arg_data[4] = const_cast<_Src2ValueType*>(&(*__first2));
       __status =
	    __builtin_GOMP_direct_invoke_spmd_kernel
	    (0, (void*)__hsa_transform_reduce_binary_array
	     <_Src1ValueType, _Src2ValueType, _T,
	      _TransformRetType,_TransformArg1Type,
	      _TransformArg2Type, _ReduceRetType,
	      _ReduceArg1Type, _ReduceArg2Type, __pairs_per_wi,
	     __wg_size, __tr_needs_this_ptr, __red_needs_this_ptr>,
	     __grid_x_size, __wg_size, 0, 0, 0, 0, &__arg_data,
	     __tr_needs_this_ptr ?
	     (void*) _PSTL_HSA_TR_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER_M
	     <_TransformRetType, _TransformArg1Type, _TransformArg2Type> :
	     (void*) _PSTL_HSA_TR_TRANSFORM_BIN_FUNCTOR_PLACEHOLDER
	     <_TransformRetType, _TransformArg1Type, _TransformArg2Type>,
	     __tr_device_function_address,
	     __red_needs_this_ptr ?
	     (void*) _PSTL_HSA_TR_REDUCE_FUNCTOR_PLACEHOLDER_M
	     <_ReduceRetType, _ReduceArg1Type, _ReduceArg2Type> :
	     (void*) _PSTL_HSA_TR_REDUCE_FUNCTOR_PLACEHOLDER
	     <_ReduceRetType, _ReduceArg1Type, _ReduceArg2Type>,
	     __red_device_function_address,
	     NULL, NULL, NULL, NULL);

       if (__status == 1)
	 return transform_reduce(__first1, __last1, __first2,
				 __init, __reduce_op, __transform_op);

       if (__leftover > 0)
	 std::transform(__first1 + __data_size - __leftover, __last1,
			__first2 + __data_size - __leftover,
			__result.get() + __result_size - __leftover,
			__transform_op);

#ifdef _TESTING_PAR_OFFLOAD
       __pstl_was_offloaded = true;
#endif
       _T __final_result = __init;
       int counter = 0;
       for (size_t __i = 0; __i < __result_size; ++__i)
	 __final_result = __reduce_op (__result[__i], __final_result);
       return __final_result;
    }
} // namespace execution
} // namespace experimental
  using std::experimental::execution::transform_reduce;
} // namespace std

#endif
