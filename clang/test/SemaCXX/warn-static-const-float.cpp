// RUN: %clang_cc1 -verify %s -std=c++11 -DERR
// RUN: %clang_cc1 -verify %s -std=c++11 -Wno-gnu -DERR
// RUN: %clang_cc1 -verify %s -std=c++11 -Wno-static-float-init -DNONE
// RUN: %clang_cc1 -verify %s -std=c++11 -Wno-gnu-static-float-init -DERR

#if NONE
// expected-no-diagnostics
#elif ERR
// expected-error@14 {{in-class initializer for static data member of type 'const double' requires 'constexpr' specifier}}
// expected-note@14 {{add 'constexpr'}}
#endif

struct X {
  static const double x = 0.0;
};
