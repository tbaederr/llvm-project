// RUN: %clang_cc1 -std=c++11 -fsyntax-only %s

#if 0
R"(
#else
#error in c++98 only)"
#endif
