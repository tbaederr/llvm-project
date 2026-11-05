// RUN: %clang_cc1 -fsyntax-only -verify -Wno-deprecated-builtins -std=c++11 %s
// RUN: %clang_cc1 -fsyntax-only -verify -Wno-deprecated-builtins %s

//   A program that calls for default-initialization or value-initialization of
//   an entity of reference type is illformed. If T is a cv-qualified type, the
//   cv-unqualified version of T is used for these definitions of
//   zero-initialization, default-initialization, and value-initialization.

typedef int &IR;
IR r; // expected-error {{declaration of reference variable 'r' requires an initializer}}
int n = IR(); // expected-error {{reference to type 'int' requires an initializer}}

struct S {
  int &x; // expected-note 4{{because field 'x' of reference type 'int &' would not be initialized}}
};
S s; // expected-error {{deleted default constructor}}
S f() {
  return S(); // expected-error {{deleted default constructor}}
}

struct T
  : S { // expected-note 2{{because base class 'S' has a deleted default constructor}}
};
T t = T(); // expected-error {{deleted default constructor}}

struct U {
  T t[3]; // expected-note {{because field 't' has a deleted default constructor}}
};
U u = U(); // expected-error {{deleted default constructor}}

// Ensure that we handle C++11 in-class initializers properly as an extension.
// In this case, there is no user-declared default constructor, so we
// recursively apply the value-initialization checks, but we will emit a
// constructor call anyway, because the default constructor is not trivial.
struct V {
  int n;
  int &r = n; // expected-warning 0-1{{C++11}}
};
V v = V(); // ok
struct W {
  int n;
  S s = { n }; // expected-warning 0-1{{C++11}}
};
W w = W(); // ok

// Ensure we're not faking this up by making the default constructor
// non-trivial.
_Static_assert(__has_trivial_constructor(S), "");
_Static_assert(__has_trivial_constructor(T), "");
_Static_assert(__has_trivial_constructor(U), "");
_Static_assert(!__has_trivial_constructor(V), "");
_Static_assert(!__has_trivial_constructor(W), "");
