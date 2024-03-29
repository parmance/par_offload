// { dg-do compile  { target c++11 } }
// PR 81050 ICE in invalid after error

template<typename... T> struct A
{
  static const int i;
};

template<typename... T>
const int A<T>::i // { dg-error "template definition of non-template" }
= []{ return 0; }(); // BOOM!
