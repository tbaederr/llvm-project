// RUN: %clang_cc1 -fsyntax-only -std=c++17 -Wc++11-compat-pedantic -verify %s


decltype(auto) x = 0; // expected-warning {{'decltype(auto)' type specifier is incompatible}}

auto init_capture = [a(0)] {}; // expected-warning {{initialized lambda captures are incompatible with C++ standards before C++14}}

auto generic_lambda =
  [](
       auto // expected-warning {{generic lambdas are incompatible}}
    *a) {};

auto deduced_return_type(); // expected-warning {{incompatible with C++ standards before C++14}}
auto *another_deduced_return_type(); // expected-warning {{incompatible with C++ standards before C++14}}
decltype(auto) also_deduced_return_type(); // expected-warning {{return type deduction}} expected-warning {{'decltype(auto)' type specifier is incompatible}}
int f();
auto (*not_deduced_return_type)() = f;

auto deduced_lambda_return_type = []() ->
  auto // expected-warning {{return type deduction is incompatible}}
{};

auto trailing_non_deduced_return_type() -> int;
auto trailing_deduced_return_type() -> auto; // expected-warning {{incompatible with C++ standards before C++14}}

struct A {
  operator auto(); // expected-warning {{return type deduction is incompatible}}
};

