/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include <iterator>
#include <optional>
#include <vector>
#include "CodeGenerator.h"
#include "DPrint.h"
#include "Insights.h"
#include "InsightsHelpers.h"
#include "NumberIterator.h"

#include <algorithm>
//-----------------------------------------------------------------------------

namespace clang::insights {

constexpr std::string_view CORO_FRAME_NAME{"__f"sv};
const std::string          CORO_FRAME_ACCESS{StrCat(CORO_FRAME_NAME, "->"sv)};
const std::string          CORO_FRAME_ACCESS_THIS{StrCat(CORO_FRAME_ACCESS, kwInternalThis)};
const std::string          SUSPEND_INDEX_NAME{BuildInternalVarName("suspend_index"sv)};
const std::string          INITIAL_AWAIT_SUSPEND_CALLED_NAME{BuildInternalVarName("initial_await_suspend_called"sv)};
const std::string          RESUME_LABEL_PREFIX{BuildInternalVarName("resume"sv)};
const std::string          FINAL_SUSPEND_NAME{BuildInternalVarName("final_suspend"sv)};
//-----------------------------------------------------------------------------

namespace asthelpers {

auto* mkLabelDecl(std::string_view name)
{
    auto& ctx = GetGlobalAST();

    return LabelDecl::Create(const_cast<ASTContext&>(ctx), ctx.getTranslationUnitDecl(), {}, &ctx.Idents.get(name));
}

LabelStmt* mkLabelStmt(std::string_view name)
{
    return new(GetGlobalAST()) LabelStmt({}, mkLabelDecl(name), nullptr);
}

CompoundStmt* mkCompoundStmt(ArrayRef<Stmt*> bodyStmts, SourceLocation beginLoc = {}, SourceLocation endLoc = {})
{
    return CompoundStmt::Create(GetGlobalAST(),
                                bodyStmts,
#if IS_CLANG_NEWER_THAN(14)
                                FPOptionsOverride{},
#endif
                                beginLoc,
                                endLoc);
}

auto* mkIfStmt(Expr* condition, ArrayRef<Stmt*> bodyStmts)
{
    return IfStmt::Create(GetGlobalAST(),
                          {},
                          IfStatementKind::Ordinary,
                          nullptr,
                          nullptr,
                          condition,
                          {},
                          {},
                          mkCompoundStmt(bodyStmts),
                          {},
                          /*else*/ nullptr);
}

auto* mkUnaryOperator(Expr* stmt, UnaryOperatorKind kind, QualType type)
{
    return UnaryOperator::Create(GetGlobalAST(), stmt, kind, type, VK_PRValue, OK_Ordinary, {}, false, {});
}

auto* mkNotIfStmt(Expr* stmt, ArrayRef<Stmt*> bodyStmts)
{
    auto* opNot = mkUnaryOperator(stmt, UO_LNot, stmt->getType());

    return mkIfStmt(opNot, bodyStmts);
}

auto* mkIntegerLiteral32(uint64_t value)
{
    auto&       ctx = GetGlobalAST();
    llvm::APInt v{32, value, true};

    return IntegerLiteral::Create(ctx, v, ctx.IntTy, {});
}

auto* mkBoolLiteral(bool value)
{
    auto& ctx = GetGlobalAST();

    return new(ctx) CXXBoolLiteralExpr(value, ctx.BoolTy, {});
}

auto* mkMemberExpr(Expr* expr, ValueDecl* vd, bool isArrow = true)
{
    return MemberExpr::CreateImplicit(
        GetGlobalAST(), expr, isArrow, vd, vd->getType().getNonReferenceType(), VK_LValue, OK_Ordinary);
}

auto* mkBinaryOperator(Expr* lhs, Expr* rhs, BinaryOperator::Opcode opc, QualType resType)
{
    return BinaryOperator::Create(
        GetGlobalAST(), lhs, rhs, opc, resType.getNonReferenceType(), VK_LValue, OK_Ordinary, {}, {});
}

auto* mkAssign(DeclRefExpr* declRef, FieldDecl* field, Expr* assignExpr)
{
    auto* me = mkMemberExpr(declRef, field);

    return mkBinaryOperator(me, assignExpr, BO_Assign, field->getType());
}

GotoStmt* mkGotoStmt(std::string_view labelName)
{
    return new(GetGlobalAST()) GotoStmt(asthelpers::mkLabelDecl(labelName), {}, {});
}

using params_vector = std::vector<std::pair<std::string_view, QualType>>;

auto* mkFunctionDeclBase(std::string_view     name,
                         QualType             returnType,
                         const params_vector& parameters,
                         DeclContext*         declCtx)
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

    SmallVector<ParmVarDecl*, 16> paramVarDecls{};

    for(const auto& [name, type] : parameters) {
        ParmVarDecl* param = ParmVarDecl::Create(
            const_cast<ASTContext&>(ctx), fdd, {}, {}, &ctx.Idents.get(name), type, nullptr, SC_None, nullptr);
        param->setScopeInfo(0, 0);
        paramVarDecls.push_back(param);
    }

    fdd->setParams(paramVarDecls);

