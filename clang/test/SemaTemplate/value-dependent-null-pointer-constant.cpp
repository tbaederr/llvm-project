// RUN: %clang_cc1 -fsyntax-only -verify %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

template<typename T, int N>
struct X0 {
  const char *f0(bool Cond) {
    return Cond? "honk" : N;
// expected-error@-1 {{incompatible operand types ('const char *' and 'int')}}
  }

  const char *f1(bool Cond) {
    return Cond? N : "honk";
// expected-error@-1 {{incompatible operand types ('int' and 'const char *')}}
  }
  
  bool f2(const char *str) {
    return str == N;
// expected-error@-1 {{comparison between pointer and integer ('const char *' and 'int')}}
  }
};

// PR4996
template<unsigned I> int f0() { 
  return __builtin_choose_expr(I, 0, 1); 
}

// PR5041
struct A { };

template <typename T> void f(T *t)
{
  (void)static_cast<void*>(static_cast<A*>(t));
}
