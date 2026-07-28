//===------------- InterpBuiltinObjectSize.cpp ------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Implementation of the frontend part of the __builtin_object_size and
// __builtin_dynamic_object_size builtins.

#include "InterpHelpers.h"
#include "Pointer.h"
#include "Record.h"
#include "clang/AST/RecordLayout.h"

using namespace clang;
using namespace clang::interp;

static bool b = false;

static QualType pointeeOrSelf(const Type *T) {
  if (T->isPointerOrReferenceType())
    return T->getPointeeType();
  return QualType(T, 0);
}

const clang::FieldDecl *getLastField(const RecordDecl *RD) {
  assert(RD);
  if (!RD->getDefinition() || RD->field_empty())
    return nullptr;

  const FieldDecl *LastField = nullptr;
  // Iterate through the singly-linked list of fields
  for (const FieldDecl *F : RD->fields())
    LastField = F;

  return LastField;
}

static QualType computeFieldType(const ASTContext &ASTCtx,
                                 const OpaquePointer &OP) {
  QualType CurType = pointeeOrSelf(OP.ObjectType);
  for (unsigned I = 0; I != OP.PathLength; ++I) {
    const PointerPathEntry &Entry = OP.Path[I];
    switch (Entry.Kind) {
    case PointerPathEntry::Base:
      CurType = ASTCtx.getCanonicalTagType(Entry.RD.getPointer());
      break;
    case PointerPathEntry::Field:
      CurType = Entry.FD->getType();
      break;
    case PointerPathEntry::Array: {
      if (!CurType->isArrayType())
        continue;

      const ArrayType *AT = CurType->getAsArrayTypeUnsafe();
      assert(AT);
      CurType = AT->getElementType();
    }
    }
  }

  return CurType;
}

static std::optional<unsigned> computeFullDescSize(const ASTContext &ASTCtx,
                                                   const Descriptor *Desc) {
  if (Desc->isPrimitive() || Desc->isArray()) {
    return ASTCtx.getTypeSizeInChars(Desc->getType()).getQuantity();
  }

  if (Desc->isRecord()) {
    // Can't use Descriptor::getType() as that may return a pointer type. Look
    // at the decl directly.
    return ASTCtx
        .getTypeSizeInChars(
            ASTCtx.getCanonicalTagType(Desc->ElemRecord->getDecl()))
        .getQuantity();
  }

  return std::nullopt;
}

/// Compute the byte offset of \p Ptr in the full declaration.
static unsigned computePointerOffset(const ASTContext &ASTCtx,
                                     const Pointer &Ptr) {
  if (auto p = Ptr.computeLayoutOffset(ASTCtx))
    return *p;
  return 0;

  unsigned Result = 0;
  Pointer P = Ptr;
  while (P.isField() || P.isArrayElement()) {
    P = P.expand();
    const Descriptor *D = P.getFieldDesc();

    if (P.isArrayElement()) {
      unsigned ElemSize =
          ASTCtx.getTypeSizeInChars(D->getElemQualType()).getQuantity();
      if (P.isOnePastEnd())
        Result += ElemSize * P.getNumElems();
      else
        Result += ElemSize * P.getIndex();
      P = P.expand().getArray();
    } else if (P.isBaseClass()) {
      const auto *RD = cast<CXXRecordDecl>(D->asDecl());
      bool IsVirtual = Ptr.isVirtualBaseClass();
      P = P.getBase();
      const Record *BaseRecord = P.getRecord();

      const ASTRecordLayout &Layout =
          ASTCtx.getASTRecordLayout(cast<CXXRecordDecl>(BaseRecord->getDecl()));
      if (IsVirtual)
        Result += Layout.getVBaseClassOffset(RD).getQuantity();
      else
        Result += Layout.getBaseClassOffset(RD).getQuantity();
    } else if (P.isField()) {
      const FieldDecl *FD = P.getField();
      const ASTRecordLayout &Layout =
          ASTCtx.getASTRecordLayout(FD->getParent());
      unsigned FieldIndex = FD->getFieldIndex();
      uint64_t FieldOffset =
          ASTCtx.toCharUnitsFromBits(Layout.getFieldOffset(FieldIndex))
              .getQuantity();
      Result += FieldOffset;
      P = P.getBase();
    } else
      llvm_unreachable("Unhandled descriptor type");
  }

  return Result;
}