    return fdd;
}

auto* mkFunctionDecl(std::string_view name, QualType returnType, const params_vector& parameters)
{
    return mkFunctionDeclBase(name, returnType, parameters, GetGlobalAST().getTranslationUnitDecl());
}

auto* mkStdFunctionDecl(std::string_view name, QualType returnType, const params_vector& parameters)
{
    auto&          ctx   = GetGlobalAST();
    NamespaceDecl* stdNs = NamespaceDecl::Create(const_cast<ASTContext&>(ctx),
                                                 ctx.getTranslationUnitDecl(),
                                                 false,
                                                 {},
                                                 {},
                                                 &ctx.Idents.get("std"),
                                                 nullptr
#if IS_CLANG_NEWER_THAN(15)
                                                 ,
                                                 false
#endif
    );

    return mkFunctionDeclBase(name, returnType, parameters, stdNs);
}

auto* mkDeclRefExpr(ValueDecl* vd)
{
    return DeclRefExpr::Create(GetGlobalAST(),
                               NestedNameSpecifierLoc{},
                               SourceLocation{},
                               vd,
                               false,
                               SourceLocation{},
                               vd->getType(),
                               VK_LValue,
                               nullptr,
                               nullptr,
                               NOUR_None);
}

auto* mkCallExpr(FunctionDecl* fd, QualType retType, const std::vector<Expr*>& params)
{
    auto* fdDref = asthelpers::mkDeclRefExpr(fd);

    return CallExpr::Create(GetGlobalAST(), fdDref, params, retType, VK_LValue, {}, {});
}

auto* mkMemberCallExpr(MemberExpr* memExpr, QualType retType /*, const std::vector<Expr*>& params*/)
{
    return CXXMemberCallExpr::Create(GetGlobalAST(), memExpr, /*params*/ {}, retType, VK_LValue, {}, {});
}

auto* mkReturnStmt(Expr* stmt = nullptr)
{
    return ReturnStmt::Create(GetGlobalAST(), {}, stmt, nullptr);
}

auto* mkCaseStmt(int value, Stmt* stmt)
{
    auto* il       = asthelpers::mkIntegerLiteral32(value);
    auto* caseStmt = CaseStmt::Create(GetGlobalAST(), il, nullptr, {}, {}, {});
    caseStmt->setSubStmt(stmt);

    return caseStmt;
}

auto* mkVarDecl(std::string_view name, QualType type)
{
    auto& ctx = GetGlobalAST();

    return VarDecl::Create(const_cast<ASTContext&>(ctx),
                           ctx.getTranslationUnitDecl(),
                           {},
                           {},
                           &ctx.Idents.get(name),
                           type,
                           nullptr,
                           SC_None);
}

auto* mkNullStmt()
{
    static auto* nstmt = new(GetGlobalAST()) NullStmt({}, false);
    return nstmt;
}

auto* mkInsightsComment(std::string_view comment)
{
    return new(GetGlobalAST()) CppInsightsCommentStmt{comment};
}

auto* mkFunctionReference(FunctionDecl* fd)
{
    auto* dref = asthelpers::mkDeclRefExpr(fd);
    return asthelpers::mkUnaryOperator(dref, UO_AddrOf, fd->getType());
}

auto* mkCXXRecordDecl(std::string_view name)
{
    auto& ctx = GetGlobalAST();

    return CXXRecordDecl::Create(
        ctx, TTK_Struct, ctx.getTranslationUnitDecl(), {}, {}, &ctx.Idents.get(name), nullptr, false);
}
}  // namespace asthelpers

UnaryExprOrTypeTraitExpr* Sizeof(QualType toType)
{
    const auto& ctx = GetGlobalAST();
    return new(ctx) UnaryExprOrTypeTraitExpr(UETT_SizeOf, ctx.getTrivialTypeSourceInfo(toType), toType, {}, {});
}
//-----------------------------------------------------------------------------

QualType CoroutinesCodeGenerator::GetFramePointerType() const
{
    return GetGlobalAST().getPointerType(GetFrameType());
}
//-----------------------------------------------------------------------------

CoroutinesCodeGenerator::~CoroutinesCodeGenerator()
{
    RETURN_IF(not(mASTData.mFrameType and mASTData.mDoInsertInDtor));

    mASTData.mFrameType->completeDefinition();

    OutputFormatHelper ofm{};

    // Using the "normal" CodeGenerator here as this is only about inserting the made up coroutine-frame.
    CodeGenerator codeGenerator{ofm};
    codeGenerator.InsertArg(mASTData.mFrameType);

    // Insert the made-up struct before the function declaration
    mOutputFormatHelper.InsertAt(mPosBeforeFunc, ofm);
}
//-----------------------------------------------------------------------------

///! A helper type to have a container for ArrayRef
struct StmtsContainer
{
    std::vector<Stmt*> mStmts{};

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

    operator ArrayRef<Stmt*>() { return mStmts; }
};
//-----------------------------------------------------------------------------

static FieldDecl* AddField(CoroutineASTData& astData, std::string_view name, QualType type)
{
    if(nullptr == astData.mFrameType) {
        return nullptr;
    }

    auto& ctx       = GetGlobalAST();
    auto* fieldDecl = FieldDecl::Create(
        ctx, astData.mFrameType, {}, {}, &ctx.Idents.get(name), type, nullptr, nullptr, false, ICIS_NoInit);

    fieldDecl->setAccess(AS_public);
    astData.mFrameType->addDecl(fieldDecl);

    return fieldDecl;
}
//-----------------------------------------------------------------------------

FieldDecl* CoroutinesCodeGenerator::AddField(std::string_view name, QualType type)
{
    return ::clang::insights::AddField(mASTData, name, type);
}
//-----------------------------------------------------------------------------

static void ReplaceAll(std::string& str, std::string_view from, std::string_view to)
{
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();  // Handles case where 'to' is a substring of 'from'
    }
}
//-----------------------------------------------------------------------------

static auto* CreateFunctionDecl(std::string funcName, asthelpers::params_vector params)
{
    const std::string coroFsmName{BuildInternalVarName(funcName)};
    return asthelpers::mkFunctionDecl(coroFsmName, GetGlobalAST().VoidTy, params);
}
//-----------------------------------------------------------------------------

static void SetFunctionBody(FunctionDecl* fd, StmtsContainer& bodyStmts)
{
    fd->setBody(asthelpers::mkCompoundStmt(bodyStmts));
}
//-----------------------------------------------------------------------------

static std::string BuildSuspendVarName(const OpaqueValueExpr* stmt)
{
    return BuildInternalVarName(
        MakeLineColumnName(GetGlobalAST().getSourceManager(), stmt->getSourceExpr()->getBeginLoc(), "suspend_"sv));
}
//-----------------------------------------------------------------------------

/// \brief Find a \c SuspendsExpr's in a coroutine body statement for early transformation.
///
/// Traverse the whole CoroutineBodyStmt to find all appearing \c VarDecl. These need to be rerouted to the
/// coroutine frame and hence prefixed by something like __f->. For that reason we only look for \c VarDecls
/// directly appearing in the body, \c CallExpr will be skipped.
class CoroutineASTTransformer : public StmtVisitor<CoroutineASTTransformer>
{
    StmtsContainer                        mBodyStmts{};
    Stmt*                                 mPrevStmt{};  // used to insert the suspendexpr
    CoroutineASTData&                     mASTData;
    Stmt*                                 mStaged{};
    bool                                  mSkip{};
    bool                                  mFinalSuspend{};
    size_t&                               mSuspendsCount;
    llvm::DenseMap<VarDecl*, MemberExpr*> mVarNamePrefix{};

public:
    CoroutineASTTransformer(CoroutineASTData&                     coroutineASTData,
                            size_t&                               suspendsCounter,
                            Stmt*                                 stmt,
                            llvm::DenseMap<VarDecl*, MemberExpr*> varNamePrefix,
                            Stmt*                                 prev = nullptr)
    : mASTData{coroutineASTData}
    , mPrevStmt{prev}
    , mSuspendsCount{suspendsCounter}
    , mVarNamePrefix{varNamePrefix}
    {
        if(nullptr == mPrevStmt) {
            mPrevStmt = stmt;
        }

        Visit(stmt);
    }

    void Visit(Stmt* stmt)
    {
        if(stmt) {
            StmtVisitor<CoroutineASTTransformer>::Visit(stmt);
        }
    }

