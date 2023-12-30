// RUN: %clang_cc1 -fexperimental-new-constant-interpreter -verify=both,expected -std=c++98 %s
// RUN: %clang_cc1 -verify=both,ref -std=c++98 %s



namespace IntOrEnum {
  const int k = 0;
  const int &p = k; // both-note {{declared here}}
  template<int n> struct S {};
  S<p> s; // both-error {{not an integral constant expression}} \
          // both-note {{read of variable 'p' of non-integral, non-enumeration type 'const int &'}}
}
