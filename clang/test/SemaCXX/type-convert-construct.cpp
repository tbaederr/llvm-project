// RUN: %clang_cc1 -fsyntax-only -verify %s 
// RUN: %clang_cc1 -fsyntax-only -verify -std=gnu++11 %s 

void f() {
  float v1 = float(1);
  int v2 = typeof(int)(1,2); // expected-error {{excess elements in scalar initializer}}
  typedef int arr[];
  arr(); // expected-error {{array types cannot be value-initialized}}
  typedef void fn_ty();
  fn_ty(); // expected-error {{cannot create object of function type 'fn_ty'}}
  fn_ty(0); // expected-error {{functional-style cast from 'int' to 'fn_ty'}}
  fn_ty(0, 0); // expected-error {{cannot create object of function type 'fn_ty'}}
  fn_ty{}; // expected-error {{cannot create object of function type 'fn_ty'}}
  fn_ty{0}; // expected-error {{cannot create object of function type 'fn_ty'}}
  fn_ty{0, 0}; // expected-error {{cannot create object of function type 'fn_ty'}}
  fn_ty({}); // expected-error {{cannot create object of function type 'fn_ty'}}
  int v4 = int();
  int v5 = int; // expected-error {{expected '(' for function-style cast or type construction}}
  typedef int T;
  int *p;
  bool v6 = T(0) == p;
  // expected-error@-1 {{comparison between pointer and integer ('T' (aka 'int') and 'int *')}}
  char *str;
  str = "a string";
  // expected-warning@-1 {{ISO C++11 does not allow conversion from string literal to 'char *'}}
  wchar_t *wstr;
  wstr = L"a wide string";
  // expected-warning@-1 {{ISO C++11 does not allow conversion from string literal to 'wchar_t *'}}
}