    void VisitCompoundStmt(CompoundStmt* stmt)
    {
        for(auto* child : stmt->body()) {
            mStaged = child;
            Visit(child);

            if(not mSkip) {
                mBodyStmts.Add(child);
            }

            mSkip = false;
        }

        auto* comp = asthelpers::mkCompoundStmt(mBodyStmts);

        std::replace(mPrevStmt->child_begin(), mPrevStmt->child_end(), stmt, comp);

        mBodyStmts.clear();
    }

    void VisitSwitchStmt(SwitchStmt* stmt)
    {
        Visit(stmt->getCond());

        CoroutineASTTransformer{mASTData, mSuspendsCount, stmt->getBody(), mVarNamePrefix, stmt};
    }

    void VisitDoStmt(DoStmt* stmt)
    {
        Visit(stmt->getCond());

        CoroutineASTTransformer{mASTData, mSuspendsCount, stmt->getBody(), mVarNamePrefix, stmt};
    }

    void VisitWhileStmt(WhileStmt* stmt)
    {
        Visit(stmt->getCond());

        CoroutineASTTransformer{mASTData, mSuspendsCount, stmt->getBody(), mVarNamePrefix, stmt};
    }

    void VisitIfStmt(IfStmt* stmt)
    {
        Visit(stmt->getCond());

        CoroutineASTTransformer{mASTData, mSuspendsCount, stmt->getThen(), mVarNamePrefix, stmt};

        CoroutineASTTransformer{mASTData, mSuspendsCount, stmt->getElse(), mVarNamePrefix, stmt};
    }

    void VisitForStmt(ForStmt* stmt)
    {
        // technically because of the init the entire for loop should be put into a dedicated scope
        Visit(stmt->getInit());

        // Special case. A VarDecl in init will be added to the body of the function and the actual init is left
        // untouched. Work some magic to put it in the right place.
        if(mSkip) {
            auto* oldInit = stmt->getInit();
            auto* newInit = mBodyStmts.mStmts.back();
            mBodyStmts.mStmts.pop_back();

            std::replace(stmt->child_begin(),
                         stmt->child_end(),
                         dyn_cast_or_null<Stmt>(oldInit),
                         dyn_cast_or_null<Stmt>(newInit));

            mSkip = false;
        }

        Visit(stmt->getCond());

        Visit(stmt->getInc());

        CoroutineASTTransformer{mASTData, mSuspendsCount, stmt->getBody(), mVarNamePrefix, stmt};

        mSkip = false;
    }

    void VisitCXXForRangeStmt(CXXForRangeStmt* stmt)
    {
        Visit(stmt->getRangeStmt());

        // ignoring the loop variable should be fine.

        CoroutineASTTransformer{mASTData, mSuspendsCount, stmt->getBody(), mVarNamePrefix, stmt};
    }

    void VisitDeclRefExpr(DeclRefExpr* stmt)
    {
        if(auto* vd = dyn_cast_or_null<VarDecl>(stmt->getDecl())) {
            if(not vd->isLocalVarDeclOrParm() or not Contains(mVarNamePrefix, vd)) {
                return;
            }

            auto* memberExpr = mVarNamePrefix[vd];

            std::replace(mPrevStmt->child_begin(),
                         mPrevStmt->child_end(),
                         dyn_cast_or_null<Stmt>(stmt),
                         dyn_cast_or_null<Stmt>(memberExpr));
        }
    }

    void VisitDeclStmt(DeclStmt* stmt)
    {
        for(auto* decl : stmt->decls()) {
            if(auto* varDecl = dyn_cast_or_null<VarDecl>(decl)) {
                // add this point a placement-new would be appropriate for at least some cases.

                auto* field  = AddField(mASTData, GetName(*varDecl), varDecl->getType());
                auto* me     = asthelpers::mkMemberExpr(mASTData.mFrameAccessDeclRef, field);
                auto* assign = asthelpers::mkBinaryOperator(me, varDecl->getInit(), BO_Assign, field->getType());

                mVarNamePrefix.insert(std::make_pair(varDecl, me));

                Visit(varDecl->getInit());

                mSkip = true;
                mBodyStmts.Add(assign);

            } else if(const auto* recordDecl = dyn_cast_or_null<CXXRecordDecl>(decl)) {
                mASTData.mFrameType->addDecl(const_cast<CXXRecordDecl*>(recordDecl));
            }
        }
    }

    void VisitCXXThisExpr(CXXThisExpr* stmt)
    {
        auto& ctx       = GetGlobalAST();
        auto* fieldDecl = FieldDecl::Create(ctx,
                                            mASTData.mFrameType,
                                            {},
                                            {},
                                            &ctx.Idents.get(kwInternalThis),
                                            stmt->getType(),
                                            nullptr,
                                            nullptr,
                                            false,
                                            ICIS_NoInit);
        fieldDecl->setAccess(AS_public);

        auto* indirectThisMemberExpr = asthelpers::mkMemberExpr(mASTData.mFrameAccessDeclRef, fieldDecl);

        std::replace(mPrevStmt->child_begin(),
                     mPrevStmt->child_end(),
                     dyn_cast_or_null<Stmt>(stmt),
                     dyn_cast_or_null<Stmt>(indirectThisMemberExpr));

        if(0 == mASTData.mThisExprs.size()) {
            mASTData.mThisExprs.push_back(stmt);
        }
    }

    void VisitCallExpr(CallExpr* stmt)
    {
        auto* tmp = mPrevStmt;
        mPrevStmt = stmt;

        for(auto* arg : stmt->arguments()) {
            Visit(arg);
        }

        mPrevStmt = tmp;
    }

    void VisitCXXMemberCallExpr(CXXMemberCallExpr* stmt)
    {
        auto* tmp = mPrevStmt;
        mPrevStmt = stmt->getCallee();

        Visit(stmt->getCallee());

        mPrevStmt = tmp;

        StmtVisitor<CoroutineASTTransformer>::VisitCXXMemberCallExpr(stmt);
    }

    void VisitCoreturnStmt(CoreturnStmt* stmt)
    {
        Visit(stmt->getOperand());
        Visit(stmt->getPromiseCall());
    }

    void VisitCoyieldExpr(CoyieldExpr* stmt)
    {
        ++mSuspendsCount;

        if(isa<ExprWithCleanups>(mStaged)) {
            mBodyStmts.Add(stmt);
            mSkip = true;
        }

        Visit(stmt->getOperand());
    }

