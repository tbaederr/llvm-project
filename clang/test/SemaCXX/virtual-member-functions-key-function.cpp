// RUN: %clang_cc1 -fsyntax-only -verify %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

struct A {
  virtual ~A();
// expected-note@-1 2 {{overridden virtual function is here}}
};

struct B : A {
// expected-error@-1 {{deleted function '~B' cannot override a non-deleted function}}
// expected-note@-2  {{virtual destructor requires an unambiguous, accessible 'operator delete'}}
  B() { }

  void operator delete(void *, int);
}; 

struct C : A {
// expected-error@-1 {{deleted function '~C' cannot override a non-deleted function}}
// expected-note@-2  {{virtual destructor requires an unambiguous, accessible 'operator delete'}}
  void operator delete(void *, int);
}; 

void f() {
  (void)new B; 
  (void)new C;
}

// Make sure that the key-function computation is consistent when the
// first virtual member function of a nested class has an inline body.
struct Outer {
  struct Inner {
    virtual void f() { }
    void g();
  };
};

void Outer::Inner::g() { }
