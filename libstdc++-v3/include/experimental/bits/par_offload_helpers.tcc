// Offloading ParallelSTL algorithm implementation helpers -*- C++ -*-

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

/** @file experimental/bits/par_offload_helpers.tcc
 *  This is an internal header file included by the various par_offload
 *  implementations for helper functionality.
 *  Do not attempt to use it directly.
 */

#ifndef _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_HELPERS_TCC
#define _GLIBCXX_EXPERIMENTAL_PAR_OFFLOAD_HELPERS_TCC 1

#include <functional>

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

namespace experimental
{
namespace execution
{

  /// Check if the given T is an std::function that wraps a C function.

  template <typename _T, typename _FRetType, typename ..._FArgTypes>
    constexpr bool __is_std_function_with_c_function_v =
      std::is_same<_T, std::function<_FRetType (_FArgTypes...)>>::value;

  /// Check if the given T is function call member function operator pointer.
  template <typename _T, typename _FRetType, typename ..._FArgTypes,
	    typename _Enabler=decltype(&_T::operator())>
    constexpr bool __is_function_call_member_operator_v =
      std::is_same<decltype(&_T::operator()),
		   _FRetType (_T::*)(_FArgTypes...) const >::value;

  /// Check if the given T is a plain C function pointer.
  template <typename _T>
    constexpr bool __is_c_function_pointer_v =
      std::is_function<typename std::remove_pointer<_T>::type>::value;

  /// Checks for both the par_offload exec policy and that the iterators
  /// are valid for the offloading implementation.
  /// TODO: ContiguousIterators not yet implemented. Check for random access
  /// iterators instead, although strictly put it's too loose.

  template <typename _ExecPolicy, typename _It1, typename _It2 = int*>
  constexpr bool __par_offload_ok =
    std::is_same<typename std::decay<_ExecPolicy>::type,
		 std::experimental::execution::parallel_offload_policy>::value &&
    std::is_same<typename std::iterator_traits<_It1>::iterator_category,
		 std::random_access_iterator_tag>::value &&
    std::is_same<typename std::iterator_traits<_It2>::iterator_category,
		 std::random_access_iterator_tag>::value;


  /// Lambdas with captures, and in general, functors with operator().
  /// TODO: cv
  template <typename _FuncT, typename _UserFuncRetType,
	    typename ..._UserFuncArgTypes>
    constexpr bool
    __call_needs_this_ptr (typename std::enable_if_t<
			     __is_function_call_member_operator_v<
			       _FuncT, _UserFuncRetType, _UserFuncArgTypes...>
			   &&
			     !__is_std_function_with_c_function_v<
			       _FuncT, _UserFuncRetType, _UserFuncArgTypes...>,
			   _FuncT> *_f = nullptr)
    {
       return true;
    }

  /// Traditional C function pointers.
  template <typename _FuncT, typename _UserFuncRetType,
	    typename ..._UserFuncArgTypes>
    constexpr bool
    __call_needs_this_ptr (typename std::enable_if_t<
			     __is_c_function_pointer_v<_FuncT>,
			       _FuncT> *_f = nullptr)
    {
       return false;
    }

  /// C functions wrapped in std::function. TODO: CV qual
  template <typename _FuncT, typename _UserFuncRetType,
	    typename ..._UserFuncArgTypes>
     constexpr bool
     __call_needs_this_ptr (typename std::enable_if_t<
			      __is_std_function_with_c_function_v<
			        _FuncT, _UserFuncRetType,
			        _UserFuncArgTypes...>,
				_FuncT> *_f = nullptr)
     {
        return false;
     }

  /// Helpers to extract the underlying function pointers from various
  /// function definition types.
  template <typename _FuncT, typename _UserFuncRetType,
	    typename ..._UserFuncArgTypes>
    void *
    __to_c_function_addr (typename std::enable_if_t<
			    __is_function_call_member_operator_v<
			      _FuncT, _UserFuncRetType, _UserFuncArgTypes...>
			  &&
			    !__is_std_function_with_c_function_v<
			      _FuncT, _UserFuncRetType, _UserFuncArgTypes...>,
			  _FuncT> __f = nullptr)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpmf-conversions"
       using _InvocationType =
	 std::experimental::raw_invocation_type<_FuncT(_UserFuncArgTypes...)>;
       typedef typename _InvocationType::type _FuncType;
       _FuncType __p =
	 (_FuncType) &std::remove_reference<_FuncT>::type::operator();
       return (void*)__p;
#pragma GCC diagnostic pop
    }

  /// Traditional C function pointers.
  template <typename _FuncT, typename _UserFuncRetType,
 	    typename ..._UserFuncArgTypes>
    constexpr void *
    __to_c_function_addr (_UserFuncRetType (*__f)(_UserFuncArgTypes...))
    {
       return (void*)__f;
    }

  //  std::function wrapped functions.
  template <typename _FuncT, typename _UserFuncRetType,
	    typename ..._UserFuncArgTypes>
    constexpr void *
    __to_c_function_addr (typename std::enable_if_t<
			    std::is_same<_FuncT,
			    std::function<
                              _UserFuncRetType (_UserFuncArgTypes...)>>::value,
			  _FuncT> __f)
    {
       // The target function is stored in the beginning of the
       // std::function object. TODO: a more robust way.
       return (void*)*(uint64_t*)&__f;
    }

} // namespace execution
} // namespace experimental
} // namespace std

#endif

