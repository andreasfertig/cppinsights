/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_AST_HELPERS_H
#define INSIGHTS_AST_HELPERS_H
//-----------------------------------------------------------------------------

#include <array>
#include <span>
#include <string_view>
#include <vector>

#include "clang/AST/ASTContext.h"
#include "clang/AST/ExprCXX.h"
#include "llvm/ADT/SmallVector.h"

#include "InsightsStrongTypes.h"
//-----------------------------------------------------------------------------

namespace clang::insights::asthelpers {
void ReplaceNode(Stmt* parent, Stmt* oldNode, Stmt* newNode);

using params_vector = std::vector<std::pair<std::string_view, QualType>>;
using params_store  = std::vector<std::pair<std::string, QualType>>;

params_vector to_params_view(params_store& params);

QualType GetRecordDeclType(const CXXMethodDecl* md);
QualType GetRecordDeclType(const RecordDecl* rd);

DeclRefExpr* mkVarDeclRefExpr(std::string_view name, QualType type);

STRONG_BOOL(DoCast);
STRONG_BOOL(AsReference);

CallExpr* CallConstructor(QualType         ctorType,
                          QualType         lhsType,
                          const FieldDecl* fieldDecl,
                          ArrayRef<Expr*>  callParams,
                          DoCast           doCast      = DoCast::No,
                          AsReference      asReference = AsReference::No);

CallExpr* CallConstructor(QualType        ctorType,
                          const VarDecl*  fieldDecl,
                          ArrayRef<Expr*> callParams,
                          DoCast          doCast      = DoCast::No,
                          AsReference     asReference = AsReference::No);

CXXBoolLiteralExpr* Bool(bool b);
CallExpr*           CallDestructor(const VarDecl* fieldDecl);
CXXNewExpr*         New(ArrayRef<Expr*> placementArgs, const Expr* expr, QualType t);
BinaryOperator*     Mul(Expr* lhs, Expr* rhs);
BinaryOperator*     And(VarDecl* lhs, Expr* rhs);
QualType            Typedef(std::string_view name, QualType underlayingType);

SmallVector<Expr*, 5> ArgsToExprVector(const Expr* expr);

///! A helper type to have a container for ArrayRef
struct StmtsContainer
{
    SmallVector<Stmt*, 64> mStmts{};

    StmtsContainer() = default;
    StmtsContainer(std::initializer_list<const Stmt*> stmts)
    {
        for(const auto& stmt : stmts) {
            Add(stmt);
        }
    }

    void clear() { mStmts.clear(); }

    void Add(const Stmt* stmt)
    {
        if(stmt) {
            mStmts.push_back(const_cast<Stmt*>(stmt));
        }
    }

    void AddBodyStmts(Stmt* body);

    operator ArrayRef<Stmt*>() { return mStmts; }
};
//-----------------------------------------------------------------------------

template<typename... Dcls>
DeclStmt* mkDeclStmt(Dcls... dcls)
{
    std::array<Decl*, sizeof...(dcls)> decls{dcls...};

    DeclStmt* _mkDeclStmt(std::span<Decl*> decls);
    return _mkDeclStmt(decls);
}

Stmt*          Comment(std::string_view comment);
VarDecl*       Variable(std::string_view name, QualType type, DeclContext* dc = nullptr);
CXXRecordDecl* Struct(std::string_view name);
ReturnStmt*    Return(Expr* stmt = nullptr);
ReturnStmt*    Return(const ValueDecl* stmt);

CompoundStmt* mkCompoundStmt(ArrayRef<Stmt*> bodyStmts, SourceLocation beginLoc = {}, SourceLocation endLoc = {});
DeclRefExpr*  mkDeclRefExpr(const ValueDecl* vd);
NullStmt*     mkNullStmt();
FieldDecl*    mkFieldDecl(DeclContext* dc, std::string_view name, QualType type);

ArraySubscriptExpr*       ArraySubscript(const Expr* lhs, uint64_t index, QualType type);
MemberExpr*               AccessMember(const Expr* expr, const ValueDecl* vd, bool isArrow = true);
CXXMemberCallExpr*        CallMemberFun(Expr* memExpr, QualType retType);
ImplicitCastExpr*         CastLToRValue(const VarDecl* vd);
FunctionDecl*             Function(std::string_view name, QualType returnType, const params_vector& parameters);
ParmVarDecl*              Parameter(const FunctionDecl* fd, std::string_view name, QualType type);
BinaryOperator*           Assign(DeclRefExpr* declRef, ValueDecl* field, Expr* assignExpr);
BinaryOperator*           Assign(MemberExpr* me, ValueDecl* field, Expr* assignExpr);
BinaryOperator*           Assign(DeclRefExpr* declRef, Expr* assignExpr);
BinaryOperator*           Assign(const VarDecl* var, Expr* assignExpr);
BinaryOperator*           Assign(UnaryOperator* var, Expr* assignExpr);
BinaryOperator*           Assign(Expr* var, Expr* assignExpr);
BinaryOperator*           Equal(Expr* var, Expr* assignExpr);
CXXStaticCastExpr*        StaticCast(QualType toType, const Expr* toExpr, bool makePointer = false);
CXXStaticCastExpr*        CastToVoidFunPtr(std::string_view name);
CXXStaticCastExpr*        Cast(const Expr* toExpr, QualType toType);
IntegerLiteral*           Int32(uint64_t value);
IfStmt*                   If(const Expr* condition, ArrayRef<Stmt*> bodyStmts);
SwitchStmt*               Switch(Expr* stmt);
CaseStmt*                 Case(int value, Stmt* stmt);
BreakStmt*                Break();
LabelStmt*                Label(std::string_view name);
GotoStmt*                 Goto(std::string_view labelName);
UnaryOperator*            Not(const Expr* stmt);
UnaryOperator*            Ref(const Expr* e);
UnaryOperator*            Ref(const ValueDecl* d);
UnaryOperator*            Dref(const Expr* stmt);
UnaryOperator*            AddrOf(const Expr* stmt);
CallExpr*                 Call(const FunctionDecl* fd, ArrayRef<Expr*> params);
CallExpr*                 Call(MemberExpr* fd, ArrayRef<Expr*> params);
CallExpr*                 Call(std::string_view name, ArrayRef<Expr*> args);
CXXTryStmt*               Try(const Stmt* tryBody, CXXCatchStmt* catchAllBody);
CXXCatchStmt*             Catch(Stmt* body);
CXXCatchStmt*             Catch(ArrayRef<Stmt*> body);
CXXThrowExpr*             Throw(const Expr* expr = nullptr);
UnaryExprOrTypeTraitExpr* Sizeof(QualType toType);
QualType                  Ptr(QualType srcType);
CanQualType               VoidTy();

}  // namespace clang::insights::asthelpers

#endif /* INSIGHTS_AST_HELPERS_H */
