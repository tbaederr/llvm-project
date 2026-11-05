// RUN: %clang_cc1 -fsyntax-only -verify=expected,expected-cxx11 %s
// RUN: %clang_cc1 -fsyntax-only -verify=expected,expected-cxx11 -std=c++11 %s

namespace BooleanFalse {
int* j = false;
// expected-error@-1 {{cannot initialize a variable of type 'int *' with an rvalue of type 'bool'}}

// expected-error@+2 {{cannot initialize a parameter of type 'int *' with an rvalue of type 'bool'}}
// expected-note@+1 {{passing argument to parameter 'j' here}}
void bar(int *j = false);

// expected-note@+1 4{{candidate function not viable: no known conversion}}
void foo(int *i)
{
  foo(false);
// expected-error@-1 {{no matching function for call to 'foo'}}

  foo((int*)false); // OK: explicit cast
  foo(0); // OK: not a bool, even though it's convertible to bool

  foo(false == true);
// expected-error@-1 {{no matching function for call to 'foo'}}

  foo((42 + 24) < 32);
// expected-error@-1 {{no matching function for call to 'foo'}}

  const bool kFlag = false;
  foo(kFlag);
// expected-error@-1 {{no matching function for call to 'foo'}}
}

char f(struct Undefined*);
double f(...);

// Ensure that when using false in metaprogramming machinery its conversion
// isn't flagged.
template <int N> struct S {};
S<sizeof(f(false))> s;

}

namespace Function {
void f1();

struct S {
  static void f2();
};

extern void f3() __attribute__((weak_import));

struct S2 {
  static void f4() __attribute__((weak_import));
};

bool f5();
bool f6(int);
auto f7 = []{};
auto f8 = [](){};

void foo() {
  bool b;
  b = f7; // expected-warning {{address of lambda function pointer conversion operator will always evaluate to 'true'}}
  b = f8; // expected-warning {{address of lambda function pointer conversion operator will always evaluate to 'true'}}
  bool is_true = [](){ return true; };
  // expected-warning@-1{{address of lambda function pointer conversion operator will always evaluate to 'true'}}
}

template <typename... Ts>
static bool IsFalse(const Ts&...) { return false; }
template <typename T>
static bool IsFalse(const T& p) {
  bool b;
  b = f7; // expected-warning {{address of lambda function pointer conversion operator will always evaluate to 'true'}}
  // Intentionally not warned on because p could be a lambda type in one
  // instantiation, but a pointer type in another.
  return p ? false : true;
}

bool use_instantiation() {
  return IsFalse([]() { return 0; });
}

void bar() {
  bool b;

  b = f1; // expected-warning {{address of function 'f1' will always evaluate to 'true'}} \
             expected-note {{prefix with the address-of operator to silence this warning}}
  if (f1) {} // expected-warning {{address of function 'f1' will always evaluate to 'true'}} \
                expected-note {{prefix with the address-of operator to silence this warning}}
  b = S::f2; // expected-warning {{address of function 'S::f2' will always evaluate to 'true'}} \
                expected-note {{prefix with the address-of operator to silence this warning}}
  if (S::f2) {} // expected-warning {{address of function 'S::f2' will always evaluate to 'true'}} \
                   expected-note {{prefix with the address-of operator to silence this warning}}
  b = f5; // expected-warning {{address of function 'f5' will always evaluate to 'true'}} \
             expected-note {{prefix with the address-of operator to silence this warning}} \
             expected-note {{suffix with parentheses to turn this into a function call}}
  b = f6; // expected-warning {{address of function 'f6' will always evaluate to 'true'}} \
             expected-note {{prefix with the address-of operator to silence this warning}}

  // implicit casts of weakly imported symbols are ok:
  b = f3;
  if (f3) {}
  b = S2::f4;
  if (S2::f4) {}
}

// GH46362: a reference to a function is never null, like a bare function name.
void func_ref(void (&f)(), int *&ptr) {
  bool b;
  b = f; // expected-warning {{address of function 'f' will always evaluate to 'true'}}
  if (f) {} // expected-warning {{address of function 'f' will always evaluate to 'true'}}
  b = ptr;
  if (ptr) {}
}
}

namespace Array {
  #define GetValue(ptr)  ((ptr) ? ptr[0] : 0)
  extern int a[] __attribute__((weak));
  int b[] = {8,13,21};
  struct {
    int x[10];
  } c;
  const char str[] = "text";
  void ignore() {
    if (a) {}
    if (a) {}
    (void)GetValue(b);
  }
  void test() {
    if (b) {}
    // expected-warning@-1{{address of array 'b' will always evaluate to 'true'}}
    if (b) {}
    // expected-warning@-1{{address of array 'b' will always evaluate to 'true'}}
    if (c.x) {}
    // expected-warning@-1{{address of array 'c.x' will always evaluate to 'true'}}
    if (str) {}
    // expected-warning@-1{{address of array 'str' will always evaluate to 'true'}}
  }
}

namespace Pointer {
  extern int a __attribute__((weak));
  int b;
  static int c;
  class S {
  public:
    static int a;
    int b;
  };
  void ignored() {
    if (&a) {}
  }
  void test() {
    S s;
    if (&b) {}
    // expected-warning@-1{{address of 'b' will always evaluate to 'true'}}
    if (&c) {}
    // expected-warning@-1{{address of 'c' will always evaluate to 'true'}}
    if (&s.a) {}
    // expected-warning@-1{{address of 's.a' will always evaluate to 'true'}}
    if (&s.b) {}
    // expected-warning@-1{{address of 's.b' will always evaluate to 'true'}}
    if (&S::a) {}
    // expected-warning@-1{{address of 'S::a' will always evaluate to 'true'}}
  }
}

namespace macros {
  #define assert(x) if (x) {}
  #define zero_on_null(x) ((x) ? *(x) : 0)

  int array[5];
  void fun();
  int x;

  void test() {
    assert(array);
    assert(array && "expecting null pointer");
    // expected-warning@-1{{address of array 'array' will always evaluate to 'true'}}

    assert(fun);
    assert(fun && "expecting null pointer");
    // expected-warning@-1{{address of function 'fun' will always evaluate to 'true'}}
    // expected-note@-2 {{prefix with the address-of operator to silence this warning}}

    // TODO: warn on assert(&x) while not warning on zero_on_null(&x)
    zero_on_null(&x);
    assert(zero_on_null(&x));
    assert(&x);
    assert(&x && "expecting null pointer");
    // expected-warning@-1{{address of 'x' will always evaluate to 'true'}}
  }
}

#if __cplusplus < 201703L
namespace Template {
  // FIXME: These cases should not warn.
  template<int *p> void f() { if (p) {} } // expected-warning 2{{will always evaluate to 'true'}} expected-cxx11-warning {{implicit conversion of nullptr}}
  template<int (*p)[3]> void g() { if (p) {} } // expected-warning 2{{will always evaluate to 'true'}} expected-cxx11-warning {{implicit conversion of nullptr}}
  template<int (*p)()> void h() { if (p) {} }

  int a, b[3], c[3][3], d();
  template void f<&a>(); // expected-note {{instantiation of}}
  template void f<b>(); // expected-note {{instantiation of}}
  template void f<(int*)nullptr>(); // expected-note {{instantiation of}}
  template void g<&b>(); // expected-note {{instantiation of}}
  template void g<c>(); // expected-note {{instantiation of}}
  template void g<(int(*)[3])nullptr>(); // expected-note {{instantiation of}}
  template void h<d>();
}
#endif // __cplusplus < 201703L
