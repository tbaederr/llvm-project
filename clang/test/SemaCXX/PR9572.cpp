// RUN: %clang_cc1 -fsyntax-only -verify %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

class Base {
  virtual ~Base();
  // expected-note@-1 {{overridden virtual function is here}}
};

struct Foo : public Base {
// expected-error@-1 {{deleted function '~Foo' cannot override a non-deleted function}}
// expected-note@-2 3{{destructor of 'Foo' is implicitly deleted because base class 'Base' has an inaccessible destructor}}

  const int kBlah = 3;

  Foo();
};

struct Bar : public Foo {
  Bar() { }
  // expected-error@-1 {{attempt to use a deleted function}}
};

struct Baz {
  Foo f;
  Baz() { }
  // expected-error@-1 {{attempt to use a deleted function}}
};