    void VisitCoawaitExpr(CoawaitExpr* stmt)
    {
        if(not mFinalSuspend) {
            ++mSuspendsCount;
        }

        if(const bool returnsVoid{stmt->getResumeExpr()->getType()->isVoidType()}; returnsVoid) {
            Visit(stmt->getOperand());

            // in the void return case there is nothing to do, because this expression (potentially) is not nested.
            return;
        }

        mBodyStmts.Add(stmt);

        // Note: Add the this pointer to the name isn't the best but s quick approach
        const std::string name{StrCat(CORO_FRAME_ACCESS, BuildSuspendVarName(stmt->getOpaqueValue()), "_res"sv)};

        auto* resultVar        = asthelpers::mkVarDecl(name, stmt->getType());
        auto* resultVarDeclRef = asthelpers::mkDeclRefExpr(resultVar);

        std::replace(mPrevStmt->child_begin(),
                     mPrevStmt->child_end(),
                     dyn_cast_or_null<Stmt>(stmt),
                     dyn_cast_or_null<Stmt>(resultVarDeclRef));

        Visit(stmt->getCommonExpr());
        Visit(stmt->getOperand());
        Visit(stmt->getSuspendExpr());
        Visit(stmt->getReadyExpr());
        Visit(stmt->getResumeExpr());
    }

    void VisitCoroutineBodyStmt(CoroutineBodyStmt* stmt)
    {
        auto* varDecl = stmt->getPromiseDecl();

        mASTData.mPromiseField = AddField(mASTData, GetName(*varDecl), varDecl->getType());
        auto* me               = asthelpers::mkMemberExpr(mASTData.mFrameAccessDeclRef, mASTData.mPromiseField);
        auto* assign =
            asthelpers::mkBinaryOperator(me, varDecl->getInit(), BO_Assign, mASTData.mPromiseField->getType());

        mVarNamePrefix.insert(std::make_pair(varDecl, me));

        auto& ctx = GetGlobalAST();

        // add the suspend index variable
        mASTData.mSuspendIndexField = AddField(mASTData, SUSPEND_INDEX_NAME, ctx.IntTy);
        mASTData.mSuspendIndexAccess =
            asthelpers::mkMemberExpr(mASTData.mFrameAccessDeclRef, mASTData.mSuspendIndexField);

        // https://timsong-cpp.github.io/cppwp/n4861/dcl.fct.def.coroutine#5.3
        mASTData.mInitialAwaitResumeCalledField = AddField(mASTData, INITIAL_AWAIT_SUSPEND_CALLED_NAME, ctx.BoolTy);
        mASTData.mInitialAwaitResumeCalledAccess =
            asthelpers::mkMemberExpr(mASTData.mFrameAccessDeclRef, mASTData.mInitialAwaitResumeCalledField);

        for(auto* param : stmt->getParamMoves()) {
            if(auto* declStmt = dyn_cast_or_null<DeclStmt>(param)) {
                if(auto* varDecl2 = dyn_cast_or_null<VarDecl>(declStmt->getSingleDecl())) {
                    //  For the captured parameters we need to find the ParmVarDecl instead of the newly created VarDecl
                    if(auto* declRef = FindDeclRef(varDecl2->getAnyInitializer())) {
                        auto* varDecl = dyn_cast<ParmVarDecl>(declRef->getDecl());

                        auto* field  = AddField(mASTData, GetName(*varDecl), varDecl->getType());
                        auto* me     = asthelpers::mkMemberExpr(mASTData.mFrameAccessDeclRef, field);
                        auto* assign = asthelpers::mkBinaryOperator(
                            me, const_cast<Expr*>(varDecl2->getInit()), BO_Assign, field->getType());

                        mVarNamePrefix.insert(std::make_pair(const_cast<ParmVarDecl*>(varDecl), me));
                    }
                }
            }
        }

        Visit(stmt->getBody());

        Visit(stmt->getReturnStmt());
        Visit(stmt->getReturnValue());
        Visit(stmt->getReturnValueInit());
        Visit(stmt->getExceptionHandler());
        Visit(stmt->getReturnStmtOnAllocFailure());

        Visit(stmt->getInitSuspendStmt());

        // final suspend point doesn't need a label
        mFinalSuspend = true;
        Visit(stmt->getFinalSuspendStmt());
        mFinalSuspend = false;
    }

    void VisitStmt(Stmt* stmt)
    {
        auto* tmp = mPrevStmt;
        mPrevStmt = stmt;

        for(auto* child : stmt->children()) {
            Visit(child);
        }

        mPrevStmt = tmp;
    }
};
//-----------------------------------------------------------------------------

