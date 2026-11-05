// RUN: %clang_cc1 -std=gnu++11 -fsyntax-only -verify -Wno-objc-root-class %s
struct Y { 
  Y(); 

private:
  ~Y(); // expected-note {{declared private here}}
};

template<typename T>
struct X : T { }; // expected-note {{default constructor of 'X<Y>' is implicitly deleted because base class 'Y' has an inaccessible destructor}}

struct Z; // expected-note{{forward declaration}}

@interface A {
  X<Y> x; // expected-note{{default constructed field 'x' declared here}}
  Y y; // expected-error{{private destructor}}
}
@end

@implementation A // expected-error{{call to implicitly-deleted default constructor of 'X<Y>'}}
@end

@interface B {
  Z z; // expected-error{{incomplete type}}
}
@end

@implementation B
@end

template<typename T> struct Incomplete; // expected-note{{declared here}}

@interface C {
  Incomplete<int> a[4][4][4]; // expected-error{{implicit instantiation of undefined template 'Incomplete<int>'}}
}
@end
