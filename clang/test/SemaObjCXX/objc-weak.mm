// RUN: %clang_cc1 -fsyntax-only -fobjc-runtime-has-weak -fobjc-weak -fblocks -Wno-objc-root-class -std=c++11 -Wno-c++0x-extensions -verify %s

@interface AnObject
@property(weak) id value;
@end

__attribute__((objc_arc_weak_reference_unavailable))
@interface NOWEAK : AnObject // expected-note 2 {{class is declared here}}
@end

struct S {
  __weak id a;
};

union U {
  __weak id a;
  S b;
};

void testCast(AnObject *o) {
  __weak id a = reinterpret_cast<__weak NOWEAK *>(o); // expected-error {{class is incompatible with __weak references}} \
                                                      // expected-error {{explicit ownership qualifier on cast result has no effect}} \
                                                      // expected-error {{assignment of a weak-unavailable object to a __weak object}}

  __weak id b = static_cast<__weak NOWEAK *>(o); // expected-error {{class is incompatible with __weak references}} \
                                                 // expected-error {{explicit ownership qualifier on cast result has no effect}} \
                                                 // expected-error {{assignment of a weak-unavailable object to a __weak object}}
}