void CoroutinesCodeGenerator::InsertCoroutine(const FunctionDecl& fd, const CoroutineBodyStmt* stmt)
{
    mOutputFormatHelper.OpenScope();

    auto& ctx = GetGlobalAST();

    mFSMName = [&] {
        OutputFormatHelper ofm{};
        CodeGenerator      codeGenerator{ofm};

        // Coroutines can be templates and then we end up with the same FSM name but different template parameters.
        // XXX: This will fail with NTTP's like 3.14
        if(const auto* args = fd.getTemplateSpecializationArgs()) {
            ofm.Append('_');
            OnceFalse needsUnderscore{};

            for(const auto& arg : args->asArray()) {
                if(needsUnderscore) {
                    ofm.Append('_');
                }

                codeGenerator.InsertTemplateArg(arg);
            }
        }

        auto& str = ofm.GetString();
        ReplaceAll(str, "<"sv, ""sv);
        ReplaceAll(str, ":"sv, ""sv);
        ReplaceAll(str, ">"sv, ""sv);
        ReplaceAll(str, ","sv, ""sv);
        ReplaceAll(str, " "sv, ""sv);

        if(fd.isOverloadedOperator()) {
            return StrCat(MakeLineColumnName(ctx.getSourceManager(), stmt->getBeginLoc(), "operator_"sv), str);
        } else {
            return StrCat(GetName(fd), str);
        }
    }();

    mFrameName = BuildInternalVarName(StrCat(mFSMName, "Frame"sv));

    // Insert a made up struct which holds the "captured" parameters stored in the coroutine frame
    mASTData.mFrameType = asthelpers::mkCXXRecordDecl(mFrameName);
    mASTData.mFrameType->startDefinition();

    // A "normal" struct has itself attached as a Decl. To make everything work do the same thing here
    auto* selfDecl = asthelpers::mkCXXRecordDecl(mFrameName);
    selfDecl->setAccess(AS_public);
    mASTData.mFrameType->addDecl(selfDecl);

    auto* frameAccess            = asthelpers::mkVarDecl(CORO_FRAME_NAME, GetFrameType());
    mASTData.mFrameAccessDeclRef = asthelpers::mkDeclRefExpr(frameAccess);

    // The coroutine frame starts with two function pointers to the resume and destroy function. See:
    // https://gcc.gnu.org/legacy-ml/gcc-patches/2020-01/msg01096.html:
    // "The ABI mandates that pointers into the coroutine frame point to an area
    // begining with two function pointers (to the resume and destroy functions
    // described below); these are immediately followed by the "promise object"
    // described in the standard."
    //
    // and
    // https://llvm.org/docs/Coroutines.html#id72 "Coroutine Representation"
    auto* resumeFnFd =
        asthelpers::mkFunctionDecl(hlpResumeFn, ctx.VoidTy, {{CORO_FRAME_NAME, ctx.getPointerType(GetFrameType())}});
    auto resumeFnType       = ctx.getPointerType(resumeFnFd->getType());
    mASTData.mResumeFnField = AddField(hlpResumeFn, resumeFnType);

    auto* destroyFnFd =
        asthelpers::mkFunctionDecl(hlpDestroyFn, ctx.VoidTy, {{CORO_FRAME_NAME, ctx.getPointerType(GetFrameType())}});
    auto destroyFnType       = ctx.getPointerType(destroyFnFd->getType());
    mASTData.mDestroyFnField = AddField(hlpDestroyFn, destroyFnType);

    // Allocated the made up frame
    mOutputFormatHelper.AppendCommentNewLine("Allocate the frame including the promise"sv);
    mOutputFormatHelper.AppendCommentNewLine("Note: The actual parameter new is __builtin_coro_size"sv);

    auto* coroFrameVar = asthelpers::mkVarDecl(CORO_FRAME_NAME, GetFramePointerType());

    CXXReinterpretCastExpr* reicast =
        CXXReinterpretCastExpr::Create(ctx,
                                       GetFramePointerType(),
                                       VK_LValue,
                                       CK_BitCast,
                                       stmt->getAllocate(),
                                       nullptr,
                                       ctx.getTrivialTypeSourceInfo(GetFramePointerType()),
                                       {},
                                       {},
                                       {});

    coroFrameVar->setInit(reicast);

    InsertArg(coroFrameVar);

    // P0057R8: [dcl.fct.def.coroutine] p8: get_return_object_on_allocation_failure indicates that new may return a
    // nullptr. In this case return get_return_object_on_allocation_failure.
    if(stmt->getReturnStmtOnAllocFailure()) {
        auto* nptr = new(ctx) CXXNullPtrLiteralExpr({});
        auto* bop  = asthelpers::mkBinaryOperator(nptr, mASTData.mFrameAccessDeclRef, BO_EQ, ctx.BoolTy);

        // Create an IfStmt.
        StmtsContainer bodyStmts{stmt->getReturnStmtOnAllocFailure()};
        auto*          ifStmt = asthelpers::mkIfStmt(bop, bodyStmts);

        mOutputFormatHelper.AppendNewLine();
        InsertArg(ifStmt);
    }

    CoroutineASTTransformer{
        mASTData, mSuspendsCounter, const_cast<CoroutineBodyStmt*>(stmt), llvm::DenseMap<VarDecl*, MemberExpr*>{}};

    // set initial suspend count to zero.
    auto* setSuspendIndexToZero = asthelpers::mkAssign(
        mASTData.mFrameAccessDeclRef, mASTData.mSuspendIndexField, asthelpers::mkIntegerLiteral32(0));
    InsertArgWithNull(setSuspendIndexToZero);

    // https://timsong-cpp.github.io/cppwp/n4861/dcl.fct.def.coroutine#5.3
    auto* initializeInitialAwaitResume = asthelpers::mkAssign(
        mASTData.mFrameAccessDeclRef, mASTData.mInitialAwaitResumeCalledField, asthelpers::mkBoolLiteral(false));
    InsertArgWithNull(initializeInitialAwaitResume);

    // Move the parameters first
    for(auto* param : stmt->getParamMoves()) {
        if(const auto* declStmt = dyn_cast_or_null<DeclStmt>(param)) {
            if(const auto* varDecl = dyn_cast_or_null<VarDecl>(declStmt->getSingleDecl())) {
                const auto varName = GetName(*varDecl);

                mOutputFormatHelper.AppendNewLine(CORO_FRAME_ACCESS,
                                                  varName,
                                                  " = "sv,
                                                  "std::forward<"sv,
                                                  GetName(varDecl->getType()),
                                                  ">("sv,
                                                  varName,
                                                  ");"sv);
            }
        }
    }

    std::vector<Expr*> exprs{};

    // According to https://eel.is/c++draft/dcl.fct.def.coroutine#5.7 the promise_type constructor can have
    // parameters. If so, they must be equal to the coroutines function parameters.
    // The code here performs a _simple_ lookup for a matching ctor without using Clang's overload resolution.
    ArrayRef<ParmVarDecl*>    funParams = fd.parameters();
    std::vector<ParmVarDecl*> funParamStorage{};
    QualType                  cxxMethodType{};

    if(const auto* cxxMethodDecl = dyn_cast_or_null<CXXMethodDecl>(&fd)) {
        funParamStorage.reserve(funParams.size() + 1);

        cxxMethodType = cxxMethodDecl->getThisObjectType();

        // In case we have a member function the first parameter is a reference to this. The following code injects
        // this parameter.
        funParamStorage.push_back(ParmVarDecl::Create(const_cast<ASTContext&>(ctx),
                                                      const_cast<FunctionDecl*>(&fd),
                                                      {},
                                                      {},
                                                      &ctx.Idents.get(CORO_FRAME_ACCESS_THIS),
                                                      cxxMethodType,
                                                      nullptr,
                                                      SC_None,
                                                      nullptr));

        // XXX: use ranges once available
        std::copy(funParams.begin(), funParams.end(), std::back_inserter(funParamStorage));

        funParams = funParamStorage;
    }

    auto getNonRefType = [&](auto* var) -> QualType {
        if(const auto* et = var->getType().getNonReferenceType()->template getAs<ElaboratedType>()) {
            return et->getNamedType();
        } else {
            return QualType(var->getType().getNonReferenceType().getTypePtrOrNull(), 0);
        }
    };

    for(auto* promiseTypeRecordDecl = mASTData.mPromiseField->getType()->getAsCXXRecordDecl();
        auto* ctor : promiseTypeRecordDecl->ctors()) {

        // XXX: use ranges once available
        if(not std::equal(
               ctor->param_begin(), ctor->param_end(), funParams.begin(), funParams.end(), [&](auto& a, auto& b) {
                   return getNonRefType(a) == getNonRefType(b);
               })) {
            continue;
        }

        // In case of a promise ctor which takes this as the first argument, that parameter needs to be deferences,
        // as it can only be taken as a reference.
        OnceTrue derefFirstParam{};

        if(not ctor->param_empty() and
           (getNonRefType(ctor->getParamDecl(0)) == QualType(cxxMethodType.getTypePtrOrNull(), 0))) {
            if(0 == mASTData.mThisExprs.size()) {
                mASTData.mThisExprs.push_back(new(ctx) CXXThisExpr({}, ctx.getPointerType(cxxMethodType), false));
            }
        } else {
            (void)static_cast<bool>(derefFirstParam);  // set it to false
        }

        for(const auto& fparam : funParams) {
            if(derefFirstParam) {
                exprs.push_back(
                    asthelpers::mkUnaryOperator(asthelpers::mkDeclRefExpr(fparam), UO_Deref, fparam->getType()));

            } else {
                exprs.push_back(asthelpers::mkMemberExpr(mASTData.mFrameAccessDeclRef, fparam));
            }
        }

        if(funParams.size()) {
            // The <new> header needs to be included.
            mHaveCoroutine = true;
        }

        break;  // We've found what we were looking for
    }

    if(mASTData.mThisExprs.size()) {
        mOutputFormatHelper.AppendNewLine(CORO_FRAME_ACCESS_THIS, " = this;"sv);
    }

    // Now call the promise ctor, as it may access some of the parameters it comes at this point.
    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.AppendCommentNewLine("Construct the promise."sv);
    auto* me    = asthelpers::mkMemberExpr(mASTData.mFrameAccessDeclRef, mASTData.mPromiseField);
    auto* asRef = asthelpers::mkUnaryOperator(me, UO_AddrOf, mASTData.mPromiseField->getType());

    auto* ctorArgs = new(ctx) InitListExpr{ctx, {}, static_cast<ArrayRef<Expr*>>(exprs), {}};

    CXXNewExpr* newFrame =
        CXXNewExpr::Create(ctx,
                           false,
                           nullptr,
                           nullptr,
                           true,
                           false,
                           {asRef},
                           SourceRange{},
#if IS_CLANG_NEWER_THAN(15)
                           std::optional<Expr*>{},
#else
                           Optional<Expr*>{},
#endif
                           CXXNewExpr::ListInit,
                           ctorArgs,
                           ctx.getPointerType(mASTData.mPromiseField->getType()),
                           ctx.getTrivialTypeSourceInfo(ctx.getPointerType(mASTData.mPromiseField->getType())),
                           SourceRange{},
                           SourceRange{});

    InsertArgWithNull(newFrame);

    // Add parameters from the original function to the list

    // P0057R8: [dcl.fct.def.coroutine] p5: before initial_suspend and at tops 1
#if not IS_CLANG_NEWER_THAN(14)
    mOutputFormatHelper.AppendNewLine();
    InsertArg(stmt->getResultDecl());
#endif

    // Make a call to the made up state machine function for the initial suspend
    mOutputFormatHelper.AppendNewLine();

    // [dcl.fct.def.coroutine]
    mOutputFormatHelper.AppendCommentNewLine("Forward declare the resume and destroy function."sv);

    auto* fsmFuncDecl = CreateFunctionDecl(StrCat(mFSMName, "Resume"sv), {{CORO_FRAME_NAME, GetFramePointerType()}});
    InsertArg(fsmFuncDecl);
    auto* deallocFuncDecl =
        CreateFunctionDecl(StrCat(mFSMName, "Destroy"sv), {{CORO_FRAME_NAME, GetFramePointerType()}});
    InsertArg(deallocFuncDecl);

    mOutputFormatHelper.AppendNewLine();

    mOutputFormatHelper.AppendCommentNewLine("Assign the resume and destroy function pointers."sv);

    auto* assignResumeFn = asthelpers::mkAssign(
        mASTData.mFrameAccessDeclRef, mASTData.mResumeFnField, asthelpers::mkFunctionReference(fsmFuncDecl));
    InsertArgWithNull(assignResumeFn);

    auto* assignDestroyFn = asthelpers::mkAssign(
        mASTData.mFrameAccessDeclRef, mASTData.mDestroyFnField, asthelpers::mkFunctionReference(deallocFuncDecl));
    InsertArgWithNull(assignDestroyFn);
    mOutputFormatHelper.AppendNewLine();

    mOutputFormatHelper.AppendCommentNewLine(
        R"A(Call the made up function with the coroutine body for initial suspend.
     This function will be called subsequently by coroutine_handle<>::resume()
     which calls __builtin_coro_resume(__handle_))A"sv);

    auto* callCoroFSM = asthelpers::mkCallExpr(fsmFuncDecl, fsmFuncDecl->getType(), {mASTData.mFrameAccessDeclRef});
    InsertArgWithNull(callCoroFSM);

    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.AppendNewLine();

    InsertArg(stmt->getReturnStmt());

    mOutputFormatHelper.AppendSemiNewLine();

    mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::Yes);
    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.AppendNewLine();

    // add contents of the original function to the body of our made up function
    StmtsContainer fsmFuncBodyStmts{stmt};

    mOutputFormatHelper.AppendCommentNewLine("This function invoked by coroutine_handle<>::resume()"sv);
    SetFunctionBody(fsmFuncDecl, fsmFuncBodyStmts);
    InsertArg(fsmFuncDecl);

    mASTData.mDoInsertInDtor = true;  // As we have a coroutine insert the frame when this object goes out of scope.

