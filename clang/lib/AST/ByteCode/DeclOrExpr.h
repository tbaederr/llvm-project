

#include "llvm/ADT/PointerUnion.h"

namespace clang {
class VarDecl;
class Expr;
namespace interp {

struct DeclOrExpr {
  llvm::PointerUnion<const VarDecl *, const Expr *> V;

  DeclOrExpr() : V(nullptr) {}
  DeclOrExpr(std::nullptr_t) : V(nullptr) {}
  DeclOrExpr(const VarDecl *VD) : V(VD) {}
  DeclOrExpr(const Expr *E) : V(E) {}

  bool isExpr() const { return isa<const Expr *>(V); }
  const Expr *asExpr() const { return V.dyn_cast<const Expr *>(); }
  bool isVarDecl() const { return isa<const VarDecl *>(V); }
  const VarDecl *asVarDecl() const { return V.dyn_cast<const VarDecl *>(); }

  template<typename T>
  bool isParmVarDecl() const {
    return isa<T>(V);
  }

  const void *getOpaqueValue() const { return V.getOpaqueValue(); }

  bool operator==(const DeclOrExpr &O) const {
    return O.V == V;
  }
  bool operator!=(const DeclOrExpr &O) const {
    return O.V != V;
  }
  explicit operator bool() const { return !V.isNull(); }

  QualType getType() const {
    if (const auto *VD = asVarDecl())
      return VD->getType();
    return asExpr()->getType();
  }



};
static_assert(sizeof(DeclOrExpr) == sizeof(void*));

inline DeclOrExpr getSwappedBytes(DeclOrExpr F) { return F;}

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, DeclOrExpr DOE) {
  return OS;
}



}
}
