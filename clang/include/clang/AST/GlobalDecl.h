//===- GlobalDecl.h - Global declaration holder -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// A GlobalDecl can hold either a regular variable/function or a C++ ctor/dtor
// together with its type.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_GLOBALDECL_H
#define LLVM_CLANG_AST_GLOBALDECL_H

#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/DeclOpenMP.h"
#include "clang/Basic/ABI.h"
#include "clang/Basic/LLVM.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/type_traits.h"
#include <cassert>

namespace clang {

enum class DynamicInitKind : unsigned {
  NoStub = 0,
  Initializer,
  AtExit,
  GlobalArrayDestructor
};

/// GlobalDecl - represents a global declaration. This can either be a
/// CXXConstructorDecl and the constructor type (Base, Complete).
/// a CXXDestructorDecl and the destructor type (Base, Complete) or
/// a VarDecl, a FunctionDecl or a BlockDecl.
class GlobalDecl {
  llvm::PointerIntPair<const Decl *, 2> Value;
  unsigned MultiVersionIndex = 0;

  void Init(const Decl *D) {
    assert(!isa<CXXConstructorDecl>(D) && "Use other ctor with ctor decls!");
    assert(!isa<CXXDestructorDecl>(D) && "Use other ctor with dtor decls!");

    Value.setPointer(D);
  }

public:
  GlobalDecl() = default;
  GlobalDecl(const VarDecl *D) { Init(D);}
  GlobalDecl(const FunctionDecl *D, unsigned MVIndex = 0)
      : MultiVersionIndex(MVIndex) {
    Init(D);
  }
  GlobalDecl(const BlockDecl *D) { Init(D); }
  GlobalDecl(const CapturedDecl *D) { Init(D); }
  GlobalDecl(const ObjCMethodDecl *D) { Init(D); }
  GlobalDecl(const OMPDeclareReductionDecl *D) { Init(D); }
  GlobalDecl(const OMPDeclareMapperDecl *D) { Init(D); }
  GlobalDecl(const CXXConstructorDecl *D, CXXCtorType Type) : Value(D, Type) {}
  GlobalDecl(const CXXDestructorDecl *D, CXXDtorType Type) : Value(D, Type) {}
  GlobalDecl(const VarDecl *D, DynamicInitKind StubKind)
      : Value(D, unsigned(StubKind)) {}

  GlobalDecl getCanonicalDecl() const {
    GlobalDecl CanonGD;
    CanonGD.Value.setPointer(Value.getPointer()->getCanonicalDecl());
    CanonGD.Value.setInt(Value.getInt());
    CanonGD.MultiVersionIndex = MultiVersionIndex;

    return CanonGD;
  }

  const Decl *getDecl() const { return Value.getPointer(); }

  CXXCtorType getCtorType() const {
    assert(isa<CXXConstructorDecl>(getDecl()) && "Decl is not a ctor!");
    return static_cast<CXXCtorType>(Value.getInt());
  }

  CXXDtorType getDtorType() const {
    assert(isa<CXXDestructorDecl>(getDecl()) && "Decl is not a dtor!");
    return static_cast<CXXDtorType>(Value.getInt());
  }

  DynamicInitKind getDynamicInitKind() const {
    assert(isa<VarDecl>(getDecl()) &&
           cast<VarDecl>(getDecl())->hasGlobalStorage() &&
           "Decl is not a global variable!");
    return static_cast<DynamicInitKind>(Value.getInt());
  }

  unsigned getMultiVersionIndex() const {
    assert(isa<FunctionDecl>(getDecl()) &&
           !isa<CXXConstructorDecl>(getDecl()) &&
           !isa<CXXDestructorDecl>(getDecl()) &&
           "Decl is not a plain FunctionDecl!");
    return MultiVersionIndex;
  }

  friend bool operator==(const GlobalDecl &LHS, const GlobalDecl &RHS) {
    return LHS.Value == RHS.Value &&
           LHS.MultiVersionIndex == RHS.MultiVersionIndex;
  }

  void *getAsOpaquePtr() const { return Value.getOpaqueValue(); }

  static GlobalDecl getFromOpaquePtr(void *P) {
    GlobalDecl GD;
    GD.Value.setFromOpaqueValue(P);
    return GD;
  }

  GlobalDecl getWithDecl(const Decl *D) {
    GlobalDecl Result(*this);
    Result.Value.setPointer(D);
    return Result;
  }

  GlobalDecl getWithCtorType(CXXCtorType Type) {
    assert(isa<CXXConstructorDecl>(getDecl()));
    GlobalDecl Result(*this);
    Result.Value.setInt(Type);
    return Result;
  }

  GlobalDecl getWithDtorType(CXXDtorType Type) {
    assert(isa<CXXDestructorDecl>(getDecl()));
    GlobalDecl Result(*this);
    Result.Value.setInt(Type);
    return Result;
  }

  GlobalDecl getWithMultiVersionIndex(unsigned Index) {
    assert(isa<FunctionDecl>(getDecl()) &&
           !isa<CXXConstructorDecl>(getDecl()) &&
           !isa<CXXDestructorDecl>(getDecl()) &&
           "Decl is not a plain FunctionDecl!");
    GlobalDecl Result(*this);
    Result.MultiVersionIndex = Index;
    return Result;
  }
};

} // namespace clang

namespace llvm {

  template<> struct DenseMapInfo<clang::GlobalDecl> {
    static inline clang::GlobalDecl getEmptyKey() {
      return clang::GlobalDecl();
    }

    static inline clang::GlobalDecl getTombstoneKey() {
      return clang::GlobalDecl::
        getFromOpaquePtr(reinterpret_cast<void*>(-1));
    }

    static unsigned getHashValue(clang::GlobalDecl GD) {
      return DenseMapInfo<void*>::getHashValue(GD.getAsOpaquePtr());
    }

    static bool isEqual(clang::GlobalDecl LHS,
                        clang::GlobalDecl RHS) {
      return LHS == RHS;
    }
  };

} // namespace llvm

#endif // LLVM_CLANG_AST_GLOBALDECL_H