#if 0  // Preserve for later. Technically the destructor for the entire frame that's made up below takes care of
       // everything.

    // A destructor is only present, if they promise_type or one of its members is non-trivially destructible.
    if(auto* dtor = mASTData.mPromiseField->getType()->getAsCXXRecordDecl()->getDestructor()) {
        deallocFuncBodyStmts.Add(asthelpers::mkInsightsComment("Deallocating the coroutine promise type"sv));

        auto* promiseAccess  = asthelpers::mkMemberExpr(mASTData.mFrameAccessDeclRef, mASTData.mPromiseField);
        auto* deallocPromise = asthelpers::mkMemberExpr(promiseAccess, dtor, false);
        auto* dtorCall       = asthelpers::mkMemberCallExpr(deallocPromise, dtor->getType());
        deallocFuncBodyStmts.Add(dtorCall);

    } else {
        deallocFuncBodyStmts.Add(
            asthelpers::mkInsightsComment("promise_type is trivially destructible, no dtor required."sv));
    }
#endif

    // This code isn't really there but it is the easiest and cleanest way to visualize the destruction of all
    // member in the frame. The deallocation function:
    // https://devblogs.microsoft.com/oldnewthing/20210331-00/?p=105028
    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.AppendCommentNewLine("This function invoked by coroutine_handle<>::destroy()"sv);

    StmtsContainer deallocFuncBodyStmts{asthelpers::mkInsightsComment("destroy all variables with dtors"sv)};

    auto* dtorFuncDecl = asthelpers::mkFunctionDecl(
        StrCat("~"sv, GetName(*mASTData.mFrameType)), ctx.VoidTy, {{CORO_FRAME_NAME, GetFramePointerType()}});
    auto* deallocPromise = asthelpers::mkMemberExpr(mASTData.mFrameAccessDeclRef, dtorFuncDecl);
    auto* dtorCall       = asthelpers::mkMemberCallExpr(deallocPromise, GetFrameType());
    deallocFuncBodyStmts.Add(dtorCall);

    deallocFuncBodyStmts.Add(asthelpers::mkInsightsComment("Deallocating the coroutine frame"sv));
    deallocFuncBodyStmts.Add(asthelpers::mkInsightsComment(
        "Note: The actual argument to delete is __builtin_coro_frame with the promise as parameter"sv));

    deallocFuncBodyStmts.Add(stmt->getDeallocate());

    SetFunctionBody(deallocFuncDecl, deallocFuncBodyStmts);
    InsertArg(deallocFuncDecl);
}
//-----------------------------------------------------------------------------

