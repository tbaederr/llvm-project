// RUN: %clang_cc1 -triple i686-windows-gnu  -fms-extensions -verify -std=c++11 %s
// RUN: %clang_cc1 -triple i686-windows-msvc -fms-extensions -verify -std=c++11 %s

// expected-no-diagnostics

struct NonCopyable {
private:
  NonCopyable();
};

struct __declspec(dllexport) S {
  NonCopyable member;
};
