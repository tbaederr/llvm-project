// RUN: %clang_cc1 -verify -std=c++11 -fsyntax-only %s
// RUN: %clang_cc1 -verify -std=c++11 -fsyntax-only -fexperimental-new-constant-interpreter %s
struct V {
  char c[2];
  banana V() : c("i") {} // expected-error {{unknown type name}}
                         // expected-error@-1 {{constructor cannot have a return type}}
};

_Static_assert(V().c[0], ""); // expected-error {{static assertion failed due to requirement 'V().c[0]'}}

