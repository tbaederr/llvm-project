// RUN: %clang_cc1 -fsyntax-only -triple %itanium_abi_triple -verify %s
// RUN: %clang_cc1 -fsyntax-only -triple %itanium_abi_triple -verify -std=c++11 %s
// RUN: %clang_cc1 -fsyntax-only -triple %ms_abi_triple -DMSABI -verify %s
// RUN: %clang_cc1 -fsyntax-only -triple %ms_abi_triple -DMSABI -verify -std=c++11 %s

struct A {
  virtual ~A();
// expected-note@-1 3 {{overridden virtual function is here}}
};

struct B : A {
// expected-error@-1 {{deleted function '~B' cannot override a non-deleted function}}
// expected-note@-2  {{virtual destructor requires an unambiguous, accessible 'operator delete'}}
#ifdef MSABI
// expected-note@-4 {{virtual destructor requires an unambiguous, accessible 'operator delete'}}
#endif
  virtual void f();

  void operator delete (void *, int);
};

#ifdef MSABI
B b;
// expected-error@-1 {{attempt to use a deleted function}}

#else
void B::f() {
}
#endif

struct C : A {
// expected-error@-1 {{deleted function '~C' cannot override a non-deleted function}}
// expected-note@-2  {{virtual destructor requires an unambiguous, accessible 'operator delete'}}

  C();
  void operator delete(void *, int);
};

C::C() { }

struct D : A {
// expected-error@-1 {{deleted function '~D' cannot override a non-deleted function}}
// expected-note@-2  {{virtual destructor requires an unambiguous, accessible 'operator delete'}}
  void operator delete(void *, int);
}; 

void f() {
  new D;
}