void CoroutinesCodeGenerator::InsertArg(const CoroutineBodyStmt* stmt)
{
    auto& ctx = GetGlobalAST();

    // insert a made up switch for continuing a resume
    SwitchStmt* sstmt = SwitchStmt::Create(ctx, nullptr, nullptr, mASTData.mSuspendIndexAccess, {}, {});

    // insert 0 with break for consistency
    auto*          initialSuspendCase = asthelpers::mkCaseStmt(0, new(ctx) BreakStmt(SourceLocation{}));
    StmtsContainer switchBodyStmts{initialSuspendCase};

    for(const auto& i : NumberIterator{mSuspendsCounter}) {
        switchBodyStmts.Add(asthelpers::mkCaseStmt(i + 1, asthelpers::mkGotoStmt(BuildResumeLabelName(i + 1))));
    }

    auto* switchBody = asthelpers::mkCompoundStmt(switchBodyStmts);
    sstmt->setBody(switchBody);

    StmtsContainer funcBodyStmts{asthelpers::mkInsightsComment("Create a switch to get to the correct resume point"sv),
                                 sstmt,
                                 stmt->getInitSuspendStmt()};

    // insert the init suspend expr
    mState = eState::InitialSuspend;

    if(mASTData.mThisExprs.size()) {
        AddField(kwInternalThis, mASTData.mThisExprs.at(0)->getType());
    }

    mInsertVarDecl      = false;
    mSupressRecordDecls = true;

    for(const auto* c : stmt->getBody()->children()) {
        funcBodyStmts.Add(c);
    }

    // InsertArg(stmt->getFallthroughHandler());

    auto* gotoFinalSuspend = asthelpers::mkGotoStmt(FINAL_SUSPEND_NAME);
    funcBodyStmts.Add(gotoFinalSuspend);

    auto* body = [&]() -> Stmt* {
        auto* tryBody = asthelpers::mkCompoundStmt(funcBodyStmts);

        // First open the try-catch block, as we get an error when jumping across such blocks with goto
        if(const auto* exceptionHandler = stmt->getExceptionHandler()) {
            // If we encounter an exceptionbefore inital_suspend's await_suspend was called we re-throw the
            // exception.
            auto* throwExpr = new(ctx) CXXThrowExpr(nullptr, {}, {}, false);
            auto* ifStmt    = asthelpers::mkNotIfStmt(mASTData.mInitialAwaitResumeCalledAccess, throwExpr);

            StmtsContainer catchBodyStmts{ifStmt, exceptionHandler};
            auto*          catchAll = new(ctx) CXXCatchStmt({}, nullptr, asthelpers::mkCompoundStmt(catchBodyStmts));

            return CXXTryStmt::Create(ctx, {}, tryBody, {catchAll});
        }

        return tryBody;
    }();

    InsertArg(body);

    mOutputFormatHelper.AppendNewLine();

    auto* finalSuspendLabel = asthelpers::mkLabelStmt(FINAL_SUSPEND_NAME);
    InsertArg(finalSuspendLabel);
    mState = eState::FinalSuspend;
    InsertArg(stmt->getFinalSuspendStmt());

    // disable prefixing names and types
    mInsertVarDecl = true;
}
//-----------------------------------------------------------------------------

void CoroutinesCodeGenerator::InsertArg(const CXXRecordDecl* stmt)
{
    if(not mSupressRecordDecls) {
        CodeGenerator::InsertArg(stmt);
    }
}
//-----------------------------------------------------------------------------

// We seem to need this, to peal of some static_casts in a CoroutineSuspendExpr.
void CoroutinesCodeGenerator::InsertArg(const ImplicitCastExpr* stmt)
{
    if(mSupressCasts) {
        InsertArg(stmt->getSubExpr());
    } else {
        CodeGenerator::InsertArg(stmt);
    }
}
//-----------------------------------------------------------------------------

// A special hack to avoid having calls to __builtin_coro_xxx as some of them result in a crash
// of the compiler and have assumption on the call order and function location.
void CoroutinesCodeGenerator::InsertArg(const CallExpr* stmt)
{
    if(const auto* callee = dyn_cast_or_null<DeclRefExpr>(stmt->getCallee()->IgnoreCasts())) {
        if(GetPlainName(*callee) == "__builtin_coro_frame"sv) {
            auto& ctx = GetGlobalAST();

            CXXStaticCastExpr* toVoid = CXXStaticCastExpr::Create(ctx,
                                                                  ctx.VoidPtrTy,
                                                                  VK_LValue,
                                                                  CK_BitCast,
                                                                  mASTData.mFrameAccessDeclRef,
                                                                  nullptr,
                                                                  ctx.getTrivialTypeSourceInfo(ctx.VoidPtrTy),
                                                                  {},
                                                                  {},
                                                                  {},
                                                                  {});

            CodeGenerator::InsertArg(toVoid);
            return;
        } else if(GetPlainName(*callee) == "__builtin_coro_free"sv) {
            CodeGenerator::InsertArg(stmt->getArg(0));
            return;
        } else if(GetPlainName(*callee) == "__builtin_coro_size"sv) {
            CodeGenerator::InsertArg(Sizeof(GetFrameType()));
            return;
        }
    }

    CodeGenerator::InsertArg(stmt);
}
//-----------------------------------------------------------------------------

static std::optional<std::string> FindValue(llvm::DenseMap<const Expr*, std::string>& map, const Expr* key)
{
    if(const auto& s = map.find(key); s != map.end()) {
        return s->second;
    }

    return {};
}
//-----------------------------------------------------------------------------

