// RUN: %clang_cc1 -std=c++11 -DCXX11 -fsyntax-only %s
// RUN: %clang_cc1 -std=c++20 -DCXX11 -DCXX20 -fsyntax-only %s
// RUN: %clang_cc1 -std=c++11 -DCXX11 -fdeclspec -DDECLSPEC -fsyntax-only %s
// RUN: %clang_cc1 -std=c++11 -DCXX11 -fms-extensions -DDECLSPEC -fsyntax-only %s
// RUN: %clang_cc1 -std=c++11 -DCXX11 -fborland-extensions -DDECLSPEC -fsyntax-only %s
// RUN: %clang_cc1 -std=c++11 -DCXX11 -fms-extensions -fno-declspec -fsyntax-only %s
// RUN: %clang_cc1 -std=c++11 -DCXX11 -fborland-extensions -fno-declspec -fsyntax-only %s
// RUN: %clang_cc1 -std=c++11 -DCXX11 -fno-declspec -fdeclspec -DDECLSPEC -fsyntax-only %s
// RUN: %clang_cc1 -std=c++11 -DCXX11 -fdeclspec -fno-declspec -fsyntax-only %s
// RUN: %clang_cc1 -std=c++11 -DCXX11 -fms-extensions -fno-declspec -fdeclspec -DDECLSPEC -fsyntax-only %s
// RUN: %clang_cc1 -std=c++11 -DCXX11 -fms-extensions -fdeclspec -fno-declspec -fsyntax-only %s
// RUN: %clang -std=c++11 -DCXX11 -target i686-windows-msvc -DMS -DDECLSPEC -fsyntax-only %s
// RUN: %clang -std=c++11 -DCXX11 -target x86_64-scei-ps4 -DDECLSPEC -fsyntax-only %s
// RUN: %clang -std=c++11 -DCXX11 -target i686-windows-msvc -DMS -fno-declspec -fsyntax-only %s
// RUN: %clang -std=c++11 -DCXX11 -target x86_64-scei-ps4 -fno-declspec -fsyntax-only %s

// RUN: %clang_cc1 -std=c++11 -DCXX11 -DFutureKeyword -fsyntax-only -Wc++20-compat -verify=cxx11 %s

#define IS_KEYWORD(NAME) _Static_assert(!__is_identifier(NAME), #NAME)
#define NOT_KEYWORD(NAME) _Static_assert(__is_identifier(NAME), #NAME)
#define IS_TYPE(NAME) void is_##NAME##_type() { int f(NAME); }

#if defined(CXX20)
#define CXX20_KEYWORD(NAME)  IS_KEYWORD(NAME)
#else
#define CXX20_KEYWORD(NAME)  NOT_KEYWORD(NAME)
#endif

#ifdef DECLSPEC
#define DECLSPEC_KEYWORD(NAME)  IS_KEYWORD(NAME)
#else
#define DECLSPEC_KEYWORD(NAME)  NOT_KEYWORD(NAME)
#endif

#ifdef CXX11
#define CXX11_KEYWORD(NAME)  IS_KEYWORD(NAME)
#define CXX11_TYPE(NAME)     IS_TYPE(NAME)
#else
#define CXX11_KEYWORD(NAME)  NOT_KEYWORD(NAME)
#define CXX11_TYPE(NAME)
#endif

// C++11 keywords
CXX11_KEYWORD(nullptr);
CXX11_KEYWORD(decltype);
CXX11_KEYWORD(alignof);
CXX11_KEYWORD(alignas);
CXX11_KEYWORD(char16_t);
CXX11_TYPE(char16_t);
CXX11_KEYWORD(char32_t);
CXX11_TYPE(char32_t);
CXX11_KEYWORD(constexpr);
CXX11_KEYWORD(noexcept);

#ifndef MS
CXX11_KEYWORD(static_assert);
#else
// MS compiler recognizes static_assert in all modes. So should we.
IS_KEYWORD(static_assert);
#endif

CXX11_KEYWORD(thread_local);

// Concepts keywords
CXX20_KEYWORD(concept);
CXX20_KEYWORD(requires);
CXX20_KEYWORD(consteval);
CXX20_KEYWORD(constinit);
CXX20_KEYWORD(co_await);
CXX20_KEYWORD(co_return);
CXX20_KEYWORD(co_yield);

// __declspec extension
DECLSPEC_KEYWORD(__declspec);

// Clang extension
IS_KEYWORD(__char16_t);
IS_TYPE(__char16_t);
IS_KEYWORD(__char32_t);
IS_TYPE(__char32_t);

#ifdef FutureKeyword

int co_await; // cxx11-warning {{'co_await' is a keyword in C++20}}
char co_return; // cxx11-warning {{'co_return' is a keyword in C++20}}
char co_yield; // cxx11-warning {{'co_yield' is a keyword in C++20}}
int constinit; // cxx11-warning {{'constinit' is a keyword in C++20}}
int consteval; // cxx11-warning {{'consteval' is a keyword in C++20}}
int requires; // cxx11-warning {{'requires' is a keyword in C++20}}
int concept; // cxx11-warning {{'concept' is a keyword in C++20}}

#endif