/// Does Ptr point to the last subobject?
static bool pointsToLastObject(const Pointer &Ptr) {
  Pointer P = Ptr;
  while (!P.isRoot()) {

    if (P.isArrayElement()) {
      P = P.expand().getArray();
      continue;
    }
    if (P.isBaseClass()) {
      if (P.getRecord()->getNumFields() > 0)
        return false;
      P = P.getBase();
      continue;
    }

    Pointer Base = P.getBase();
    if (const Record *R = Base.getRecord()) {
      assert(P.getField());
      if (P.getField()->getFieldIndex() != R->getNumFields() - 1)
        return false;
    }
    P = Base;
  }

  return true;
}

/// Does Ptr point to the last object AND to a flexible array member?
static bool isUserWritingOffTheEnd(const ASTContext &Ctx, const Pointer &Ptr,
                                   bool InvalidBase) {
  auto isFlexibleArrayMember = [&](const Descriptor *FieldDesc) {
    using FAMKind = LangOptions::StrictFlexArraysLevelKind;
    FAMKind StrictFlexArraysLevel =
        Ctx.getLangOpts().getStrictFlexArraysLevel();

    if (StrictFlexArraysLevel == FAMKind::Default)
      return true;

    unsigned NumElems = FieldDesc->getNumElems();
    if (NumElems == 0 && StrictFlexArraysLevel != FAMKind::IncompleteOnly)
      return true;

    if (NumElems == 1 && StrictFlexArraysLevel == FAMKind::OneZeroOrIncomplete)
      return true;
    return false;
  };

  const Descriptor *FieldDesc = Ptr.getFieldDesc();
  if (!FieldDesc->isArray())
    return false;

  return InvalidBase && pointsToLastObject(Ptr) &&
         isFlexibleArrayMember(FieldDesc);
}

struct Var {
  QualType T;
  const InitListExpr *Init;
};

Var typeOfClosestSurroundingVariable(const ASTContext &ASTCtx,
                                     const OpaquePointer &OP) {
  if (b)
    llvm::errs() << __PRETTY_FUNCTION__ << '\n';

  if (OP.PathLength == 0)
    return {QualType(OP.ObjectType, 0), nullptr};

  bool IsBaseCast = (OP.PathLength > 0 &&
                     OP.Path[OP.PathLength - 1].Kind == PointerPathEntry::Base);

  if (IsBaseCast) {
    // llvm::errs() << "BaseCast!\n";
    // return {pointeeOrSelf(OP.ObjectType), nullptr};
  }

  if (OP.path().back().Kind != PointerPathEntry::Array)
    return {computeFieldType(ASTCtx, OP), nullptr};

  const VarDecl *Base = OP.asVarDecl();//cast<const VarDecl*>(OP.Base);
  QualType CurType = pointeeOrSelf(OP.ObjectType);
  // assert(ASTCtx.hasSameUnqualifiedType(Init->getType(), CurType));

  QualType ClosestArrayType = CurType;
  const InitListExpr *ClosestArrayInit = nullptr;

  const InitListExpr *CurInit =
      dyn_cast_if_present<InitListExpr>(Base->getInit());
  for (const PointerPathEntry &Entry : OP.path()) {
    // llvm::errs() << "Iteration\n";
    switch (Entry.Kind) {
    case PointerPathEntry::Base:
      CurType = ASTCtx.getCanonicalTagType(Entry.RD.getPointer());
      break;
    case PointerPathEntry::Field:
      // llvm::errs() << "FIELD\n";
      if (CurInit)
        CurInit =
            dyn_cast<InitListExpr>(CurInit->getInit(Entry.FD->getFieldIndex()));
      CurType = Entry.FD->getType();
      break;
    case PointerPathEntry::Array: {
      if (!CurType->isArrayType())
        break;
      ClosestArrayType = CurType;
      ClosestArrayInit = CurInit;
      const ArrayType *AT = CurType->getAsArrayTypeUnsafe();
      assert(AT);
      CurType = AT->getElementType();
    }
    }
  }

  // llvm::errs() << "ClosestArrayInit:\n";
  // ClosestArrayInit->dump();

  // llvm::errs() << "-> END\n";
  // assert(false);
  return {ClosestArrayType, ClosestArrayInit};
}

