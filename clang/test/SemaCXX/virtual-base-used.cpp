// RUN: %clang_cc1 -fsyntax-only -triple %itanium_abi_triple -verify %s
// RUN: %clang_cc1 -fsyntax-only -triple %itanium_abi_triple -verify -std=c++11 %s
// RUN: %clang_cc1 -fsyntax-only -triple %ms_abi_triple -DMSABI -verify %s
// RUN: %clang_cc1 -fsyntax-only -triple %ms_abi_triple -DMSABI -verify -std=c++11 %s
// PR7800

// The Microsoft ABI doesn't have the concept of key functions, so we have different
// expectations about when functions are first required for that case.

class NoDestroy { ~NoDestroy(); };

struct A {
  virtual ~A();
  // expected-note@-1 3 {{overridden virtual function is here}}
};

struct B : public virtual A {
// expected-error@-1 {{deleted function '~B' cannot override a non-deleted function}}
// expected-note@-2 {{overridden virtual function is here}}

  NoDestroy x;
  // expected-note@-1 {{destructor of 'B' is implicitly deleted because field 'x' has an inaccessible destructor}}
#ifdef MSABI
  // expected-note@-3 {{default constructor of 'B' is implicitly deleted because field 'x' has an inaccessible destructor}}
#endif
};

struct D : public virtual B {
#ifdef MSABI
// expected-note@-2 {{default constructor of 'D' is implicitly deleted because base class 'B' has a deleted default constructor}}
#endif
  virtual void foo();
  ~D();
  //expected-error@-1 {{non-deleted function '~D' cannot override a deleted function}}
};

#ifdef MSABI
D d;
// expected-error@-1 {{call to implicitly-deleted default constructor of 'D'}}å
#else
void D::foo() {
}
#endif

struct E : public virtual A {
// expected-error@-1 {{deleted function '~E' cannot override a non-deleted function}}

  NoDestroy x;
  // expected-note@-1 {{destructor of 'E' is implicitly deleted because field 'x' has an inaccessible destructor}}
#ifdef MSABI
  // expected-note@-3 {{default constructor of 'E' is implicitly deleted because field 'x' has an inaccessible destructor}}
#endif
};

struct F : public E {
// expected-note@-1 {{overridden virtual function is here}}
#ifdef MSABI
// expected-note@-3 {{default constructor of 'F' is implicitly deleted because base class 'E' has a deleted default constructor}}
#endif
};


struct G : public virtual F {
#ifdef MSABI
// expected-note@-2 {{default constructor of 'G' is implicitly deleted because base class 'F' has a deleted default constructor}}
#endif

  virtual void foo();
  ~G();
  //expected-error@-1 {{non-deleted function '~G' cannot override a deleted function}}
};

#ifdef MSABI
G g;
// expected-error@-1 {{call to implicitly-deleted default constructor of 'G'}}
#else
void G::foo() {
}
#endif

struct H : public virtual A {
// expected-error@-1 {{deleted function '~H' cannot override a non-deleted function}}
// expected-note@-2 {{overridden virtual function is here}}

  NoDestroy x;
  // expected-note@-1 {{destructor of 'H' is implicitly deleted because field 'x' has an inaccessible destructor}}
#ifdef MSABI
  // expected-note@-3 {{default constructor of 'H' is implicitly deleted because field 'x' has an inaccessible destructor}}
#endif
};

struct I : public virtual H {
#ifdef MSABI
// expected-note@-2 {{default constructor of 'I' is implicitly deleted because base class 'H' has a deleted default constructor}}
#endif

  ~I();
// expected-error@-1 {{non-deleted function '~I' cannot override a deleted function}}
};

struct J : public I {
#ifdef MSABI
// expected-note@-2 {{default constructor of 'J' is implicitly deleted because base class 'I' has a deleted default constructor}}
#endif

  virtual void foo();
  ~J();
};

#ifdef MSABI
J j;
// expected-error@-1 {{call to implicitly-deleted default constructor of 'J'}}

#else
void J::foo() {
}
#endif
