// RUN: %clang_cc1 -fsyntax-only -verify -Wreserved-user-defined-literal -fms-extensions -fms-compatibility -std=c++11 %s

#define bar(x) #x
const char * f() {
  return "foo"bar("bar")"baz";    /*expected-warning {{invalid suffix on literal; C++11 requires a space between literal and identifier}} */
}