struct OpaqueArrayData {
  int64_t ElemsRemaining;
  int64_t ElemSize;
};

/// If the pointer points to an array element, retrieve the size and elem size
/// of the array (NOT the element).
static OpaqueArrayData getArrayData(const ASTContext &ASTCtx,
                                    const OpaquePointer &OP) {
  // llvm::errs() << __PRETTY_FUNCTION__ << '\n';
  assert(OP.PathLength > 0);
  QualType CurType = pointeeOrSelf(OP.ObjectType);

  for (const PointerPathEntry &Entry :
       OP.path().drop_back(OP.path().back().Kind == PointerPathEntry::Array)) {
    switch (Entry.Kind) {
    case PointerPathEntry::Base:
      CurType = ASTCtx.getCanonicalTagType(Entry.RD.getPointer());
      break;
    case PointerPathEntry::Field:
      CurType = Entry.FD->getType();
      break;
    case PointerPathEntry::Array: {
      if (CurType->isRecordType())
        break;
      const ArrayType *AT = CurType->getAsArrayTypeUnsafe();
      CurType = AT->getElementType();
    }
    }
  }

  const ArrayType *AT = CurType->getAsArrayTypeUnsafe();
  if (!AT) {
    return {-1, -1};
  }

  int64_t ElemSize =
      ASTCtx.getTypeSizeInChars(AT->getElementType()).getQuantity();

  if (const auto *CAT = dyn_cast<ConstantArrayType>(AT))
    return {static_cast<int64_t>(CAT->getZExtSize()), ElemSize};

  if (isa<IncompleteArrayType>(CurType)) {
    auto V = typeOfClosestSurroundingVariable(ASTCtx, OP);
    if (!V.Init)
      return {0, ElemSize};

    return {V.Init->getNumInits(), ElemSize};
  }

  return {0, 0};
}

static bool isUserWritingOffTheEnd2(const ASTContext &ASTCtx,
                                    const Pointer &Ptr, bool InvalidBase) {
  if (b)
    llvm::errs() << __PRETTY_FUNCTION__ << '\n';

  assert(Ptr.isOpaquePointer());
  const OpaquePointer &OP = Ptr.asOpaquePointer();

  if (OP.PathLength == 0)
    return false;

  QualType CurType = pointeeOrSelf(OP.ObjectType);
  for (unsigned I = 0; I != OP.PathLength; ++I) {
    const PointerPathEntry &Entry = OP.Path[I];
    switch (Entry.Kind) {
    case PointerPathEntry::Base:
      return false;
    case PointerPathEntry::Field: {
      const FieldDecl *FD = OP.Path[I].FD;
      if (!FD->getParent()->isUnion() &&
          FD->getFieldIndex() != FD->getParent()->getNumFields() - 1)
        return false;
      CurType = FD->getType();
    } break;
    case PointerPathEntry::Array: {
      if (I == OP.PathLength - 1)
        break;

      if (!CurType->isArrayType())
        break;

      unsigned Index = OP.Path[I].Index;
      const ArrayType *AT = CurType->getAsArrayTypeUnsafe();
      assert(AT);
      // llvm::errs() << "array type\n";
      if (const auto *CAT = dyn_cast<ConstantArrayType>(AT)) {

        if (Index != CAT->getLimitedSize() - 1)
          return false;
        CurType = CAT->getElementType();
      } else {
        return false;
      }
    }
    }
  }

  if (b) {
    llvm::errs() << "CurType at end:\n";
    CurType->dump();
  }
  // We're pointing to the last field in the full object.
  // CurType is now the most derived type.

  if (!CurType->isArrayType())
    return false;

  if (isa<IncompleteArrayType>(CurType))
    return true;

  using FAMKind = LangOptions::StrictFlexArraysLevelKind;
  FAMKind StrictFlexArraysLevel =
      ASTCtx.getLangOpts().getStrictFlexArraysLevel();

  if (StrictFlexArraysLevel == FAMKind::Default)
    return true;

  auto V = getArrayData(ASTCtx, OP);

  unsigned Index = V.ElemsRemaining;
  if (Index == 0 && StrictFlexArraysLevel != FAMKind::IncompleteOnly)
    return true;

  if (Index == 1 && StrictFlexArraysLevel == FAMKind::OneZeroOrIncomplete)
    return true;
  return false;
}

