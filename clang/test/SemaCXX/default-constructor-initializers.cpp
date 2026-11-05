// RUN: %clang_cc1 -fsyntax-only -verify %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

struct X1 { // has no implicit default constructor
   X1(int);
};

struct X2  : X1 {

   X2(int);
};

struct X3 : public X2 {
// expected-note@-1 {{default constructor of 'X3' is implicitly deleted because base class 'X2' has no default constructor}}
};

X3 x3;
// expected-error@-1 {{call to implicitly-deleted default constructor of 'X3'}}

struct X4 {

  X2 x2;
  // expected-note@-1 {{default constructor of 'X4' is implicitly deleted because field 'x2' has no default constructor}}

  X2 & rx2;
};

X4 x4;
// expected-error@-1 {{call to implicitly-deleted default constructor of 'X4'}}

struct Y1 { // has no implicit default constructor
   Y1(int);
};

struct Y2  : Y1 { 
   Y2(int);
   Y2();
};

struct Y3 : public Y2 {
};
Y3 y3; 

struct Y4 {
  Y2 y2; 
};

Y4 y4;

// More tests

struct Z1 {

  int& z;
  // expected-note@-1 {{default constructor of 'Z1' is implicitly deleted because field 'z' of reference type 'int &' would not be initialized}}

  const int c1;
  volatile int v1;
};

// Test default initialization which *requires* a constructor call for non-POD.
Z1 z1;
// expected-error@-1 {{call to implicitly-deleted default constructor of 'Z1'}}

// Ensure that value initialization doesn't use trivial implicit constructors.
namespace PR7948 {
  // Note that this is also non-POD to ensure we don't just special case PODs.
  struct S { const int x; ~S(); };
  const S arr[2] = { { 42 } };
}

// This is valid
union U {
  const int i;
  float f;
};
U u;
