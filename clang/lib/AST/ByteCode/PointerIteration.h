//===--- PointerIteration.h - Pointer iteration utilities -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Defines iteration utilities for Pointer that require Record to be complete.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_INTERP_POINTER_ITERATION_H
#define LLVM_CLANG_AST_INTERP_POINTER_ITERATION_H

#include "Pointer.h"
#include "Record.h"

namespace clang {
namespace interp {

template <typename Func>
void Pointer::forEachSubobject(Func &&Callback) const {
  // Only block pointers have subobjects
  if (!isBlockPointer() || isZero())
    return;

  // Stack-based iteration to avoid recursion overhead
  llvm::SmallVector<Pointer, 32> Worklist;
  Worklist.push_back(*this);

  while (!Worklist.empty()) {
    Pointer P = Worklist.pop_back_val();

    // Call the callback for this subobject
    Callback(P);

    const Descriptor *Desc = P.getFieldDesc();
    if (!Desc)
      continue;

    // Handle arrays
    if (Desc->IsArray && !Desc->isPrimitiveArray()) {
      unsigned NumElems = P.getNumElems();
      // Add elements in reverse order to maintain depth-first left-to-right
      for (unsigned I = NumElems; I > 0; --I) {
        Pointer Elem = P.atIndex(I - 1);
        if (Desc->ElemDesc)
          Worklist.push_back(Elem.narrow());
        else
          Worklist.push_back(Elem);
      }
      continue;
    }

    // Handle records (structs/classes)
    const Record *R = Desc->ElemRecord;
    if (!R)
      continue;

    // Add virtual bases in reverse order
    for (unsigned I = R->getNumVirtualBases(); I > 0; --I) {
      const Record::Base *VB = R->getVirtualBase(I - 1);
      Worklist.push_back(P.atField(VB->Offset));
    }

    // Add bases in reverse order
    for (unsigned I = R->getNumBases(); I > 0; --I) {
      const Record::Base *B = R->getBase(I - 1);
      Worklist.push_back(P.atField(B->Offset));
    }

    // Add fields in reverse order to maintain depth-first left-to-right
    for (unsigned I = R->getNumFields(); I > 0; --I) {
      const Record::Field *F = R->getField(I - 1);
      Worklist.push_back(P.atField(F->Offset));
    }
  }
}

} // namespace interp
} // namespace clang

#endif