static std::optional<uint64_t>
computeOpaquePtrOffset(const ASTContext &ASTCtx, const Pointer &Ptr,
                       bool UseClosestSurroundingVariable, bool InvalidBase) {
  if (b)
    llvm::errs() << __PRETTY_FUNCTION__ << '\n';
  const OpaquePointer &OP = Ptr.asOpaquePointer();
  unsigned Offset = 0;

  QualType CurType = pointeeOrSelf(OP.ObjectType);
  for (unsigned I = 0; I != OP.PathLength; ++I) {
    const PointerPathEntry &Entry = OP.Path[I];
    switch (Entry.Kind) {
    case PointerPathEntry::Base: {

      const ASTRecordLayout &Layout =
          ASTCtx.getASTRecordLayout(CurType->getAsRecordDecl());
      Offset += Layout.getBaseClassOffset(cast<CXXRecordDecl>(OP.Path[I].RD.getPointer()))
                    .getQuantity();

      CurType = ASTCtx.getCanonicalTagType(OP.Path[I].RD.getPointer());
    } break;

    case PointerPathEntry::Field: {
      const FieldDecl *FD = OP.Path[I].FD;
      const ASTRecordLayout &Layout =
          ASTCtx.getASTRecordLayout(FD->getParent());
      Offset +=
          ASTCtx.toCharUnitsFromBits(Layout.getFieldOffset(FD->getFieldIndex()))
              .getQuantity();

      CurType = FD->getType();
    } break;
    case PointerPathEntry::Array: {
      unsigned Index = OP.Path[I].Index;
      if (!CurType->isArrayType()) { // CurType->isRecordType()) {
        // llvm::errs() << "array on non-array\n";
        // llvm::errs() << I << " / " << OP.PathLength - 1 << '\n';
        // CurType->dump();
        if (I == 0 && InvalidBase && !isa_and_nonnull<ParmVarDecl>(OP.Base.asVarDecl()))
          return std::nullopt;
        // if (InvalidBase)
        // return std::nullopt;
        if (I == OP.PathLength - 1 && UseClosestSurroundingVariable) {
          return Index * ASTCtx.getTypeSizeInChars(CurType).getQuantity();
        }
        Offset += Index * ASTCtx.getTypeSizeInChars(CurType).getQuantity();
        continue;
      }
      const ArrayType *AT = CurType->getAsArrayTypeUnsafe();
      assert(AT);
      if (I == OP.PathLength - 1 && UseClosestSurroundingVariable) {
        // llvm::errs() << "last path entry is array\n";
        return Index *
               ASTCtx.getTypeSizeInChars(AT->getElementType()).getQuantity();
      }

      if (I == OP.PathLength - 1 && UseClosestSurroundingVariable) {
        // llvm::errs() << "last path entry is array\n";
        return Index *
               ASTCtx.getTypeSizeInChars(AT->getElementType()).getQuantity();
      }

      Offset +=
          Index * ASTCtx.getTypeSizeInChars(AT->getElementType()).getQuantity();
      CurType = AT->getElementType();
    }
    }
  }
  // llvm::errs() << "After loop\n";

  // if (InvalidBase)
  // return std::nullopt;

  QualType Ty = CurType.getNonReferenceType();

  if (Ty->isIncompleteType() || Ty->isFunctionType()) {
    // llvm::errs()<< "AHA!\n";
    // llvm::errs() << "b\n";
    return std::nullopt;
  }

  if (isa<IncompleteArrayType>(CurType))
    return std::nullopt;

  if (UseClosestSurroundingVariable)
    return 0;

  return Offset;
}

