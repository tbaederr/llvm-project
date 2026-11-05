// RUN: %clang_cc1 -fsyntax-only -verify %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

namespace std {
  template<typename T> class vector { }; // expected-note{{candidate function (the implicit copy assignment operator) not viable}}
  // expected-note@-1 {{candidate function (the implicit move assignment operator) not viable}}
}

typedef int INT;
typedef float Real;

void test() {
  using namespace std;

  std::vector<INT> v1;
  vector<Real> v2;
  v1 = v2; // expected-error{{no viable overloaded '='}}
}
