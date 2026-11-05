// RUN: %clang_cc1 -fsyntax-only -verify -Wc++11-compat %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

namespace N {
  template<class T> class Y { // expected-note{{explicit instantiation refers here}}
    void mf() { } 
  };
}

template class Z<int>; // expected-error{{explicit instantiation of undeclared template class 'Z'}}

struct Q;
template class Q<int>; // expected-error{{explicit instantiation of non-template class 'Q'}}

// FIXME: This example from the standard is wrong; note posted to CWG reflector
// on 10/27/2009
using N::Y; 
template class Y<int>;
// expected-error@-1 {{explicit instantiation of 'N::Y' must occur in namespace 'N'}}

template class N::Y<char*>; 
template void N::Y<double>::mf();
