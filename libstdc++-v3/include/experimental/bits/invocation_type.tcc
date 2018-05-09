// Experimental and incomplete invocation type traits impl. -*- C++ -*-
//
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

/** @file experimental/bits/invocation_type.tcc
 *  This is an internal header file included by the type_traits header.
 *  Do not attempt to use it directly.
 *
 *  This header has a preliminary experimental implementation of
 *  N3866 Invocation type traits (Rev. 2) as described in
 *  http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3866.html and
 *  http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4529.html
 *  #meta.trans.other
 *
 *  This is an incomplete implementation that does not support all callable
 *  cases. A proper implementation requires frontend support to access the
 *  compile time function overload resolution logic.
 */

//
// N3866 Invocation type traits
//

#ifndef _GLIBCXX_EXPERIMENTAL_INVOCATION_TYPE_TCC
#define _GLIBCXX_EXPERIMENTAL_INVOCATION_TYPE_TCC 1

#include <type_traits>
#include <bits/stl_function.h>
#include <bits/invoke.h>
#include <bits/refwrap.h>
#include <bits/functexcept.h>

#pragma GCC system_header

#if __cplusplus >= 201402L

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

namespace experimental
{
inline namespace fundamentals_v2
{

template<class _Callable>
struct _Callable_traits;

  /*  A plain C function pointer.  */
template<class _Callable, class... _Args>
struct _Callable_traits<_Callable(*)(_Args...)> :
    public _Callable_traits<_Callable(_Args...)>
{
  typedef _Callable _func_type(_Args...);
};

template<class _Callable, class... _Args>
struct _Callable_traits<_Callable(_Args...)>
{
  using _return_type = _Callable;
  typedef _Callable _func_type(_Args...);

  static constexpr std::size_t _arg_count = sizeof...(_Args);

  template <class _Head, class... _Tail>
  struct _tail_tuple
  {
    typedef std::tuple<_Tail...> _type;
  };

  typedef std::tuple<_Args...> _real_args;
  typedef typename _tail_tuple<_Args...>::_type _real_args_tail;
};

  /* A member function pointer.  */
template<class _Class, class _Callable, class... _Args>
struct _Callable_traits<_Callable(_Class::*)(_Args...)> :
    public _Callable_traits<_Callable(_Class&, _Args...)>
{
  typedef _Callable (_Class::*_func_type)(_Args...);
};

  /* Const member function pointer.  */
template<class _Class, class _Callable, class... _Args>
struct _Callable_traits<_Callable(_Class::*)(_Args...) const> :
    public _Callable_traits<_Callable(_Class&, _Args...)>
{
  typedef _Callable (_Class::*_func_type)(_Args...) const;
  //typedef _Callable _func_type(_Args...) const;
};

  /* Volatile member function pointer.  */

template<class _Class, class _Callable, class... _Args>
struct _Callable_traits<_Callable(_Class::*)(_Args...) volatile> :
    public _Callable_traits<_Callable(_Class&, _Args...)>
{
  typedef _Callable (_Class::*_func_type)(_Args...) volatile;
};

  /* Volatile const member function pointer.  */

template<class _Class, class _Callable, class... _Args>
struct _Callable_traits<_Callable(_Class::*)(_Args...) const volatile> :
    public _Callable_traits<_Callable(_Class&, _Args...)>
{
  typedef _Callable (_Class::*_func_type)(_Args...) const volatile;
};

  /* Member object pointer.  */

template<class _Class, class _Callable>
struct _Callable_traits<_Callable(_Class::*)> :
    public _Callable_traits<_Callable(_Class&)>
{
  using typename _Callable_traits<_Callable(_Class&)>::_func_type;
};

/* Functors.  */
template<class _Callable>
struct _Callable_traits
{
private:
  using _call_type = _Callable_traits<decltype(&_Callable::operator())>;
public:
  using _return_type = typename _call_type::_return_type;
  using _func_type = typename _call_type::_func_type;
  // In member functors, the first arg is the object type (the lambda itself).
  using _real_args = typename _call_type::_real_args_tail;

  static constexpr std::size_t _arg_count = _call_type::_arg_count - 1;
};

template<class _Callable>
struct _Callable_traits<_Callable&> :
    public _Callable_traits<_Callable> { };

template<class _Callable>
struct _Callable_traits<_Callable&&> :
    public _Callable_traits<_Callable> { };

template <typename _CalledSignature>
  struct raw_invocation_type;

/**
 *  Prototype raw_invocation_type for callables.
 *
 *  _Callable is something which can be called with given argument types.
 *  That is, the argument can be implicitly converted to the given argument
 *  type, which is the type of the argument of the _Callable's actually called
 *  function.
 *
 *  In a successful instantiation, it exposes the called function's type
 *  in 'type'.  It also exposes 'arg<N>::type' and 'ret_type' which are not
 *  specified in the proposal.
 */
template <class _Callable, class ..._SrcArgT>
struct raw_invocation_type<_Callable(_SrcArgT...)>
{
private:
  using _Traits = _Callable_traits<_Callable>;
public:

  template <std::size_t _ArgN>
  struct arg
  {
    using type =
      typename std::tuple_element<_ArgN, typename _Traits::_real_args>::type;
  };

  typedef typename _Traits::_return_type ret_type;

  typedef typename std::enable_if_t<
    std::is_convertible<std::tuple<_SrcArgT...>,
			typename _Traits::_real_args>::value,
    typename _Traits::_func_type> type;
};

} // namespace fundamentals_v2
} // namespace experimental

 _GLIBCXX_END_NAMESPACE_VERSION
} // namespace std

#endif // __cplusplus <= 201103L

#endif // _GLIBCXX_EXPERIMENTAL_INVOCATION_TYPE_TCC
