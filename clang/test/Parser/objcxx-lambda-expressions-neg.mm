// RUN: %clang_cc1 -fsyntax-only -Wno-unused-value -verify %s
// RUN: %clang_cc1 -fsyntax-only -Wno-unused-value -verify -std=c++11 %s

int main() {
  []{};
  // expected-no-diagnostics
}
