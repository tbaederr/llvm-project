// RUN: %clang_cc1 -fsyntax-only -fblocks -Wformat -verify %s -Wno-error=non-pod-varargs
// RUN: %clang_cc1 -fsyntax-only -fblocks -Wformat -verify %s -Wno-error=non-pod-varargs -std=c++11

int (^block) (int, const char *,...) __attribute__((__format__(__printf__,2,3))) = ^ __attribute__((__format__(__printf__,2,3))) (int arg, const char *format,...) {return 5;};

class HasNoCStr {
  const char *str;
 public:
  HasNoCStr(const char *s): str(s) { }
  const char *not_c_str() {return str;}
};

void test_block() {
  const char str[] = "test";
  HasNoCStr hncs(str);
  int n = 4;
  block(n, "%s %d", str, n); // no-warning
  block(n, "%s %s", hncs, n);
  // expected-warning@-1{{format specifies type 'char *' but the argument has type 'HasNoCStr'}}
  // expected-warning@-2{{format specifies type 'char *' but the argument has type 'int'}}
}
