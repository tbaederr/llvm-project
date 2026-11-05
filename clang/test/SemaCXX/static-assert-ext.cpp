// RUN: %clang_cc1 -std=c++11 -fsyntax-only -pedantic %s -verify=since-cxx11,precxx17,precxx26
// RUN: %clang_cc1 -std=c++17 -fsyntax-only -pedantic %s -verify=since-cxx11,since-cxx17,precxx26 -Wpre-c++17-compat
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -pedantic %s -verify=since-cxx11,since-cxx17,since-cxx26 -Wpre-c++17-compat -Wpre-c++26-compat

static_assert(false, "a");
// since-cxx11-error@-1 {{static assertion failed: a}}

static_assert(false);
// precxx17-warning@-1 {{'static_assert' with no message is a C++17 extension}}
// since-cxx17-warning@-2 {{'static_assert' with no message is incompatible with C++ standards before C++17}}
// since-cxx11-error@-3 {{static assertion failed}}

struct X {
    static constexpr int size() { return 1; }
    static constexpr const char* data() { return "b"; }
};

static_assert(false, X());
// precxx26-warning@-1 {{'static_assert' with a user-generated message is a C++2c extension}}
// since-cxx26-warning@-2 {{'static_assert' with a user-generated message is incompatible with C++ standards before C++2c}}
// since-cxx11-error@-3 {{static assertion failed: b}}
