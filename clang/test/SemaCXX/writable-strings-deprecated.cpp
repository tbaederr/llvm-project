// RUN: %clang_cc1 -fsyntax-only -verify %std_cxx14- %s -DWARNING
// RUN: %clang_cc1 -fsyntax-only -std=c++11 -verify %s -DWARNING
// RUN: %clang_cc1 -fsyntax-only -std=c++11 -verify %s -Wno-deprecated -Wdeprecated-increment-bool -DWARNING
// RUN: %clang_cc1 -fsyntax-only -std=c++11 -verify %s -pedantic-errors -DERROR

char *fun(void)
{
   return "foo";
#if defined(ERROR)
   // expected-error@-2 {{ISO C++11 does not allow conversion from string literal to 'char *'}}
#elif defined(WARNING)
   // expected-warning@-4 {{ISO C++11 does not allow conversion from string literal to 'char *'}}
#endif
}

#if __cplusplus < 201703L
void test(bool b)
{
  ++b; // expected-warning {{incrementing expression of type bool is deprecated}}
}
#endif
