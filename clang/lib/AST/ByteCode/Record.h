//===--- Record.h - struct and class metadata for the VM --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// A record is part of a program to describe the layout and methods of a struct.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_INTERP_RECORD_H
#define LLVM_CLANG_AST_INTERP_RECORD_H

#include "Descriptor.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"

namespace clang {
namespace interp {
class Program;

/// Structure/Class descriptor.
class Record final {
public:
  /// Describes a record field.
  struct Field {
    const Descriptor *Desc;
    unsigned Offset;

    Field(const Descriptor *Desc, unsigned Offset)
        : Desc(Desc), Offset(Offset) {}

    bool isBitField() const { return getDecl()->isBitField(); }
    bool isUnnamedBitField() const { return getDecl()->isUnnamedBitField(); }
    unsigned bitWidth() const {
      assert(isBitField());
      return getDecl()->getBitWidthValue();
    }
    const FieldDecl *getDecl() const { return Desc->asFieldDecl(); }
  };

  /// Describes a base class.
  struct Base {
    const Descriptor *Desc;
    unsigned Offset;

    Base(const Descriptor *Desc, unsigned Offset)
        : Desc(Desc), Offset(Offset) {}

    const CXXRecordDecl *getDecl() const {
      return cast<CXXRecordDecl>(Desc->asRecordDecl());
    }
    const Record *getRecord() const { return Desc->ElemRecord; }
  };

  /// Mapping from identifiers to field descriptors.
  using FieldList = llvm::SmallVector<Field, 8>;
  /// Mapping from identifiers to base classes.
  using BaseList = llvm::SmallVector<Base, 2>;
  /// List of virtual base classes.
  using VirtualBaseList = llvm::SmallVector<Base, 0>;

public:
  /// Returns the underlying declaration.
  const RecordDecl *getDecl() const { return Decl; }
  /// Returns the name of the underlying declaration.
  std::string getName() const;
  /// Checks if the record is a union.
  bool isUnion() const { return IsUnion; }
  /// Checks if the record is an anonymous union.
  bool isAnonymousUnion() const { return IsAnonymousUnion; }
  /// Returns the size of the record.
  unsigned getSize() const { return BaseSize; }
  /// Returns the full size of the record, including records.
  unsigned getFullSize() const { return BaseSize + VirtualSize; }
  /// Returns the destructor of the record, if any.
  const CXXDestructorDecl *getDestructor() const {
    if (const auto *CXXDecl = dyn_cast<CXXRecordDecl>(Decl))
      return CXXDecl->getDestructor();
    return nullptr;
  }
  /// If this record (or any of its bases) contains a field of type PT_Ptr.
  bool hasPtrField() const { return HasPtrField; }

  /// Returns true for anonymous unions and records
  /// with no destructor or for those with a trivial destructor.
  bool hasTrivialDtor() const { return HasTrivialDtor; }

  using const_field_iter = FieldList::const_iterator;
  llvm::iterator_range<const_field_iter> fields() const {
    return llvm::make_range(Fields.begin(), Fields.end());
  }

  unsigned getNumFields() const { return Fields.size(); }
  const Field *getField(unsigned I) const { return &Fields[I]; }
  /// Find a field with the given offset.
  /// This does a linear search, so use sparingly.
  const Field *findField(unsigned Offset) const;
  /// Returns a field.
  const Field *getField(const FieldDecl *FD) const {
    return &Fields[FD->getFieldIndex()];
  }

  using const_base_iter = BaseList::const_iterator;
  llvm::iterator_range<const_base_iter> bases() const {
    return llvm::make_range(Bases.begin(), Bases.end());
  }

  unsigned getNumBases() const { return Bases.size(); }
  const Base *getBase(unsigned I) const {
    assert(I < getNumBases());
    return &Bases[I];
  }
  /// Returns a base descriptor.
  const Base *getBase(QualType T) const;
  /// Returns a base descriptor.
  const Base *getBase(const RecordDecl *RD) const;
  const Base *getBaseOrNull(const RecordDecl *RD) const;
  const Base *findBase(unsigned Offset) const;

  using const_virtual_iter = VirtualBaseList::const_iterator;
  llvm::iterator_range<const_virtual_iter> virtual_bases() const {
    return llvm::make_range(VirtualBases.begin(), VirtualBases.end());
  }

  unsigned getNumVirtualBases() const { return VirtualBases.size(); }
  const Base *getVirtualBase(unsigned I) const { return &VirtualBases[I]; }
  /// Returns a virtual base descriptor.
  const Base *getVirtualBase(const RecordDecl *RD) const;

  void dump(llvm::raw_ostream &OS, unsigned Indentation = 0,
            unsigned Offset = 0) const;
  void dump() const { dump(llvm::errs()); }

private:
  /// Constructor used by Program to create record descriptors.
  Record(const RecordDecl *, BaseList &&Bases, FieldList &&Fields,
         VirtualBaseList &&VirtualBases, unsigned VirtualSize,
         unsigned BaseSize, bool HasPtrField = true);

private:
  friend class Program;

  /// Original declaration.
  const RecordDecl *Decl;
  /// List of all base classes.
  BaseList Bases;
  /// List of all the fields in the record.
  FieldList Fields;
  /// List of all virtual bases.
  VirtualBaseList VirtualBases;

  /// Size of the structure.
  unsigned BaseSize;
  /// Size of all virtual bases.
  unsigned VirtualSize;
  /// If this record is a union.
  bool IsUnion;
  /// If this is an anonymous union.
  bool IsAnonymousUnion;
  /// If any of the fields are pointers (or references).
  bool HasPtrField = false;
  bool HasTrivialDtor = false;
};

} // namespace interp
} // namespace clang

#endif