static bool pointsToCompleteObject(const Pointer &Ptr,
                                   const ASTContext &ASTCtx) {
  const OpaquePointer &OP = Ptr.asOpaquePointer();

  if (OP.PathLength == 0)
    return true;

  QualType CurType = pointeeOrSelf(OP.ObjectType);
  for (const PointerPathEntry &Entry : OP.path()) {
    switch (Entry.Kind) {
    case PointerPathEntry::Base:
      CurType = ASTCtx.getCanonicalTagType(Entry.RD.getPointer());
      break;
    case PointerPathEntry::Field:
      CurType = Entry.FD->getType();
      break;
    case PointerPathEntry::Array: {
      if (!CurType->isArrayType())
        break;
      const ArrayType *AT = CurType->getAsArrayTypeUnsafe();
      assert(AT);
      if (const auto *CAT = dyn_cast<ConstantArrayType>(AT)) {
        CurType = CAT->getElementType();
      } else {
        // FIXME: Propagate error?
        return false;
      }
    }
    }
  }

  return isa<IncompleteArrayType>(CurType);
}

/// Returns the size of the whole variable the pointer points to, including the
/// size of potential flexible array members at the end.
size_t sizeOfWholeVariable(const ASTContext &ASTCtx, const OpaquePointer &OP) {
  const VarDecl *Base = OP.asVarDecl();//cast<VarDecl>(OP.Base);
  QualType T = Base->getType();
  if (T->isPointerType())
    T = T->getPointeeType();

  size_t TypeSize = ASTCtx.getTypeSizeInChars(T).getQuantity();
  if (!T->isRecordType()) {
    return TypeSize;
  }

  const FieldDecl *LastField = getLastField(T->getAsRecordDecl());
  if (!LastField)
    return TypeSize;

  if (!isa<IncompleteArrayType>(LastField->getType()))
    return TypeSize;

  unsigned Size = 0;
  const Expr *Init = cast<VarDecl>(Base)->getInit();
  if (const auto *ILE = dyn_cast_if_present<InitListExpr>(Init)) {
    const Expr *LastFieldInit = ILE->getInits()[ILE->getNumInits() - 1];
    Size = ASTCtx.getTypeSizeInChars(LastFieldInit->getType()).getQuantity();
  }

  return Size + TypeSize;
}

