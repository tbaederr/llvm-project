/* RUN: %clang_cc1 -x c   -std=c89   -fsyntax-only -verify -pedantic-errors -Wno-empty-translation-unit %s
 * RUN: %clang_cc1 -x c   -std=c99   -fsyntax-only -verify -pedantic-errors -Wno-empty-translation-unit %s
 * RUN: %clang_cc1 -x c++ -std=c++11 -fsyntax-only -verify -pedantic-errors -Wno-empty-translation-unit %s
 */

#if !defined(__cplusplus) && __STDC_VERSION__ < 199901L
/* expected-error@12 {{'long long' is an extension when C99 mode is not enabled}} */
#else
/* expected-no-diagnostics */
#endif

#if 1 > 2LL
#  error should not happen
#endif
