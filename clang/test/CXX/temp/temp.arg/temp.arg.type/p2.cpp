// RUN: %clang_cc1 -fsyntax-only -verify -Wvla %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

template<class T> struct A {
  static T t; // expected-error{{static data member instantiated with function type 'int ()'}}
};
typedef int function();
A<function> a; // expected-note{{instantiation of}}

template<typename T> struct B {
  B() { T t; } // expected-error{{variable instantiated with function type 'int ()'}}
};
B<function> b; // expected-note{{instantiation of}}

template <typename T> int f0(void *, const T&); // expected-note{{candidate template ignored: substitution failure}}
enum {e};

void test_f0(int n) { // #here
  int i = f0(0, e);

  int vla[n]; // expected-warning {{variable length arrays in C++ are a Clang extension}}
  // expected-note@-1 {{function parameter 'n' with unknown value cannot be used in a constant expression}}
  // expected-note@#here {{declared here}}
  f0(0, vla); // expected-error{{no matching function for call to 'f0'}}
}

namespace N0 {
  template <typename R, typename A1> void f0(R (*)(A1));
  template <typename T> int f1(T);
  template <typename T, typename U> int f1(T, U);
  enum {e1};

  enum {e2};

  enum {e3};

  template<typename T> struct X;
  template<typename T> struct X<T*> { };

  void f() {
    f0(

       &f1<__typeof__(e1)>);

    int (*fp1)(int, __typeof__(e2)) = f1;

    f1(e2);

    f1(e2);

    X<__typeof__(e3)*> x;
  }
}
