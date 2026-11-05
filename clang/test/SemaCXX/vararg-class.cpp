// RUN: %clang_cc1 -verify -Wclass-varargs -std=c++11 %s

struct A {};
struct B { ~B(); };
class C { char *c_str(); };
struct D { char *c_str(); };
struct E { E(); };
struct F { F(); char *c_str(); };

void v(...);
void w(const char*, ...) __attribute__((format(printf, 1, 2)));

void test(A a, B b, C c, D d, E e, F f) {
  v(a); // expected-warning-re {{passing object of class type 'A' through variadic function{{$}}}}
  v(b); // expected-error-re {{cannot pass object of non-{{POD|trivial}} type 'B' through variadic function; call will abort at runtime}}
  v(c); // expected-warning {{passing object of class type 'C' through variadic function; did you mean to call '.c_str()'?}}
  v(d); // expected-warning {{passing object of class type 'D' through variadic function; did you mean to call '.c_str()'?}}
  v(e);
  v(f);
  // expected-warning-re@-2 {{passing object of class type 'E' through variadic function{{$}}}}
  // expected-warning@-2 {{passing object of class type 'F' through variadic function; did you mean to call '.c_str()'?}}

  v(d.c_str());
  v(f.c_str());
  v(0);
  v('x');

  w("%s", a); // expected-warning {{format specifies type 'char *' but the argument has type 'A'}}
  w("%s", b); // expected-error-re {{cannot pass non-{{POD|trivial}} object of type 'B' to variadic function; expected type from format string was 'char *'}}
  w("%s", c); // expected-warning {{format specifies type 'char *' but the argument has type 'C'}}
  w("%s", d); // expected-warning {{format specifies type 'char *' but the argument has type 'D'}}
  w("%s", e);
  w("%s", f);
  // expected-warning@-2 {{format specifies type 'char *' but the argument has type 'E'}}
  // expected-warning@-2 {{format specifies type 'char *' but the argument has type 'F'}}
}
