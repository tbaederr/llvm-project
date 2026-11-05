// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

// https://github.com/llvm/llvm-project/issues/173943

constexpr void f(this auto& self)
    // expected-warning@-1 {{'auto' parameters are a C++20 extension}}
    // expected-error@-2 {{explicit object parameters are incompatible with C++ standards before C++2b}}
    // expected-error@-3 {{expected ';' after top level declarator}}
    // expected-error@-4 {{an explicit object parameter cannot appear in a non-member function}}

void g() {
  f(); // expected-error {{no matching function for call to 'f'}}
}
