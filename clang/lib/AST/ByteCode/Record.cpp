//===--- Record.cpp - struct and class metadata for the VM ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Record.h"
#include "clang/AST/ASTContext.h"
#include "llvm/Support/ErrorHandling.h"

using namespace clang;
using namespace clang::interp;

Record::Record(const RecordDecl *Decl, BaseList &&SrcBases,
               FieldList &&SrcFields, VirtualBaseList &&SrcVirtualBases,
               unsigned VirtualSize, unsigned BaseSize, bool HasPtrField)
    : Decl(Decl), Bases(std::move(SrcBases)), Fields(std::move(SrcFields)),
      VirtualBases(SrcVirtualBases), BaseSize(BaseSize),
      VirtualSize(VirtualSize), IsUnion(Decl->isUnion()),
      IsAnonymousUnion(IsUnion && Decl->isAnonymousStructOrUnion()),
      HasPtrField(HasPtrField) {

  for (Base &V : VirtualBases)
    V.Offset += BaseSize;

  if (isAnonymousUnion()) {
    HasTrivialDtor = true;
  } else {
    const CXXDestructorDecl *Dtor = getDestructor();
    HasTrivialDtor = !Dtor || Dtor->isTrivial();
  }
}

std::string Record::getName() const {
  std::string Ret;
  llvm::raw_string_ostream OS(Ret);
  Decl->getNameForDiagnostic(OS, Decl->getASTContext().getPrintingPolicy(),
                             /*Qualified=*/true);
  return Ret;
}

const Record::Field *Record::findField(unsigned Offset) const {
  if (auto *It = llvm::find_if(
          Fields,
          [=](const Record::Field &F) -> bool { return F.Offset == Offset; });
      It != Fields.end())
    return &*It;
  return nullptr;
}

const Record::Base *Record::getBase(const RecordDecl *RD) const {
  for (const Base &B : Bases) {
    if (B.getDecl() == RD)
      return &B;
  }
  llvm_unreachable("Base should exist");
  return nullptr;
}

const Record::Base *Record::getBaseOrNull(const RecordDecl *RD) const {
  for (const Base &B : Bases) {
    if (B.getDecl() == RD)
      return &B;
  }
  return nullptr;
}

const Record::Base *Record::getBase(QualType T) const {
  if (auto *RD = T->getAsCXXRecordDecl())
    return getBase(RD);
  return nullptr;
}

const Record::Base *Record::findBase(unsigned Offset) const {
  if (auto *It = llvm::find_if(
          Bases,
          [=](const Record::Base &B) -> bool { return B.Offset == Offset; });
      It != Bases.end())
    return &*It;
  return nullptr;
}

const Record::Base *Record::getVirtualBase(const RecordDecl *RD) const {
  for (const Base &B : VirtualBases) {
    if (B.getDecl() == RD)
      return &B;
  }
  return nullptr;
}
