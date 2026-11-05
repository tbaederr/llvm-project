// RUN: %clang_cc1 -fsyntax-only -verify %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=gnu++11 %s

static void test() {
  int *pi;
  int x;
  typeof pi[x] y; 
}

// From the gcc test suite.
struct S {
  int i;
  __typeof(S::i) foo();
  // expected-no-diagnostics
};