void CoroutinesCodeGenerator::InsertArg(const OpaqueValueExpr* stmt)
{
    const auto* sourceExpr = stmt->getSourceExpr();

    if(const auto& s = FindValue(mOpaqueValues, sourceExpr)) {
        mOutputFormatHelper.Append(s.value());

    } else {
        // Needs to be internal because a user can create the same type and it gets put into the stack frame
        std::string name{BuildSuspendVarName(stmt)};

        // The initial_suspend and final_suspend expressions carry the same location info. If we hit such a case,
        // make up another name.
        // Below is a std::find_if. However, the same code looks unreadable with std::find_if
        for(const auto lookupName{StrCat(CORO_FRAME_ACCESS, name)}; const auto& [k, v] : mOpaqueValues) {
            if(v == lookupName) {
                name += "_1"sv;
                break;
            }
        }

        const auto accessName{StrCat(CORO_FRAME_ACCESS, name)};
        mOpaqueValues.insert(std::make_pair(sourceExpr, accessName));

        OutputFormatHelper      ofm{};
        CoroutinesCodeGenerator codeGenerator{ofm, mPosBeforeFunc, mFSMName, mSuspendsCount, mASTData};

        auto*           promiseField = AddField(name, stmt->getType());
        BinaryOperator* assignPromiseSuspend =
            asthelpers::mkAssign(mASTData.mFrameAccessDeclRef, promiseField, stmt->getSourceExpr());

        codeGenerator.InsertArg(assignPromiseSuspend);
        ofm.AppendSemiNewLine();

        ofm.SetIndent(mOutputFormatHelper);

        mOutputFormatHelper.InsertAt(mPosBeforeSuspendExpr, ofm);
        mOutputFormatHelper.Append(accessName);
    }
}
//-----------------------------------------------------------------------------

std::string CoroutinesCodeGenerator::BuildResumeLabelName(int index) const
{
    return StrCat(RESUME_LABEL_PREFIX, "_"sv, mFSMName, "_"sv, index);
}
//-----------------------------------------------------------------------------

void CoroutinesCodeGenerator::InsertArg(const CoroutineSuspendExpr* stmt)
{
    auto& ctx = GetGlobalAST();

    mOutputFormatHelper.AppendNewLine();
    InsertInstantiationPoint(ctx.getSourceManager(), stmt->getKeywordLoc(), [&] {
        if(isa<CoawaitExpr>(stmt)) {
            return kwCoAwaitSpace;
        } else {
            return kwCoYieldSpace;
        }
    }());

    mPosBeforeSuspendExpr = mOutputFormatHelper.CurrentPos();

    /// Represents an expression that might suspend coroutine execution;
    /// either a co_await or co_yield expression.
    ///
    /// Evaluation of this expression first evaluates its 'ready' expression. If
    /// that returns 'false':
    ///  -- execution of the coroutine is suspended
    ///  -- the 'suspend' expression is evaluated
    ///     -- if the 'suspend' expression returns 'false', the coroutine is
    ///        resumed
    ///     -- otherwise, control passes back to the resumer.
    /// If the coroutine is not suspended, or when it is resumed, the 'resume'
    /// expression is evaluated, and its result is the result of the overall
    /// expression.

    // mOutputFormatHelper.AppendNewLine("// __builtin_coro_save() // frame->suspend_index = n");

    // For why, see the implementation of CoroutinesCodeGenerator::InsertArg(const ImplicitCastExpr* stmt)
    mSupressCasts = true;

    auto* il  = asthelpers::mkIntegerLiteral32(++mSuspendsCount);
    auto* bop = asthelpers::mkBinaryOperator(
        mASTData.mSuspendIndexAccess, il, BO_Assign, mASTData.mSuspendIndexField->getType());

    // Find out whether the return type is void or bool. In case of bool, we need to insert an if-statement, to
    // suspend only, if the return value was true.
    // Technically only void, bool, or std::coroutine_handle<Z> is allowed. [expr.await] p3.7
    const bool returnsVoid{stmt->getSuspendExpr()->getType()->isVoidType()};

    // XXX: check if getResumeExpr is marked noexcept. Otherwise we need additional expcetion handling?
    // CGCoroutine.cpp:229

    StmtsContainer bodyStmts{};
    Expr*          initializeInitialAwaitResume = nullptr;

    auto addInitialAwaitSuspendCalled = [&] {
        if(eState::FinalSuspend != mState) {
            bodyStmts.Add(bop);

            if(eState::InitialSuspend == mState) {
                mState = eState::Body;
                // https://timsong-cpp.github.io/cppwp/n4861/dcl.fct.def.coroutine#5.3
                initializeInitialAwaitResume = asthelpers::mkAssign(mASTData.mFrameAccessDeclRef,
                                                                    mASTData.mInitialAwaitResumeCalledField,
                                                                    asthelpers::mkBoolLiteral(true));
                bodyStmts.Add(initializeInitialAwaitResume);
            }
        }
    };

    if(returnsVoid) {
        bodyStmts.Add(stmt->getSuspendExpr());
        addInitialAwaitSuspendCalled();

        if(eState::FinalSuspend != mState) {
            bodyStmts.Add(asthelpers::mkReturnStmt());
        }

        auto* ifReady = asthelpers::mkNotIfStmt(stmt->getReadyExpr(), bodyStmts);

        InsertArg(ifReady);

    } else {
        addInitialAwaitSuspendCalled();

        if(eState::FinalSuspend != mState) {
            bodyStmts.Add(asthelpers::mkReturnStmt());
        }

        auto* ifSuspend = asthelpers::mkIfStmt(stmt->getSuspendExpr(), bodyStmts);
        auto* ifReady   = asthelpers::mkNotIfStmt(stmt->getReadyExpr(), ifSuspend);

        InsertArg(ifReady);
    }

    if(not returnsVoid and initializeInitialAwaitResume) {
        // At this point we technically haven't called initial suspend
        InsertArgWithNull(initializeInitialAwaitResume);
        mOutputFormatHelper.AppendNewLine();
    }

    RETURN_IF(eState::FinalSuspend == mState);

    auto* suspendLabel = asthelpers::mkLabelStmt(BuildResumeLabelName(mSuspendsCount));
    InsertArg(suspendLabel);

    const auto* resumeExpr = stmt->getResumeExpr();

    if(not resumeExpr->getType()->isVoidType()) {
        const auto* sourceExpr = stmt->getOpaqueValue()->getSourceExpr();

        if(const auto& s = FindValue(mOpaqueValues, sourceExpr)) {
            const auto fieldName{StrCat(std::string_view{s.value()}.substr(CORO_FRAME_ACCESS.size()), "_res"sv)};
            mOutputFormatHelper.Append(CORO_FRAME_ACCESS, fieldName, hlpAssing);

            auto* promiseField = AddField(fieldName, resumeExpr->getType());
        }
    }

    InsertArg(resumeExpr);
}
//-----------------------------------------------------------------------------

void CoroutinesCodeGenerator::InsertArg(const CoreturnStmt* stmt)
{
    InsertInstantiationPoint(GetGlobalAST().getSourceManager(), stmt->getKeywordLoc(), kwCoReturnSpace);

    if(stmt->getPromiseCall()) {
        InsertArg(stmt->getPromiseCall());

        if(stmt->isImplicit()) {
            mOutputFormatHelper.AppendComment("implicit"sv);
        }
    }
}
//-----------------------------------------------------------------------------

void CoroutinesCodeGenerator::InsertArgWithNull(const Stmt* stmt)
{
    InsertArg(stmt);
    InsertArg(asthelpers::mkNullStmt());
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
