// RUN: %clang_cc1 -fexperimental-new-constant-interpreter -verify=both,expected -std=c++11 %s
// RUN: %clang_cc1 -verify=both,ref -std=c++11 %s



// expected-no-diagnostics
// ref-no-diagnostics


constexpr _Atomic(bool) B = true;
static_assert(B, "");