/// Compute the size of an opaque pointer. Usually, this is simply the size of
/// its type. However, when it's a struct type ending in a flexible array
/// member, we need to check that field's initializer to get the size including
/// the array member.
static unsigned computeOpaqueSize(const ASTContext &ASTCtx, const Pointer &Ptr,
                                  bool UseClosestSurroundingVariable,
                                  bool OffTheEnd) {
  if (b)
    llvm::errs() << __PRETTY_FUNCTION__ << '\n';
  const OpaquePointer &OP = Ptr.asOpaquePointer();

  bool IsBaseCast = (OP.PathLength > 0 &&
                     OP.Path[OP.PathLength - 1].Kind == PointerPathEntry::Base);
  if (b)
    llvm::errs() << "IsBaseCast: " << IsBaseCast << '\n';
  unsigned TypeSize;
  if (UseClosestSurroundingVariable) {
    if (b) {
      llvm::errs() << "type of closest surrounding variable:\n";
      typeOfClosestSurroundingVariable(ASTCtx, OP).T.dump();
    }
    // XXX Clang does not consider base casts. GCC does.

    // FIXME: Base casts on fields (PathLength > 1)?

    if (IsBaseCast && OP.PathLength == 1)
      TypeSize =
          ASTCtx.getTypeSizeInChars(pointeeOrSelf(OP.ObjectType)).getQuantity();
    else {

      auto V = typeOfClosestSurroundingVariable(ASTCtx, OP); //.T.dump();

      TypeSize = ASTCtx.getTypeSizeInChars(V.T).getQuantity();

      if (V.Init)
        TypeSize += ASTCtx.getTypeSizeInChars(V.Init->getType()).getQuantity();

      // TypeSize =
      // ASTCtx.getTypeSizeInChars(computeFieldType(ASTCtx, OP)).getQuantity();
    }

  } else {
    if (b) {
      llvm::errs() << "taking size of object...\n";
    }
    // pointeeOrSelf(OP.ObjectType)->dump();
    TypeSize =
        ASTCtx.getTypeSizeInChars(pointeeOrSelf(OP.ObjectType)).getQuantity();
    // llvm::errs() << "TypeSize: " << TypeSize << '\n';
    // llvm::errs() << "WholeVar: " << sizeOfWholeVariable(ASTCtx, OP) << '\n';
  }

  if (!OffTheEnd)
    return TypeSize;

  if (b)
    llvm::errs() << "TypeSize: " << TypeSize << '\n';

  const VarDecl *Base = OP.asVarDecl();
  if (!Base)
    return TypeSize;

  if (b)
    llvm::errs() << "SIZE WITH BASE\n";

  QualType CurType = pointeeOrSelf(OP.ObjectType);

  if (!CurType->isRecordType())
    return TypeSize;

  // If the last field is an incomplete array type, check if it has been
  // initialized via an InitListExpr. If so, we can take the size from there.
  const VarDecl *BaseVD = cast<VarDecl>(Base);
  if (!BaseVD->hasInit())
    return TypeSize;

  unsigned FlexibleArraySize =
      cast<VarDecl>(Base)->getFlexibleArrayInitChars(ASTCtx).getQuantity();
  return TypeSize + FlexibleArraySize;
}

