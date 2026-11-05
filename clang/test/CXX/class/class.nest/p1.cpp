// RUN: %clang_cc1 -fsyntax-only -verify %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

class Outer {
  int x;
  static int sx;
  int f();

  // C++11 does relax this rule (see 5.1.1.10) in the first case, but we need to enforce it in C++03 mode.
  class Inner {
    static char a[sizeof(x)];
    static char b[sizeof(sx)]; // okay
    static char c[sizeof(f)]; // expected-error {{call to non-static member function without an object argument}}
  };
};
