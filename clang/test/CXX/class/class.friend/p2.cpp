// RUN: %clang_cc1 -fsyntax-only -verify %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

struct B0;

class A {
  friend class B {}; // expected-error {{cannot define a type in a friend declaration}}
  friend int;
  friend B0;
  friend class C; // okay
};
