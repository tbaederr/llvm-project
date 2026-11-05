// RUN: %clang_cc1 -std=c++11 %s -fexceptions -fcxx-exceptions -pedantic-errors -verify=expected,since-cxx11,cxx11
// RUN: %clang_cc1 -std=c++14 %s -fexceptions -fcxx-exceptions -pedantic-errors -verify=expected,since-cxx11,since-cxx14
// RUN: %clang_cc1 -std=c++17 %s -fexceptions -fcxx-exceptions -pedantic-errors -verify=expected,since-cxx11,since-cxx14
// RUN: %clang_cc1 -std=c++20 %s -fexceptions -fcxx-exceptions -pedantic-errors -verify=expected,since-cxx11,since-cxx14,since-cxx20
// RUN: %clang_cc1 -std=c++23 %s -fexceptions -fcxx-exceptions -pedantic-errors -verify=expected,since-cxx11,since-cxx14,since-cxx20
// RUN: %clang_cc1 -std=c++2c %s -fexceptions -fcxx-exceptions -pedantic-errors -verify=expected,since-cxx11,since-cxx14,since-cxx20

namespace cwg2026 { // cwg2026: 11
  template<int> struct X {};

  const int a = a + 1; // #cwg2026-a
  // expected-warning@-1 {{variable 'a' is uninitialized when used within its own initialization}}
  X<a> xa; // #cwg2026-xa
  // since-cxx11-error@#cwg2026-xa {{non-type template argument is not a constant expression}}
  //   since-cxx11-note@#cwg2026-xa {{initializer of 'a' is not a constant expression}}
  //   since-cxx11-note@#cwg2026-a {{declared here}}

  constexpr int b = b;
  // since-cxx11-error@-1 {{constexpr variable 'b' must be initialized by a constant expression}}
  //   since-cxx11-note@-2 {{read of object outside its lifetime is not allowed in a constant expression}}
  //   since-cxx11-note@-3 {{declared here}}
  [[clang::require_constant_initialization]] int c = c;
  // since-cxx11-error@-1 {{variable does not have a constant initializer}}
  //   since-cxx11-note@-2 {{required by 'require_constant_initialization' attribute here}}
  //   cxx11-note@-3 {{read of non-const variable 'c' is not allowed in a constant expression}}
  //   cxx11-note@-4 {{declared here}}
  //   since-cxx14-note@-5 {{read of object outside its lifetime is not allowed in a constant expression}}
  //   since-cxx14-note@-6 {{declared here}}

#if __cplusplus >= 202002L
  constinit int d = d;
  // since-cxx20-error@-1 {{variable does not have a constant initializer}}
  //   since-cxx20-note@-2 {{required by 'constinit' specifier here}}
  //   since-cxx20-note@-3 {{read of object outside its lifetime is not allowed in a constant expression}}
  //   since-cxx20-note@-4 {{declared here}}
#endif

  void f() {
    static const int e = e + 1; // #cwg2026-e
    // expected-warning@-1 {{static variable 'e' is suspiciously used within its own initialization}}
    X<e> xe; // #cwg2026-xe
    // since-cxx11-error@#cwg2026-xe {{non-type template argument is not a constant expression}}
    //   since-cxx11-note@#cwg2026-xe {{initializer of 'e' is not a constant expression}}
    //   since-cxx11-note@#cwg2026-e {{declared here}}

    static constexpr int f = f;
    // since-cxx11-error@-1 {{constexpr variable 'f' must be initialized by a constant expression}}
    //   since-cxx11-note@-2 {{read of object outside its lifetime is not allowed in a constant expression}}
    //   since-cxx11-note@-3 {{declared here}}
    [[clang::require_constant_initialization]] static int g = g;
    // since-cxx11-error@-1 {{variable does not have a constant initializer}}
    //   since-cxx11-note@-2 {{required by 'require_constant_initialization' attribute here}}
    //   cxx11-note@-3 {{read of non-const variable 'g' is not allowed in a constant expression}}
    //   cxx11-note@-4 {{declared here}}
    //   since-cxx14-note@-5 {{read of object outside its lifetime is not allowed in a constant expression}}
    //   since-cxx14-note@-6 {{declared here}}

#if __cplusplus >= 202002L
    static constinit int h = h;
    // since-cxx20-error@-1 {{variable does not have a constant initializer}}
    //   since-cxx20-note@-2 {{required by 'constinit' specifier here}}
    //   since-cxx20-note@-3 {{read of object outside its lifetime is not allowed in a constant expression}}
    //   since-cxx20-note@-4 {{declared here}}
#endif
  }
} // namespace cwg2026
