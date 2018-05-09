// Offloading 'transform' algorithm -*- C++ -*-

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

/** @file experimental/bits/par_offload_transform.tcc
 *  This is an internal header file included by the par_offload header.
 *  Do not attempt to use it directly.
 *
 *  This header has an implementation of experimental 'par_offload'
 *  execution policy for the 'transform' on top of HSA Runtime.
 */

#ifndef _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_TRANSFORM_TCC
#define _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_TRANSFORM_TCC 1

#include <algorithm>
#include <experimental/type_traits>
#include <iterator>
#include <experimental/bits/par_offload_helpers.tcc>
#include <experimental/bits/par_offload_placeholders.tcc>

#pragma GCC system_header

/// HSA transform kernels for random accessible containers.
template<typename _ElementType, typename _ResultType, typename _FuncRetType,
	 typename _FuncArgType, bool _NeedsThisPtr>
  static void __attribute__((hsa_kernel))
  __hsa_transform_array (const void *__args)
  {
    unsigned __i = __builtin_hsa_workitemabsid (0);

    void* const __this = *((void* const *)__args + 0);
    _ResultType * const __d_first = *((_ResultType* const *)__args + 1);
    _ElementType * const __first1 = *((_ElementType* const  *)__args + 2);

    __d_first[__i] =
      _NeedsThisPtr ?
      _PSTL_HSA_TRANSFORM_FUNCTOR_PLACEHOLDER_M<_FuncRetType, _FuncArgType>
      (__this, __first1[__i]) :
      _PSTL_HSA_TRANSFORM_FUNCTOR_PLACEHOLDER<_FuncRetType, _FuncArgType>
      (__first1[__i]);
  }

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

namespace experimental
{
namespace execution
{
  /// Offloading 'transform' implementation using HSA.
  template<
    class _ExecutionPolicy,
    class _ForwardIt1,
    class _ForwardIt2,
    class _UnaryOperation,
    class _Enabler = typename std::enable_if<
      __par_offload_ok<_ExecutionPolicy, _ForwardIt1, _ForwardIt2>>::type
    >
    _ForwardIt2
    transform(_ExecutionPolicy&& __policy,
	      _ForwardIt1 __first1, _ForwardIt1 __last1,
	      _ForwardIt2 __d_first, _UnaryOperation __unary_op)
    {
      size_t __grid_x_size = std::distance (__first1, __last1);

      if (__grid_x_size > UINT32_MAX)
	return std::transform(__first1, __last1, __d_first, __unary_op);

      typedef typename iterator_traits<_ForwardIt1>::value_type _SrcValueType;
      typedef typename iterator_traits<_ForwardIt2>::value_type _DstValueType;

      using _InvocationType =
	std::experimental::raw_invocation_type<
	  _UnaryOperation(
	    typename std::add_lvalue_reference<_SrcValueType>::type)>;
      typedef typename _InvocationType::template arg<0>::type _UserFuncArgType;
      typedef typename _InvocationType::ret_type _UserFuncRetType;

      void* __host_function_address;
      void* __device_function_address;

      static_assert (sizeof(void*) == 8);

      __host_function_address =
	__to_c_function_addr <decltype (__unary_op), _UserFuncRetType,
			      _UserFuncArgType>(__unary_op);

      __device_function_address =
	__builtin_GOMP_host_to_device_fptr (0, __host_function_address);

#ifdef _TESTING_PAR_OFFLOAD
      __pstl_was_offloaded = false;
#endif
      // In case there was no device version of the UnaryFunction
      // available, fallback to host execution.
      if ((uint64_t)__device_function_address == UINT64_MAX)
	return std::transform(__first1, __last1, __d_first, __unary_op);

      constexpr bool __needs_this_ptr =
	__call_needs_this_ptr<
	  decltype (__unary_op), _UserFuncRetType, _UserFuncArgType>();

      int __status = 0;
      void* __arg_data[3];
      __arg_data[0] = __needs_this_ptr ? (void*)&__unary_op : nullptr;
      __arg_data[1] = (void*)&(*__d_first);
      __arg_data[2] = (void*)&(*__first1);

      __status =
	__builtin_GOMP_direct_invoke_spmd_kernel
	(0, (void*)__hsa_transform_array <
	 _SrcValueType, _DstValueType,_UserFuncRetType,
	 _UserFuncArgType, __needs_this_ptr>,
	 __grid_x_size, 0, 0, 0, 0, 0, &__arg_data,
	 __needs_this_ptr ?
	 (void*) _PSTL_HSA_TRANSFORM_FUNCTOR_PLACEHOLDER_M <
	 _UserFuncRetType, _UserFuncArgType> :
	 (void*) _PSTL_HSA_TRANSFORM_FUNCTOR_PLACEHOLDER <
	 _UserFuncRetType, _UserFuncArgType>,
	 __device_function_address, NULL, NULL, NULL, NULL, NULL, NULL);

      if (__status == 1)
	return std::transform(__first1, __last1, __d_first, __unary_op);

#ifdef _TESTING_PAR_OFFLOAD
      __pstl_was_offloaded = true;
#endif
      return __d_first + __grid_x_size;
    }
} // namespace execution
} // namespace experimental
  using std::experimental::execution::transform;
} // namespace std

#endif
