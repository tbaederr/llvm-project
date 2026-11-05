// RUN: %clang_cc1 -std=c++11 -triple x86_64-unknown-unknown %s -fexceptions -fcxx-exceptions -pedantic-errors -verify-directives -verify=expected,since-cxx11
// RUN: %clang_cc1 -std=c++14 -triple x86_64-unknown-unknown %s -fexceptions -fcxx-exceptions -pedantic-errors -verify-directives -verify=expected,since-cxx11
// RUN: %clang_cc1 -std=c++17 -triple x86_64-unknown-unknown %s -fexceptions -fcxx-exceptions -pedantic-errors -verify-directives -verify=expected,since-cxx11
// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-unknown %s -fexceptions -fcxx-exceptions -pedantic-errors -verify-directives -verify=expected,since-cxx11
// RUN: %clang_cc1 -std=c++23 -triple x86_64-unknown-unknown %s -fexceptions -fcxx-exceptions -pedantic-errors -verify-directives -verify=expected,since-cxx11
// RUN: %clang_cc1 -std=c++2c -triple x86_64-unknown-unknown %s -fexceptions -fcxx-exceptions -pedantic-errors -verify-directives -verify=expected,since-cxx11


namespace cwg2149 { // cwg2149: 3.1
#if __cplusplus <= 201103L
struct X { int i, j, k; };
#else
struct X { int i, j, k = 42; };
#endif

template<int N>
void f1(const X(&)[N]); // #cwg2149-f1

template<int N>
void f2(const X(&)[N][2]); // #cwg2149-f2

void f() {
  X a[] = { 1, 2, 3, 4, 5, 6 };
  static_assert(sizeof(a) / sizeof(X) == 2, "");
  X b[2] = { { 1, 2, 3 }, { 4, 5, 6 } };
  X c[][2] = { 1, 2, 3, 4, 5, 6 };
  static_assert(sizeof(c) / sizeof(X[2]) == 1, "");

  constexpr X ca[] = { 1, 2, 3, 4, 5, 6 };
  constexpr X cb[2] = { { 1, 2, 3 }, { 4, 5, 6 } };
  static_assert(ca[0].i == cb[0].i, "");
  static_assert(ca[0].j == cb[0].j, "");
  static_assert(ca[0].k == cb[0].k, "");
  static_assert(ca[1].i == cb[1].i, "");
  static_assert(ca[1].j == cb[1].j, "");
  static_assert(ca[1].k == cb[1].k, "");

  f1({ 1, 2, 3, 4, 5, 6 });
  // since-cxx11-error@-1 {{no matching function for call to 'f1'}}
  //   since-cxx11-note@#cwg2149-f1 {{candidate function [with N = 6] not viable: no known conversion from 'int' to 'const X' for 1st argument}}
  f2({ 1, 2, 3, 4, 5, 6 });
  // since-cxx11-error@-1 {{no matching function for call to 'f2'}}
  //   since-cxx11-note@#cwg2149-f2 {{candidate function [with N = 6] not viable: no known conversion from 'int' to 'const X[2]' for 1st argument}}
}
} // namespace cwg2149
