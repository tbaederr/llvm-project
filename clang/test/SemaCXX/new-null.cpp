// RUN: %clang_cc1 -fsyntax-only -verify %s -std=c++11

typedef __SIZE_TYPE__ size_t;

struct S1 {
   void *operator new(size_t n) {
     return nullptr; // expected-warning {{'operator new' should not return a null pointer unless it is declared 'throw()' or 'noexcept'}}
   }
   void *operator new[](size_t n) noexcept {
     return __null;
   }
};

struct S2 {
   static size_t x;
   void *operator new(size_t n) throw() {
     return 0;
   }
   void *operator new[](size_t n) {
     return (void*)0;
     // expected-warning@-1 {{'operator new[]' should not return a null pointer unless it is declared 'throw()' or 'noexcept'}}
   }
};

struct S3 {
   void *operator new(size_t n) {
     return 1-1;
     // expected-error@-1 {{cannot initialize return object of type 'void *' with an rvalue of type 'int'}}
   }
   void *operator new[](size_t n) {
     return (void*)(1-1); // expected-warning {{'operator new[]' should not return a null pointer unless it is declared 'throw()'}}
   }
};

template<bool B> struct S4 {
  void *operator new(size_t n) noexcept(B) {
    return 0; // expected-warning {{'operator new' should not return a null pointer}}
  }
};
template struct S4<true>;
template struct S4<false>; // expected-note {{in instantiation of}}

template<typename ...T> struct S5 { // expected-warning 0-1{{extension}}
  void *operator new(size_t n) throw(T...) {
    return 0; // expected-warning {{'operator new' should not return a null pointer}}
  }
};
template struct S5<>;
template struct S5<int>; // expected-note {{in instantiation of}}
