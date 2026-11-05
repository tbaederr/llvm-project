// RUN: %clang_cc1 -fsyntax-only -verify %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

// Test instantiation of static data members declared out-of-line.

template<typename T>
struct X {
  static T value;
};

template<typename T> 
  T X<T>::value = 17; // expected-error{{no viable conversion}}

struct InitOkay {
  InitOkay(int) { }
};

struct CannotInit { }; // expected-note{{candidate constructor (the implicit copy constructor) not viable}}
// expected-note@-1 {{candidate constructor (the implicit move constructor) not viable}}

int &returnInt() { return X<int>::value; }
float &returnFloat() { return X<float>::value; }

InitOkay &returnInitOkay() { return X<InitOkay>::value; }

unsigned long sizeOkay() { return sizeof(X<CannotInit>::value); }
  
CannotInit &returnError() {
  return X<CannotInit>::value; // expected-note{{instantiation}}
}
