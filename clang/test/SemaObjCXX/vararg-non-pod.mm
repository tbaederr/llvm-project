// RUN: %clang_cc1 -fsyntax-only -verify %s -Wno-error=non-pod-varargs
// RUN: %clang_cc1 -fsyntax-only -verify %s -Wno-error=non-pod-varargs -std=c++11

// expected-no-diagnostics

extern char version[];

@protocol P;

class C {
public:
  C(int);
};

@interface D 
- (void)g:(int)a, ...;
@end

void t1(D *d)
{
  C c(10);

  [d g:10, c]; 
  [d g:10, version];
}

void t2(D *d, id p)
{
  [d g:10, p];
}

void t3(D *d, id<P> p)
{
  [d g:10, p];
}
