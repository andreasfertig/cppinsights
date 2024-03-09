/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include <algorithm>

#include "ASTHelpers.h"
#include "CodeGenerator.h"
#include "Insights.h"
#include "InsightsHelpers.h"
#include "InsightsStaticStrings.h"
#include "InsightsStrCat.h"
//-----------------------------------------------------------------------------

using namespace std::literals;

namespace clang::insights::asthelpers {
DeclStmt* _mkDeclStmt(std::span<Decl*> decls)
{
    const auto& ctx = GetGlobalAST();

    auto dgRef = DeclGroupRef::Create(const_cast<ASTContext&>(ctx), decls.data(), decls.size());

    return new(ctx) DeclStmt(dgRef, {}, {});
}
//-----------------------------------------------------------------------------

BinaryOperator* Assign(const VarDecl* var, Expr* assignExpr)
{
    return Assign(mkDeclRefExpr(var), assignExpr);
}
//-----------------------------------------------------------------------------

static BinaryOperator* mkBinaryOperator(Expr* lhs, Expr* rhs, BinaryOperator::Opcode opc, QualType resType)
{
    return BinaryOperator::Create(
        GetGlobalAST(), lhs, rhs, opc, resType.getNonReferenceType(), VK_LValue, OK_Ordinary, {}, {});
}
//-----------------------------------------------------------------------------

BinaryOperator* Assign(UnaryOperator* var, Expr* assignExpr)
{
    return mkBinaryOperator(var, assignExpr, BO_Assign, assignExpr->getType());
}
//-----------------------------------------------------------------------------

BinaryOperator* Assign(MemberExpr* me, ValueDecl* field, Expr* assignExpr)
{
    return mkBinaryOperator(me, assignExpr, BO_Assign, field->getType());
}
//-----------------------------------------------------------------------------

BinaryOperator* Assign(DeclRefExpr* declRef, ValueDecl* field, Expr* assignExpr)
{
    return Assign(AccessMember(declRef, field), field, assignExpr);
}
//-----------------------------------------------------------------------------

BinaryOperator* Assign(DeclRefExpr* declRef, Expr* assignExpr)
{
    return mkBinaryOperator(declRef, assignExpr, BO_Assign, declRef->getType());
}
//-----------------------------------------------------------------------------

BinaryOperator* Assign(Expr* var, Expr* assignExpr)
{
    return mkBinaryOperator(var, assignExpr, BO_Assign, assignExpr->getType());
}
//-----------------------------------------------------------------------------

static UnaryOperator* mkUnaryOperator(const Expr* stmt, UnaryOperatorKind kind, QualType type)
{
    return UnaryOperator::Create(
        GetGlobalAST(), const_cast<Expr*>(stmt), kind, type, VK_PRValue, OK_Ordinary, {}, false, {});
}
//-----------------------------------------------------------------------------

UnaryOperator* Not(const Expr* stmt)
{
    return mkUnaryOperator(stmt, UO_LNot, stmt->getType());
}
//-----------------------------------------------------------------------------

UnaryOperator* Not(const VarDecl* stmt)
{
    return mkUnaryOperator(mkDeclRefExpr(stmt), UO_LNot, stmt->getType());
}
//-----------------------------------------------------------------------------

static UnaryOperator* mkReference(Expr* e, QualType t)
{
    return mkUnaryOperator(e, UO_AddrOf, t);
}
//-----------------------------------------------------------------------------

UnaryOperator* Ref(const Expr* e)
{
    return mkReference(const_cast<Expr*>(e), e->getType());
}
//-----------------------------------------------------------------------------

UnaryOperator* Ref(const ValueDecl* d)
{
    return Ref(mkDeclRefExpr(d));
}
//-----------------------------------------------------------------------------

UnaryOperator* Dref(const Expr* stmt)
{
    return mkUnaryOperator(stmt, UO_Deref, stmt->getType());
}
//-----------------------------------------------------------------------------

UnaryOperator* AddrOf(const Expr* stmt)
{
    return mkUnaryOperator(stmt, UO_AddrOf, stmt->getType());
}
//-----------------------------------------------------------------------------

CallExpr* Call(const FunctionDecl* fd, ArrayRef<Expr*> params)
{
    return CallExpr::Create(GetGlobalAST(), mkDeclRefExpr(fd), params, fd->getType(), VK_LValue, {}, {});
}
//-----------------------------------------------------------------------------

CallExpr* Call(std::string_view name, ArrayRef<Expr*> args)
{
    params_vector params{};
    params.reserve(args.size());

    for(const auto& param : args) {
        params.emplace_back("dummy"sv, param->getType());
    }

    auto* freeFd = Function(name, VoidTy(), params);

    return Call(freeFd, args);
}
//-----------------------------------------------------------------------------

CXXTryStmt* Try(const Stmt* tryBody, CXXCatchStmt* catchAllBody)
{
    return CXXTryStmt::Create(
        GetGlobalAST(), {}, const_cast<CompoundStmt*>(dyn_cast_or_null<CompoundStmt>(tryBody)), {catchAllBody});
}
//-----------------------------------------------------------------------------

CXXCatchStmt* Catch(ArrayRef<Stmt*> body)
{
    CompoundStmt* compStmt = mkCompoundStmt(body);

    return new(GetGlobalAST()) CXXCatchStmt({}, nullptr, compStmt);
}
//-----------------------------------------------------------------------------

CXXCatchStmt* Catch(Stmt* body)
{
    return Catch(ArrayRef<Stmt*>{body});
}
//-----------------------------------------------------------------------------

CXXThrowExpr* Throw(const Expr* expr)
{
    return new(GetGlobalAST()) CXXThrowExpr(const_cast<Expr*>(expr), VoidTy(), {}, false);
}
//-----------------------------------------------------------------------------

UnaryExprOrTypeTraitExpr* Sizeof(QualType toType)
{
    const auto& ctx = GetGlobalAST();
    return new(ctx) UnaryExprOrTypeTraitExpr(UETT_SizeOf, ctx.getTrivialTypeSourceInfo(toType), toType, {}, {});
}
//-----------------------------------------------------------------------------

CXXStaticCastExpr* Cast(const Expr* toExpr, QualType toType)
{
    return StaticCast(toType, const_cast<Expr*>(toExpr), false);
}
//-----------------------------------------------------------------------------

QualType Ptr(QualType srcType)
{
    return GetGlobalAST().getPointerType(srcType);
}
//-----------------------------------------------------------------------------

CanQualType VoidTy()
{
    return GetGlobalAST().VoidTy;
}
//-----------------------------------------------------------------------------

static QualType mkAnonVoidFunctionPointer()
{
    auto voidPtr = Ptr(VoidTy());

    params_store params{{""s, voidPtr}};

    return Ptr(Function(""sv, voidPtr, to_params_view(params))->getType());
}
//-----------------------------------------------------------------------------

static FunctionDecl* CreateFunctionDecl(std::string_view funcName, params_vector params)
{
    return Function(funcName, VoidTy(), params);
}
//-----------------------------------------------------------------------------

CXXStaticCastExpr* CastToVoidFunPtr(std::string_view name)
{
    return Cast(mkDeclRefExpr(CreateFunctionDecl(name, {})), mkAnonVoidFunctionPointer());
}
//-----------------------------------------------------------------------------

CXXStaticCastExpr* StaticCast(QualType toType, const Expr* toExpr, bool makePointer)
{
    auto& ctx = GetGlobalAST();

    QualType sourceInfoToType = makePointer ? Ptr(toType) : toType;

    return CXXStaticCastExpr::Create(ctx,
                                     toType,
                                     VK_LValue,
                                     CK_DerivedToBase,
                                     const_cast<Expr*>(toExpr),
                                     nullptr,
                                     ctx.getTrivialTypeSourceInfo(sourceInfoToType),
                                     {},
                                     {},
                                     {},
                                     {});
}
//-----------------------------------------------------------------------------

auto* mkLabelDecl(std::string_view name)
{
    auto& ctx = GetGlobalAST();

    return LabelDecl::Create(const_cast<ASTContext&>(ctx), ctx.getTranslationUnitDecl(), {}, &ctx.Idents.get(name));
}
//-----------------------------------------------------------------------------

LabelStmt* Label(std::string_view name)
{
    return new(GetGlobalAST()) LabelStmt({}, mkLabelDecl(name), nullptr);
}
//-----------------------------------------------------------------------------

CompoundStmt* mkCompoundStmt(ArrayRef<Stmt*> bodyStmts, SourceLocation beginLoc, SourceLocation endLoc)
{
    return CompoundStmt::Create(GetGlobalAST(), bodyStmts, FPOptionsOverride{}, beginLoc, endLoc);
}
//-----------------------------------------------------------------------------

IfStmt* If(const Expr* condition, ArrayRef<Stmt*> bodyStmts)
{
    return IfStmt::Create(GetGlobalAST(),
                          {},
                          IfStatementKind::Ordinary,
                          nullptr,
                          nullptr,
                          const_cast<Expr*>(condition),
                          {},
                          {},
                          mkCompoundStmt(bodyStmts),
                          {},
                          /*else*/ nullptr);
}
//-----------------------------------------------------------------------------

IntegerLiteral* Int32(uint64_t value)
{
    auto&       ctx = GetGlobalAST();
    llvm::APInt v{32, value, true};

    return IntegerLiteral::Create(ctx, v, ctx.IntTy, {});
}
//-----------------------------------------------------------------------------

MemberExpr* AccessMember(const Expr* expr, const ValueDecl* vd, bool isArrow)
{
    return MemberExpr::CreateImplicit(GetGlobalAST(),
                                      const_cast<Expr*>(expr),
                                      isArrow,
                                      const_cast<ValueDecl*>(vd),
                                      vd->getType().getNonReferenceType(),
                                      VK_LValue,
                                      OK_Ordinary);
}
//-----------------------------------------------------------------------------

BinaryOperator* Equal(Expr* var, Expr* assignExpr)
{
    return mkBinaryOperator(var, assignExpr, BO_EQ, GetGlobalAST().BoolTy);
}
//-----------------------------------------------------------------------------

GotoStmt* Goto(std::string_view labelName)
{
    return new(GetGlobalAST()) GotoStmt(mkLabelDecl(labelName), {}, {});
}
//-----------------------------------------------------------------------------

ParmVarDecl* Parameter(const FunctionDecl* fd, std::string_view name, QualType type)
{
    auto& ctx = GetGlobalAST();

    return ParmVarDecl::Create(const_cast<ASTContext&>(ctx),
                               const_cast<FunctionDecl*>(fd),
                               {},
                               {},
                               &ctx.Idents.get(name),
                               type,
                               nullptr,
                               SC_None,
                               nullptr);
}
//-----------------------------------------------------------------------------

static auto*
FunctionBase(std::string_view name, QualType returnType, const params_vector& parameters, DeclContext* declCtx)
{
    auto&                    ctx = GetGlobalAST();
    SmallVector<QualType, 8> argTypes{};

    for(const auto& [_, type] : parameters) {
        argTypes.push_back(type);
    }

    FunctionDecl* fdd = FunctionDecl::Create(
        const_cast<ASTContext&>(ctx),
        declCtx,
        {},
        {},
        &ctx.Idents.get(name),
        ctx.getFunctionType(returnType, ArrayRef<QualType>{argTypes}, FunctionProtoType::ExtProtoInfo{}),
        nullptr,
        SC_None,  // SC_Static,
        false,
        false,
        false,
        ConstexprSpecKind::Unspecified,
        nullptr);
    fdd->setImplicit(true);

    SmallVector<ParmVarDecl*, 8> paramVarDecls{};

    for(const auto& [name, type] : parameters) {
        ParmVarDecl* param = Parameter(fdd, name, type);
        param->setScopeInfo(0, 0);
        paramVarDecls.push_back(param);
    }

    fdd->setParams(paramVarDecls);

    return fdd;
}

FunctionDecl* Function(std::string_view name, QualType returnType, const params_vector& parameters)
{
    return FunctionBase(name, returnType, parameters, GetGlobalAST().getTranslationUnitDecl());
}
//-----------------------------------------------------------------------------

auto* mkStdFunctionDecl(std::string_view name, QualType returnType, const params_vector& parameters)
{
    auto&          ctx   = GetGlobalAST();
    NamespaceDecl* stdNs = NamespaceDecl::Create(const_cast<ASTContext&>(ctx),
                                                 ctx.getTranslationUnitDecl(),
                                                 false,
                                                 {},
                                                 {},
                                                 &ctx.Idents.get("std"),
                                                 nullptr,
                                                 false);

    return FunctionBase(name, returnType, parameters, stdNs);
}
//-----------------------------------------------------------------------------

DeclRefExpr* mkDeclRefExpr(const ValueDecl* vd)
{
    return DeclRefExpr::Create(GetGlobalAST(),
                               NestedNameSpecifierLoc{},
                               SourceLocation{},
                               const_cast<ValueDecl*>(vd),
                               false,
                               SourceLocation{},
                               vd->getType(),
                               VK_LValue,
                               nullptr,
                               nullptr,
                               NOUR_None);
}
//-----------------------------------------------------------------------------

ImplicitCastExpr* CastLToRValue(const VarDecl* vd)
{
    return ImplicitCastExpr::Create(
        GetGlobalAST(), vd->getType(), CK_LValueToRValue, mkDeclRefExpr(vd), {}, VK_LValue, {});
}
//-----------------------------------------------------------------------------

CXXMemberCallExpr* CallMemberFun(Expr* memExpr, QualType retType /*, const std::vector<Expr*>& params*/)
{
    return CXXMemberCallExpr::Create(GetGlobalAST(), memExpr, /*params*/ {}, retType, VK_LValue, {}, {});
}
//-----------------------------------------------------------------------------

ReturnStmt* Return(Expr* stmt)
{
    return ReturnStmt::Create(GetGlobalAST(), {}, stmt, nullptr);
}
//-----------------------------------------------------------------------------

ReturnStmt* Return(const ValueDecl* stmt)
{
    return Return(mkDeclRefExpr(stmt));
}
//-----------------------------------------------------------------------------

SwitchStmt* Switch(Expr* stmt)
{
    return SwitchStmt::Create(GetGlobalAST(), nullptr, nullptr, stmt, {}, {});
}
//-----------------------------------------------------------------------------

BreakStmt* Break()
{
    return new(GetGlobalAST()) BreakStmt(SourceLocation{});
}
//-----------------------------------------------------------------------------

CaseStmt* Case(int value, Stmt* stmt)
{
    auto* caseStmt = CaseStmt::Create(GetGlobalAST(), Int32(value), nullptr, {}, {}, {});
    caseStmt->setSubStmt(stmt);

    return caseStmt;
}
//-----------------------------------------------------------------------------

VarDecl* Variable(std::string_view name, QualType type, DeclContext* dc)
{
    auto& ctx = GetGlobalAST();

    if(nullptr == dc) {
        dc = GetGlobalAST().getTranslationUnitDecl();
    }

    return VarDecl::Create(const_cast<ASTContext&>(ctx), dc, {}, {}, &ctx.Idents.get(name), type, nullptr, SC_None);
}
//-----------------------------------------------------------------------------

NullStmt* mkNullStmt()
{
    static auto* nstmt = new(GetGlobalAST()) NullStmt({}, false);
    return nstmt;
}
//-----------------------------------------------------------------------------

Stmt* Comment(std::string_view comment)
{
    return new(GetGlobalAST()) CppInsightsCommentStmt{comment};
}
//-----------------------------------------------------------------------------

CXXRecordDecl* Struct(std::string_view name)
{
    auto getRecord = [&] {
        auto& ctx = GetGlobalAST();

        return CXXRecordDecl::Create(ctx,
#if IS_CLANG_NEWER_THAN(17)
                                     TagTypeKind::Struct
#else
                                     TTK_Struct
#endif
                                     ,
                                     ctx.getTranslationUnitDecl(),
                                     {},
                                     {},
                                     &ctx.Idents.get(name),
                                     nullptr,
                                     false);
    };

    auto* rd = getRecord();
    rd->startDefinition();

    // A "normal" struct has itself attached as a Decl. To make everything work do the same thing here
    auto* selfDecl = getRecord();
    selfDecl->setAccess(AS_public);
    rd->addDecl(selfDecl);

    return rd;
}
//-----------------------------------------------------------------------------

FieldDecl* mkFieldDecl(DeclContext* dc, std::string_view name, QualType type)
{
    auto& ctx = GetGlobalAST();
    auto* fieldDecl =
        FieldDecl::Create(ctx, dc, {}, {}, &ctx.Idents.get(name), type, nullptr, nullptr, false, ICIS_NoInit);
    fieldDecl->setAccess(AS_public);

    return fieldDecl;
}
//-----------------------------------------------------------------------------

ArraySubscriptExpr* ArraySubscript(const Expr* lhs, uint64_t index, QualType type)
{
    return new(GetGlobalAST()) ArraySubscriptExpr(
        const_cast<Expr*>(lhs), Int32(index), type, ExprValueKind::VK_LValue, ExprObjectKind::OK_Ordinary, {});
}
//-----------------------------------------------------------------------------

params_vector to_params_view(params_store& params)
{
    params_vector ret{};
    ret.reserve(params.size());

    for(const auto& [name, type] : params) {
        ret.emplace_back(name, type);
    }

    return ret;
}
//-----------------------------------------------------------------------------

DeclRefExpr* mkVarDeclRefExpr(std::string_view name, QualType type)
{
    auto* internalThisVar = Variable(name, type);
    auto* declRef         = mkDeclRefExpr(internalThisVar);

    return declRef;
}
//-----------------------------------------------------------------------------

static CallExpr* CallConstructor(QualType        ctorType,
                                 DeclRefExpr*    lhsDeclRef,
                                 Expr*           lhsMemberExpr,
                                 ArrayRef<Expr*> callParams,
                                 DoCast          doCast,
                                 AsReference     asReference)
{
    if(nullptr == lhsMemberExpr) {
        lhsMemberExpr = lhsDeclRef;
    }

    auto* callCtor =
        CreateFunctionDecl(StrCat("Constructor_"sv, GetName(ctorType)), {{kwInternalThis, lhsDeclRef->getType()}});

    SmallVector<Expr*, 8> modCallParams{};

    if(DoCast::Yes == doCast) {
        modCallParams.push_back(StaticCast(lhsDeclRef->getType(), lhsDeclRef, false));

    } else {
        if(AsReference::Yes == asReference) {
            modCallParams.push_back(Ref(lhsMemberExpr));
        } else {
            modCallParams.push_back(lhsMemberExpr);
        }
    }

    modCallParams.append(callParams.begin(), callParams.end());

    return Call(callCtor, modCallParams);
}
//-----------------------------------------------------------------------------

CallExpr* CallConstructor(QualType         ctorType,
                          QualType         lhsType,
                          const FieldDecl* fieldDecl,
                          ArrayRef<Expr*>  callParams,
                          DoCast           doCast,
                          AsReference      asReference)
{
    auto* lhsDeclRef = mkVarDeclRefExpr(kwInternalThis, lhsType);
    Expr* lhsMemberExpr{};

    if(DoCast::No == doCast) {
        lhsMemberExpr = AccessMember(lhsDeclRef, fieldDecl);
    }

    return CallConstructor(ctorType, lhsDeclRef, lhsMemberExpr, callParams, doCast, asReference);
}
//-----------------------------------------------------------------------------

CallExpr* CallConstructor(QualType        ctorType,
                          const VarDecl*  varDecl,
                          ArrayRef<Expr*> callParams,
                          DoCast          doCast,
                          AsReference     asReference)
{
    auto* lhsDeclRef = mkDeclRefExpr(varDecl);

    return CallConstructor(ctorType, lhsDeclRef, nullptr, callParams, doCast, asReference);
}
//-----------------------------------------------------------------------------

CXXBoolLiteralExpr* Bool(bool b)
{
    auto& ctx = GetGlobalAST();
    return new(ctx) CXXBoolLiteralExpr(b, ctx.BoolTy, {});
}
//-----------------------------------------------------------------------------

CallExpr* CallDestructor(const VarDecl* varDecl)
{
    auto* lhsDeclRef = mkDeclRefExpr(varDecl);

    auto* callDtor = CreateFunctionDecl(StrCat("Destructor_"sv, GetName(varDecl->getType())),
                                        {{kwInternalThis, lhsDeclRef->getType()}});

    return Call(callDtor, {Ref(lhsDeclRef)});
}
//-----------------------------------------------------------------------------

QualType Typedef(std::string_view name, QualType underlayingType)
{
    auto& ctx  = GetGlobalAST();
    auto* type = TypedefDecl::Create(const_cast<ASTContext&>(ctx),
                                     ctx.getTranslationUnitDecl(),
                                     {},
                                     {},
                                     &ctx.Idents.get(name),
                                     ctx.getTrivialTypeSourceInfo(underlayingType));

    return ctx.getTypeDeclType(type);
}
//-----------------------------------------------------------------------------

QualType GetRecordDeclType(const CXXMethodDecl* md)
{
    return QualType(md->getParent()->getTypeForDecl(), 0);
}
//-----------------------------------------------------------------------------

QualType GetRecordDeclType(const RecordDecl* rd)
{
    return QualType(rd->getTypeForDecl(), 0);
}
//-----------------------------------------------------------------------------

CXXNewExpr* New(ArrayRef<Expr*> placementArgs, const Expr* expr, QualType t)
{
    auto& ctx = GetGlobalAST();

    return CXXNewExpr::Create(ctx,
                              false,
                              nullptr,
                              nullptr,
                              true,
                              false,
                              placementArgs,
                              SourceRange{},
                              std::optional<Expr*>{},
#if IS_CLANG_NEWER_THAN(17)
                              CXXNewInitializationStyle::Parens
#else
                              CXXNewExpr::CallInit
#endif
                              ,
                              const_cast<Expr*>(expr),
                              Ptr(t),
                              ctx.getTrivialTypeSourceInfo(t),
                              SourceRange{},
                              SourceRange{});
}
//-----------------------------------------------------------------------------

BinaryOperator* Mul(Expr* lhs, Expr* rhs)
{
    return mkBinaryOperator(lhs, rhs, BinaryOperator::Opcode::BO_Mul, lhs->getType());
}
//-----------------------------------------------------------------------------

void StmtsContainer::AddBodyStmts(Stmt* body)
{
    if(auto* b = dyn_cast_or_null<CompoundStmt>(body)) {
        auto children = b->children();
        mStmts.append(children.begin(), children.end());
    } else if(not isa<NullStmt>(body)) {
        mStmts.push_back(body);
    }
}
//-----------------------------------------------------------------------------

void ReplaceNode(Stmt* parent, Stmt* oldNode, Stmt* newNode)
{
    std::replace(parent->child_begin(), parent->child_end(), oldNode, newNode);
}
//-----------------------------------------------------------------------------

SmallVector<Expr*, 5> ArgsToExprVector(const Expr* expr)
{
    if(const auto* ctorExpr = dyn_cast_or_null<CXXConstructExpr>(expr)) {
        auto mutableCtorExpr = const_cast<CXXConstructExpr*>(ctorExpr);

        return {mutableCtorExpr->arg_begin(), mutableCtorExpr->arg_end()};
    }

    return {};
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights::asthelpers