namespace clang {
namespace interp {
UnsignedOrNone evaluateBuiltinObjectSize(const ASTContext &ASTCtx,
                                         unsigned Kind, Pointer &Ptr,
                                         const Expr *E, bool IsDynamic) {
  if (b) {
    llvm::errs() << __PRETTY_FUNCTION__ << '\n';
    llvm::errs() << Ptr << '\n';
    E->dumpColor();
  }

  if (Ptr.isZero()) {
    return std::nullopt;
  }

  // if (Ptr.isDummy() && Ptr.getType()->isPointerType()) {
    // llvm::errs() << "err2\n";
    // return std::nullopt;
  // }

  // For __builtin_dynamic_object_size on a counted_by-annotated flexible
  // array member, defer to IR generation (emitCountedBySize in CGBuiltin):
  // its runtime computation uses the live 'count' field and is more accurate
  // than the layout/initializer-derived size we'd produce here. Use the same
  // findStructFieldAccess form-recognition CGBuiltin does, so we refuse to
  // fold on exactly the shapes that path handles (and, importantly, *not*
  // on '&af.fam' which designates the array-as-a-whole and stays on the
  // layout-derived path to match GCC). Checked after the negative-offset
  // early return above so that obviously out-of-bounds operands still fold
  // to 0, preserving existing behavior.
  if (IsDynamic) {
    const auto *ME = dyn_cast_if_present<MemberExpr>(findStructFieldAccess(E));
    const auto *FD = ME ? dyn_cast<FieldDecl>(ME->getMemberDecl()) : nullptr;
    if (FD && FD->getType()->isCountAttributedType()) {
      return std::nullopt;
    }
  }

  bool InvalidBase = false;

  if (Ptr.isDummy() && Ptr.getDeclDesc()) {

    if (const VarDecl *VD = Ptr.getDeclDesc()->asVarDecl();
        VD && VD->getType()->isPointerType())
      InvalidBase = true;
  }

  bool UseFieldDesc = (Kind & 1u);
  bool ReportMinimum = (Kind & 2u);
  bool UseClosestSurroundingVariable = (Kind == 1) || (Kind == 3);

  if (Ptr.isOpaquePointer()) {
    const OpaquePointer &OP = Ptr.asOpaquePointer();
    if (b)
      llvm::errs() << "------------ OPAQUE BOS\n";
    InvalidBase = Ptr.asOpaquePointer().ObjectType->isPointerType();
    bool DetermineForCompleteObject = pointsToCompleteObject(Ptr, ASTCtx);

    if (b) {
      llvm::errs() << "DetermineForCompleteObject: "
                   << DetermineForCompleteObject << '\n';
      llvm::errs() << "UseFieldDesc: " << UseFieldDesc << '\n';
      llvm::errs() << "ReportMinimum: " << ReportMinimum << '\n';
      llvm::errs() << "InvalidBase: " << InvalidBase << '\n';
      llvm::errs() << "WOTE: " << isUserWritingOffTheEnd2(ASTCtx, Ptr, false)
                   << '\n';
      llvm::errs() << "UseClosestSurroundingVariable: "
                   << UseClosestSurroundingVariable << '\n';
    }

    // if (InvalidBase && Kind != 3)
    // return std::nullopt;

    if (!UseFieldDesc || DetermineForCompleteObject) {
      if (InvalidBase) {
        if (b)
          llvm::errs() << "err4\n";
        return std::nullopt;
      }
    }

    unsigned FullSize =
        computeOpaqueSize(ASTCtx, Ptr, UseClosestSurroundingVariable,
                          isUserWritingOffTheEnd2(ASTCtx, Ptr, false) ||
                              DetermineForCompleteObject);
    if (b) {
      auto c = FullSize;
      llvm::errs() << "COMPUTED SIZE: " << c << '\n';
      llvm::errs() << "FullSize = " << c << " (was " << FullSize << ")\n";
    }

    uint64_t ElemSize = FullSize;
    uint64_t ElemsRemaining = 1;

    if (UseClosestSurroundingVariable && OP.PathLength > 0 &&
        OP.path().back().Kind == PointerPathEntry::Array) {
      auto D = getArrayData(ASTCtx, OP);
      ElemsRemaining = D.ElemsRemaining;
      ElemSize = D.ElemSize;
    }

    uint64_t Offset;
    if (std::optional<uint64_t> O = computeOpaquePtrOffset(
            ASTCtx, Ptr, UseClosestSurroundingVariable, InvalidBase)) {
      Offset = *O;
      if (b)
        llvm::errs() << "Computed offset: " << Offset << '\n';
    } else {
      if (b)
        llvm::errs() << "computeOpaquePtrOffset failed!\n";
      return std::nullopt;
    }

    if (b) {
      llvm::errs() << "ElemsRemaining: " << ElemsRemaining << '\n';
      llvm::errs() << "ElemSize: " << ElemSize << '\n';
      llvm::errs() << "FullSize: " << FullSize << '\n';
      llvm::errs() << (ElemSize * ElemsRemaining) << " - " << Offset << '\n';

      llvm::errs() << "Pointer byte offset: " << Ptr.getByteOffset() << '\n';
    }

    if (Offset > (ElemSize * ElemsRemaining)) {
      if (b) {
        llvm::errs() << Offset << " > " << (ElemSize * ElemsRemaining) << "!\n";
      }
      return 0u;
    }

    if (InvalidBase && isUserWritingOffTheEnd2(ASTCtx, Ptr, InvalidBase)) {
      if (Kind == 1)
        return std::nullopt;
    }

    assert(Offset <= (ElemSize * ElemsRemaining));

    return static_cast<unsigned>(((ElemSize * ElemsRemaining) - Offset));
  }

  // ----------------------------------------------------------------------------------------------------

  if (Ptr.isZero() || !Ptr.isBlockPointer())
    return std::nullopt;

  // According to the GCC documentation, we want the size of the subobject
  // denoted by the pointer. But that's not quite right -- what we actually
  // want is the size of the immediately-enclosing array, if there is one.
  if (Ptr.isArrayElement())
    Ptr = Ptr.expand();

  bool DetermineForCompleteObject = Ptr.getFieldDesc() == Ptr.getDeclDesc();
  const Descriptor *DeclDesc = Ptr.getDeclDesc();
  assert(DeclDesc);

  if (!UseFieldDesc || DetermineForCompleteObject) {
    // Can't read beyond the pointer decl desc.
    if (!ReportMinimum && DeclDesc->getDataType(ASTCtx)->isPointerType()) {
      llvm::errs() << "err3\n";
      return std::nullopt;
    }

    if (InvalidBase) {
      llvm::errs() << "err4\n";
      return std::nullopt;
    }
  } else {
    if (isUserWritingOffTheEnd(ASTCtx, Ptr, InvalidBase)) {
      // If we cannot determine the size of the initial allocation, then we
      // can't given an accurate upper-bound. However, we are still able to give
      // conservative lower-bounds for Type=3.
      if (Kind == 1) {
        llvm::errs() << "err5\n";
        return std::nullopt;
      }
    }
    // For Type=1, defer to the runtime path on a true incomplete-array
    // flexible array member (e.g. 'char fam[]') even when the base is a
    // concrete local/global. Without this, the bytecode interpreter would
    // happily fold &af.fam to 'NumElems * elemSize = 0' below; the default
    // const-evaluator avoids the same trap, and CGBuiltin emits
    // @llvm.objectsize for the correct layout-derived answer (matching
    // GCC's __bos/__bdos on '&af.fam').
    if (Kind == 1 && pointsToLastObject(Ptr) && Ptr.getFieldDesc()->isArray() &&
        Ptr.getFieldDesc()->getType()->isIncompleteArrayType()) {
      // llvm::errs() << "err6\n";
      return std::nullopt;
    }
  }

  // The "closest surrounding subobject" is NOT a base class,
  // so strip the base class casts.
  if (UseFieldDesc && Ptr.isBaseClass())
    Ptr = Ptr.stripBaseCasts();

  const Descriptor *Desc = UseFieldDesc ? Ptr.getFieldDesc() : DeclDesc;
  assert(Desc);

  std::optional<unsigned> FullSize = computeFullDescSize(ASTCtx, Desc);
  if (!FullSize) {
    llvm::errs() << "err7\n";
    return std::nullopt;
  }

  // llvm::errs() << "FullSize: " << *FullSize << '\n';

  unsigned ByteOffset;
  if (UseFieldDesc) {
    if (Ptr.isBaseClass()) {
      assert(computePointerOffset(ASTCtx, Ptr.getBase()) <=
             computePointerOffset(ASTCtx, Ptr));
      ByteOffset = computePointerOffset(ASTCtx, Ptr.getBase()) -
                   computePointerOffset(ASTCtx, Ptr);
    } else {
      if (Ptr.inArray())
        ByteOffset =
            computePointerOffset(ASTCtx, Ptr) -
            computePointerOffset(ASTCtx, Ptr.expand().atIndex(0).narrow());
      else {
        // llvm::errs() << "Offset is just 0 :(\n";
        ByteOffset = Ptr.isOnePastEnd() ? *FullSize : 0; // 0;
        // ByteOffset = computePointerOffset(ASTCtx, Ptr);
      }
    }
  } else
    ByteOffset = computePointerOffset(ASTCtx, Ptr);

  if (b) {
    llvm::errs() << *FullSize << " - " << ByteOffset << '\n';
    assert(ByteOffset <= *FullSize);
    llvm::errs() << "RESULT: " << (*FullSize - ByteOffset) << '\n';
  }
  return *FullSize - ByteOffset;
}
} // namespace interp
} // namespace clang
