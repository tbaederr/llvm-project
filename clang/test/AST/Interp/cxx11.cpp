// RUN: %clang_cc1 -fexperimental-new-constant-interpreter -verify=both,expected -std=c++11 %s
// RUN: %clang_cc1 -verify=both,ref -std=c++11 %s

// both-no-diagnostics

namespace IntOrEnum {
  const int k = 0;
  const int &p = k;
  template<int n> struct S {};
  S<p> s;
}
