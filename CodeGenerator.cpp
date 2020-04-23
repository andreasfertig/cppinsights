/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "CodeGenerator.h"
#include <algorithm>
#include <vector>
#include "ClangCompat.h"
#include "DPrint.h"
#include "Insights.h"
#include "InsightsBase.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "InsightsOnce.h"
#include "InsightsStrCat.h"
#include "NumberIterator.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/Path.h"
//-----------------------------------------------------------------------------

/// \brief Convenience macro to create a \ref LambdaScopeHandler on the stack.
#define LAMBDA_SCOPE_HELPER(type)                                                                                      \
    LambdaScopeHandler lambdaScopeHandler{mLambdaStack, mOutputFormatHelper, LambdaCallerType::type};
//-----------------------------------------------------------------------------

namespace clang::insights {

static const char* AccessToString(const AccessSpecifier& access)
{
    switch(access) {
        case AS_public: return "public";
        case AS_protected: return "protected";
        case AS_private: return "private";
        default: return "";
    }
}
//-----------------------------------------------------------------------------

static std::string AccessToStringWithColon(const AccessSpecifier& access)
{
    std::string accessStr = AccessToString(access);
    if(!accessStr.empty()) {
        accessStr += ": ";
    }

    return accessStr;
}
//-----------------------------------------------------------------------------

static std::string AccessToStringWithColon(const FunctionDecl& decl)
{
    return AccessToStringWithColon(decl.getAccess());
}
//-----------------------------------------------------------------------------

static const char* GetCastName(const CastKind castKind)
{
    if(CastKind::CK_BitCast == castKind) {
        return "reinterpret_cast";
    }

    return "static_cast";
}
//-----------------------------------------------------------------------------

static const char* GetClassOrStructTagName(const TagDecl& decl)
{
    const bool isClass{decl.isClass()};

    if(isClass) {
        return kwClassSpace;

    } else {
        return "struct ";
    }
}
//-----------------------------------------------------------------------------

class ArrayInitCodeGenerator final : public CodeGenerator
{
    const uint64_t mIndex;

public:
    ArrayInitCodeGenerator(OutputFormatHelper& _outputFormatHelper, const uint64_t index)
    : CodeGenerator{_outputFormatHelper}
    , mIndex{index}
    {
    }

    using CodeGenerator::InsertArg;
    void InsertArg(const ArrayInitIndexExpr*) override { mOutputFormatHelper.Append(mIndex); }
};
//-----------------------------------------------------------------------------

/// Handling specialties for decomposition declarations.
///
/// Decompositions declarations have no name. This class stores the made up name and returns it each time the anonymous
/// declaration is asked for a name.
class StructuredBindingsCodeGenerator final : public CodeGenerator
{
    std::string mVarName;

public:
    StructuredBindingsCodeGenerator(OutputFormatHelper& _outputFormatHelper, std::string&& varName)
    : CodeGenerator{_outputFormatHelper}
    , mVarName{std::move(varName)}
    {
    }

    using CodeGenerator::InsertArg;
    void InsertArg(const DeclRefExpr* stmt) override;

    /// Inserts the bindings of a decompositions declaration.
    void InsertDecompositionBindings(const DecompositionDecl& decompositionDeclStmt);
};
//-----------------------------------------------------------------------------

/// Handle using statements which pull functions ore members from a base class into the class.
class UsingCodeGenerator final : public CodeGenerator
{
public:
    UsingCodeGenerator(OutputFormatHelper& _outputFormatHelper)
    : CodeGenerator{_outputFormatHelper}
    {
    }

    using CodeGenerator::InsertArg;
    void InsertArg(const CXXMethodDecl* stmt) override
    {
        mOutputFormatHelper.Append("// ");

        InsertCXXMethodDecl(stmt, SkipBody::Yes);
    }

    void InsertArg(const FieldDecl* stmt) override
    {
        mOutputFormatHelper.Append("// ");
        CodeGenerator::InsertArg(stmt);
    }

    // makes no sense to insert the class when applying it to using
    void InsertArg(const CXXRecordDecl*) override {}

    // makes no sense to insert the typedef when applying it to using
    void InsertArg(const TypedefDecl*) override {}

protected:
    bool InsertNamespace() const override { return true; }
};
//-----------------------------------------------------------------------------

/// \brief A special code generator for Lambda init captures which use \c std::move
class LambdaInitCaptureCodeGenerator final : public CodeGenerator
{
public:
    explicit LambdaInitCaptureCodeGenerator(OutputFormatHelper& outputFormatHelper,
                                            LambdaStackType&    lambdaStack,
                                            std::string&        varName)
    : CodeGenerator{outputFormatHelper, lambdaStack}
    , mVarName{varName}
    {
    }

    using CodeGenerator::InsertArg;

    /// Replace every \c VarDecl with the given variable name. This cover init captures which introduce a new name.
    /// However, it means that _all_ VarDecl's will be changed.
    /// TODO: Check if it is really good to replace all VarDecl's
    void InsertArg(const DeclRefExpr* stmt) override
    {
        if(isa<VarDecl>(stmt->getDecl())) {
            mOutputFormatHelper.Append("_", mVarName);

        } else {

            CodeGenerator::InsertArg(stmt);
        }
    }

private:
    std::string& mVarName;  ///< The name of the variable that needs to be prefixed with _.
};
//-----------------------------------------------------------------------------

CodeGenerator::LambdaScopeHandler::LambdaScopeHandler(LambdaStackType&       stack,
                                                      OutputFormatHelper&    outputFormatHelper,
                                                      const LambdaCallerType lambdaCallerType)
: mStack{stack}
, mHelper{lambdaCallerType, GetBuffer(outputFormatHelper)}
{
    mStack.push(mHelper);
}
//-----------------------------------------------------------------------------

CodeGenerator::LambdaScopeHandler::~LambdaScopeHandler()
{
    if(!mStack.empty()) {
        mStack.pop()->finish();
    }
}
//-----------------------------------------------------------------------------

OutputFormatHelper& CodeGenerator::LambdaScopeHandler::GetBuffer(OutputFormatHelper& outputFormatHelper) const
{
    // Find the most outer element to place the lambda class definition. For example, if we have this:
    // Test( [&]() {} );
    // The lambda's class definition needs to be placed _before_ the CallExpr to Test.
    auto* element = [&]() -> LambdaHelper* {
        for(auto& l : mStack) {
            switch(l.callerType()) {
                case LambdaCallerType::CallExpr:
                case LambdaCallerType::VarDecl:
                case LambdaCallerType::ReturnStmt:
                case LambdaCallerType::OperatorCallExpr:
                case LambdaCallerType::MemberCallExpr:
                case LambdaCallerType::BinaryOperator:
                case LambdaCallerType::CXXMethodDecl: return &l;
                default: break;
            }
        }

        return nullptr;
    }();

    if(element) {
        return element->buffer();
    }

    return outputFormatHelper;
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXDependentScopeMemberExpr* stmt)
{
    if(not stmt->isImplicitAccess()) {
        InsertArg(stmt->getBase());
    } else {
        InsertNamespace(stmt->getQualifier());
    }

    const std::string op{[&] {
        if(stmt->isImplicitAccess()) {
            return "";
        }

        return stmt->isArrow() ? "->" : ".";
    }()};

    mOutputFormatHelper.Append(op, stmt->getMemberNameInfo().getAsString());
}
//-----------------------------------------------------------------------------

template<typename T>
static void AddStmt(std::vector<Stmt*>& v, const T& stmt)
{
    if(stmt) {
        v.push_back(const_cast<Stmt*>(static_cast<const Stmt*>(stmt)));
    }
}
//-----------------------------------------------------------------------------

static void AddBodyStmts(std::vector<Stmt*>& v, Stmt* body)
{
    if(auto* b = dyn_cast_or_null<CompoundStmt>(body)) {
        for(auto* st : b->children()) {
            v.push_back(st);
        }
    } else if(not isa<NullStmt>(body)) {
        v.push_back(body);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXForRangeStmt* rangeForStmt)
{
    auto&      langOpts{GetLangOpts(*rangeForStmt->getLoopVariable())};
    const bool onlyCpp11{not langOpts.CPlusPlus14};

    auto* rwStmt = const_cast<CXXForRangeStmt*>(rangeForStmt);

    std::vector<Stmt*> outerScopeStmts{};
    std::vector<Stmt*> bodyStmts{};

    // C++20 init-statement
    AddStmt(outerScopeStmts, rangeForStmt->getInit());

    // range statement
    AddStmt(outerScopeStmts, rangeForStmt->getRangeStmt());

    if(not onlyCpp11) {
        AddStmt(outerScopeStmts, rangeForStmt->getBeginStmt());
        AddStmt(outerScopeStmts, rangeForStmt->getEndStmt());
    }

    // add the loop variable to the body
    AddStmt(bodyStmts, rangeForStmt->getLoopVarStmt());

    // add the body itself, without the CompoundStmt
    AddBodyStmts(bodyStmts, rwStmt->getBody());

    const auto& ctx = rangeForStmt->getLoopVariable()->getASTContext();

    // In case of a range-based for-loop inside an unevaluated template the begin and end statements are not present. In
    // this case just add a nullptr.
    Decl* decls[2]{rwStmt->getBeginStmt() ? rwStmt->getBeginStmt()->getSingleDecl() : nullptr,
                   rwStmt->getEndStmt() ? rwStmt->getEndStmt()->getSingleDecl() : nullptr};

    auto dgRef = DeclGroupRef::Create(const_cast<ASTContext&>(ctx), decls, 2);

    auto* declStmt = [&]() -> DeclStmt* {
        if(onlyCpp11) {
            return new(ctx) DeclStmt(dgRef, rangeForStmt->getBeginLoc(), rangeForStmt->getEndLoc());
        }

        return nullptr;
    }();

    ArrayRef<Stmt*> innerScopeStmtsRef{bodyStmts};
    auto*           innerScope =
        CompoundStmt::Create(ctx, innerScopeStmtsRef, rangeForStmt->getBeginLoc(), rangeForStmt->getEndLoc());

    auto* forStmt = new(ctx) ForStmt(ctx,
                                     declStmt,
                                     rwStmt->getCond(),
                                     rwStmt->getLoopVariable(),
                                     rwStmt->getInc(),
                                     innerScope,
                                     rangeForStmt->getBeginLoc(),
                                     rangeForStmt->getEndLoc(),
                                     rangeForStmt->getEndLoc());

    AddStmt(outerScopeStmts, forStmt);

    ArrayRef<Stmt*> outerScopeStmtsRef{outerScopeStmts};
    auto*           outerScope =
        CompoundStmt::Create(ctx, outerScopeStmtsRef, rangeForStmt->getBeginLoc(), rangeForStmt->getEndLoc());

    InsertArg(outerScope);

    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

static void InsertQualifierAndName(const DeclarationName&     declName,
                                   const NestedNameSpecifier* qualifier,
                                   OutputFormatHelper&        outputFormatHelper)
{
    outputFormatHelper.Append(ScopeHandler::RemoveCurrentScope(GetNestedName(qualifier)), declName.getAsString());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertNamespace(const NestedNameSpecifier* stmt)
{
    mOutputFormatHelper.Append(ScopeHandler::RemoveCurrentScope(GetNestedName(stmt)));
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const UnresolvedLookupExpr* stmt)
{
    InsertQualifierAndName(stmt->getName(), stmt->getQualifier(), mOutputFormatHelper);

    if(stmt->getNumTemplateArgs()) {
        InsertTemplateArgs(*stmt);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const DependentScopeDeclRefExpr* stmt)
{
    if(stmt->hasTemplateKeyword()) {
        mOutputFormatHelper.Append("template ");
    }

    InsertQualifierAndName(stmt->getDeclName(), stmt->getQualifier(), mOutputFormatHelper);
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const VarTemplateDecl* stmt)
{
    const auto* templatedDecl = stmt->getTemplatedDecl();

    // Insert only the primary template here. The specializations are inserted via their instantiated
    // VarTemplateSpecializationDecl which resolved to a VarDecl. It looks like whether the variable has an initializer
    // or not can be used to distinguish between the primary template and one appearing in a templated class.
    if(not templatedDecl->hasInit()) {
        return;
    }

    // VarTemplatedDecl's can have lambdas as initializers. Push a VarDecl on the stack, otherwise the lambda would
    // appear in the middle of template<....> and the variable itself.
    LAMBDA_SCOPE_HELPER(VarDecl);

    InsertTemplateParameters(*stmt->getTemplateParameters());
    InsertArg(templatedDecl);

    OnceTrue first{};
    for(const auto* spec : stmt->specializations()) {
        if(TSK_ExplicitSpecialization != spec->getSpecializationKind()) {
            if(first) {
                mOutputFormatHelper.AppendNewLine();
            }

            InsertArg(spec);
        }
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ConditionalOperator* stmt)
{
    InsertArg(stmt->getCond());
    mOutputFormatHelper.Append(" ? ");
    InsertArg(stmt->getLHS());
    mOutputFormatHelper.Append(" : ");
    InsertArg(stmt->getRHS());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const DoStmt* stmt)
{
    mOutputFormatHelper.Append("do ");
    const auto* body = stmt->getBody();
    InsertArg(body);

    if(isa<CompoundStmt>(body)) {
        mOutputFormatHelper.Append(' ');
    } else if(!isa<NullStmt>(body)) {
        mOutputFormatHelper.Append("; ");
    }

    mOutputFormatHelper.Append("while");
    WrapInParens([&]() { InsertArg(stmt->getCond()); }, AddSpaceAtTheEnd::No);

    mOutputFormatHelper.AppendSemiNewLine();
    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CaseStmt* stmt)
{
    mOutputFormatHelper.Append("case ");
    InsertArg(stmt->getLHS());
    // TODO what is getRHS??
    mOutputFormatHelper.Append(": ");
    InsertArg(stmt->getSubStmt());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const BreakStmt* /*stmt*/)
{
    mOutputFormatHelper.Append("break");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const DefaultStmt* stmt)
{
    mOutputFormatHelper.Append("default: ");
    InsertArg(stmt->getSubStmt());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ContinueStmt* /*stmt*/)
{
    mOutputFormatHelper.Append("continue");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const GotoStmt* stmt)
{
    mOutputFormatHelper.Append("goto ");
    InsertArg(stmt->getLabel());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const LabelStmt* stmt)
{
    mOutputFormatHelper.AppendNewLine(stmt->getName(), ":");

    if(stmt->getSubStmt()) {
        InsertArg(stmt->getSubStmt());
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const SwitchStmt* stmt)
{
    const bool hasInit{stmt->getInit() || stmt->getConditionVariable()};

    if(hasInit) {
        mOutputFormatHelper.OpenScope();

        if(const auto* conditionVar = stmt->getConditionVariable()) {
            InsertArg(conditionVar);
        }

        if(const auto* init = stmt->getInit()) {
            InsertArg(init);
        }
    }

    mOutputFormatHelper.Append("switch");

    WrapInParens([&]() { InsertArg(stmt->getCond()); }, AddSpaceAtTheEnd::Yes);

    InsertArg(stmt->getBody());

    if(hasInit) {
        mOutputFormatHelper.CloseScope();
    }

    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const WhileStmt* stmt)
{
    {
        // We need to handle the case that a lambda is used in the init-statement of the for-loop.
        LAMBDA_SCOPE_HELPER(VarDecl);

        mOutputFormatHelper.Append("while");
        WrapInParens([&]() { InsertArg(stmt->getCond()); }, AddSpaceAtTheEnd::Yes);
    }

    const auto* body = stmt->getBody();
    const bool  hasCompoundStmt{isa<CompoundStmt>(body)};

    InsertArg(body);

    if(hasCompoundStmt) {
        mOutputFormatHelper.AppendNewLine();
    } else {
        const bool isBodyBraced = isa<CompoundStmt>(body);
        if(!isBodyBraced) {
            mOutputFormatHelper.AppendSemiNewLine();
        }
    }

    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

/// Get the name of a \c FieldDecl in case this \c FieldDecl is part of a lambda. The name has to be retrieved from the
/// capture fields or can be \c __this.
static Optional<std::string> GetFieldDeclNameForLambda(const FieldDecl& fieldDecl, const CXXRecordDecl& cxxRecordDecl)
{
    if(cxxRecordDecl.isLambda()) {
        llvm::DenseMap<const VarDecl*, FieldDecl*> captures{};
        FieldDecl*                                 thisCapture{};

        cxxRecordDecl.getCaptureFields(captures, thisCapture);

        if(&fieldDecl == thisCapture) {
            return {"__this"};
        } else {
            for(const auto& [key, value] : captures) {
                if(&fieldDecl == value) {
                    return GetName(*key);
                }
            }
        }
    }

    return {};
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const MemberExpr* stmt)
{
    const auto* base = stmt->getBase();
    bool        skipBase{false};
    if(const auto* implicitCast = dyn_cast_or_null<ImplicitCastExpr>(base)) {
        if(CastKind::CK_UncheckedDerivedToBase == implicitCast->getCastKind()) {
            const auto* castDest = implicitCast->IgnoreImpCasts();

            // if this calls a protected function we cannot cast it to the base, this would not compile
            if(isa<CXXThisExpr>(castDest)) {
                skipBase = true;
            }
        }
    }

    if(skipBase) {
        mOutputFormatHelper.Append("/* ");
    }

    InsertArg(base);

    const std::string op{stmt->isArrow() ? "->" : "."};
    const auto*       meDecl = stmt->getMemberDecl();
    bool              skipTemplateArgs{false};
    const auto        name = [&]() -> std::string {
        // Handle a special case where we have a lambda static invoke operator. In that case use the appropriate
        // using retType as return type
        if(const auto* m = dyn_cast_or_null<CXXMethodDecl>(meDecl)) {
            if(const auto* rd = m->getParent(); rd && rd->isLambda()) {
                skipTemplateArgs = true;

                return StrCat("operator ", GetLambdaName(*rd), "::", BuildRetTypeName(*rd));
            }
        }

        // This is at least the case for lambdas, where members are created by capturing a structured binding. See #181.
        else if(const auto* fd = dyn_cast_or_null<FieldDecl>(meDecl)) {
            if(const auto* cxxRecordDecl = dyn_cast_or_null<CXXRecordDecl>(fd->getParent())) {
                if(const auto& fieldName = GetFieldDeclNameForLambda(*fd, *cxxRecordDecl)) {
                    return fieldName.getValue();
                }
            }
        }

        return stmt->getMemberNameInfo().getName().getAsString();
    }();

    mOutputFormatHelper.Append(op);

    if(skipBase) {
        mOutputFormatHelper.Append(" */ ");
    }

    mOutputFormatHelper.Append(name);

    if(!skipTemplateArgs) {
        if(const auto cxxMethod = dyn_cast_or_null<CXXMethodDecl>(meDecl)) {
            if(const auto* tmplArgs = cxxMethod->getTemplateSpecializationArgs()) {
                OutputFormatHelper ofm{};

                ofm.Append('<');

                bool      haveArg{false};
                OnceFalse needsComma{};
                for(const auto& arg : tmplArgs->asArray()) {

                    if(arg.getKind() == TemplateArgument::Integral) {
                        ofm.AppendComma(needsComma);

                        ofm.Append(arg.getAsIntegral());
                        haveArg = true;
                    } else {

                        break;
                    }
                }

                if(haveArg) {
                    mOutputFormatHelper.Append(ofm.GetString(), ">");

                } else {
                    InsertTemplateArgs(*tmplArgs);
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const UnaryExprOrTypeTraitExpr* stmt)
{
    mOutputFormatHelper.Append(GetKind(*stmt));

    if(!stmt->isArgumentType()) {
        const auto* argExpr = stmt->getArgumentExpr();
        const bool  needsParens{!isa<ParenExpr>(argExpr)};

        WrapInParensIfNeeded(needsParens, [&] { InsertArg(argExpr); });

    } else {
        WrapInParens([&] { mOutputFormatHelper.Append(GetName(stmt->getTypeOfArgument())); });
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const IntegerLiteral* stmt)
{
    const auto& type     = stmt->getType();
    const bool  isSigned = type->isSignedIntegerType();

    mOutputFormatHelper.Append(stmt->getValue().toString(10, isSigned));
    InsertSuffix(type);
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const FloatingLiteral* stmt)
{
    // FIXME: not working correctly
    mOutputFormatHelper.Append(EvaluateAsFloat(*stmt));
    InsertSuffix(stmt->getType());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXTypeidExpr* stmt)
{
    mOutputFormatHelper.Append("typeid");
    WrapInParens([&]() {
        if(stmt->isTypeOperand()) {
            mOutputFormatHelper.Append(GetName(stmt->getType()));
        } else {
            InsertArg(stmt->getExprOperand());
        }
    });
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const BinaryOperator* stmt)
{
    LAMBDA_SCOPE_HELPER(BinaryOperator);

    const bool needLHSParens{isa<BinaryOperator>(stmt->getLHS()->IgnoreImpCasts())};
    WrapInParensIfNeeded(needLHSParens, [&] { InsertArg(stmt->getLHS()); });

    mOutputFormatHelper.Append(" ", stmt->getOpcodeStr(), " ");

    const bool needRHSParens{isa<BinaryOperator>(stmt->getRHS()->IgnoreImpCasts())};
    WrapInParensIfNeeded(needRHSParens, [&] { InsertArg(stmt->getRHS()); });
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXRewrittenBinaryOperator* stmt)
{
    LAMBDA_SCOPE_HELPER(BinaryOperator);

    InsertArg(stmt->getSemanticForm());
}
//-----------------------------------------------------------------------------

static const char* GetStorageClassAsString(const StorageClass& sc)
{
    if(SC_None != sc) {
        return VarDecl::getStorageClassSpecifierString(sc);
    }

    return "";
}
//-----------------------------------------------------------------------------

static std::string GetStorageClassAsStringWithSpace(const StorageClass& sc)
{
    std::string ret{GetStorageClassAsString(sc)};

    if(!ret.empty()) {
        ret.append(" ");
    }

    return ret;
}
//-----------------------------------------------------------------------------

static std::string GetQualifiers(const VarDecl& vd)
{
    std::string qualifiers{};

    if(vd.isInline() || vd.isInlineSpecified()) {
        qualifiers += "inline ";
    }

    qualifiers += GetStorageClassAsStringWithSpace(vd.getStorageClass());

    if(vd.isConstexpr()) {
        qualifiers += "constexpr ";
    }

    return qualifiers;
}
//-----------------------------------------------------------------------------

static std::string FormatVarTemplateSpecializationDecl(const Decl* decl, std::string&& defaultName)
{
    std::string name{std::move(defaultName)};

    if(const auto* tvd = dyn_cast_or_null<VarTemplateSpecializationDecl>(decl)) {
        OutputFormatHelper outputFormatHelper{};
        CodeGenerator      codeGenerator{outputFormatHelper};

        codeGenerator.InsertTemplateArgs(tvd->getTemplateArgs());

        name += outputFormatHelper.GetString();
    }

    return name;
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const VarDecl* stmt)
{
    LAMBDA_SCOPE_HELPER(VarDecl);
    UpdateCurrentPos();

    if(InsertComma()) {
        mOutputFormatHelper.Append(',');
    }

    if(isa<VarTemplateSpecializationDecl>(stmt)) {
        InsertTemplateSpecializationHeader();
    }

    InsertAttributes(stmt->attrs());

    if(IsTrivialStaticClassVarDecl(*stmt)) {
        HandleLocalStaticNonTrivialClass(stmt);

    } else {
        if(InsertVarDecl()) {
            mOutputFormatHelper.Append(GetQualifiers(*stmt));

            if(const auto type = GetDesugarType(stmt->getType());
               type->isFunctionPointerType() || isa<MemberPointerType>(type.getTypePtrOrNull())) {
                const auto        lineNo = GetSM(*stmt).getSpellingLineNumber(stmt->getSourceRange().getBegin());
                const std::string funcPtrName{StrCat("FuncPtr_", lineNo, " ")};

                mOutputFormatHelper.AppendNewLine("using ", funcPtrName, "= ", GetName(type), ";");
                mOutputFormatHelper.Append(funcPtrName, GetName(*stmt));

            } else {
                const auto varName = FormatVarTemplateSpecializationDecl(stmt, GetName(*stmt));

                // TODO: to keep the special handling for lambdas, do this only for template specializations
                if(stmt->getType()->getAs<TemplateSpecializationType>()) {
                    mOutputFormatHelper.Append(GetNameAsWritten(stmt->getType()), " ", varName);

                } else {
                    mOutputFormatHelper.Append(GetTypeNameAsParameter(stmt->getType(), varName));
                }
            }
        } else {
            const std::string pointer = [&]() {
                if(stmt->getType()->isAnyPointerType()) {
                    return " *";
                }
                return " ";
            }();

            mOutputFormatHelper.Append(pointer, GetName(*stmt));
        }

        if(stmt->hasInit()) {
            mOutputFormatHelper.Append(" = ");

            InsertArg(stmt->getInit());
        };

        if(stmt->isNRVOVariable()) {
            mOutputFormatHelper.Append(" /* NRVO variable */");
        }

        if(InsertSemi()) {
            mOutputFormatHelper.AppendSemiNewLine();
        }

        // Insert the bindings of a DecompositionDecl if this VarDecl is a DecompositionDecl.
        if(const auto* decompDecl = dyn_cast_or_null<DecompositionDecl>(stmt)) {
            StructuredBindingsCodeGenerator codeGenerator{mOutputFormatHelper, GetName(*stmt)};

            codeGenerator.InsertDecompositionBindings(*decompDecl);
        }
    }
}
//-----------------------------------------------------------------------------

bool CodeGenerator::InsertLambdaStaticInvoker(const CXXMethodDecl* cxxMethodDecl)
{
    if(cxxMethodDecl && cxxMethodDecl->isLambdaStaticInvoker()) {
        mOutputFormatHelper.AppendNewLine();

        const auto* lambda = cxxMethodDecl->getParent();
        const auto* callOp = lambda->getLambdaCallOperator();
        if(lambda->isGenericLambda() && cxxMethodDecl->isFunctionTemplateSpecialization()) {
            const TemplateArgumentList* tal            = cxxMethodDecl->getTemplateSpecializationArgs();
            FunctionTemplateDecl*       callOpTemplate = callOp->getDescribedFunctionTemplate();
            void*                       insertPos      = nullptr;
            FunctionDecl*               correspondingCallOpSpecialization =
                callOpTemplate->findSpecialization(tal->asArray(), insertPos);
            callOp = cast<CXXMethodDecl>(correspondingCallOpSpecialization);
        }

        InsertArg(callOp->getBody());
        mOutputFormatHelper.AppendNewLine();

        return true;
    }

    return false;
}
//-----------------------------------------------------------------------------

/// \brief Inserts the instantiation point of a template.
//
// This reveals at which place the template is first used.
static void
InsertInstantiationPoint(OutputFormatHelper& outputFormatHelper, const SourceManager& sm, const SourceLocation& instLoc)
{
    const auto  lineNo = sm.getSpellingLineNumber(instLoc);
    const auto& fileId = sm.getFileID(instLoc);
    const auto* file   = sm.getFileEntryForID(fileId);
    if(file) {
        const auto fileWithDirName = file->getName();
        const auto fileName        = llvm::sys::path::filename(fileWithDirName);

        outputFormatHelper.AppendNewLine("/* First instantiated from: ", fileName, ":", lineNo, " */");
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertTemplateGuardBegin(const FunctionDecl* stmt)
{
    if(stmt->isTemplateInstantiation() && stmt->isFunctionTemplateSpecialization()) {
        InsertInstantiationPoint(mOutputFormatHelper, GetSM(*stmt), stmt->getPointOfInstantiation());
        mOutputFormatHelper.AppendNewLine("#ifdef INSIGHTS_USE_TEMPLATE");
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertTemplateGuardEnd(const FunctionDecl* stmt)
{
    if(stmt->isTemplateInstantiation() && stmt->isFunctionTemplateSpecialization()) {
        mOutputFormatHelper.AppendNewLine("#endif");
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CoroutineBodyStmt* stmt)
{
    InsertArg(stmt->getBody());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CoroutineSuspendExpr* stmt)
{
    //	co_await or co_yield
    if(isa<CoyieldExpr>(stmt)) {
        mOutputFormatHelper.Append("co_yield ");
    } else {
        mOutputFormatHelper.Append("co_await ");
    }

    // peal of __promise.yield_value
    if(const auto* matTemp = dyn_cast_or_null<MaterializeTemporaryExpr>(stmt->getCommonExpr())) {
        const auto* temporary = GetTemporary(matTemp);

        if(const auto* memExpr = dyn_cast_or_null<CXXMemberCallExpr>(temporary)) {
            ForEachArg(memExpr->arguments(), [&](const auto& arg) { InsertArg(arg); });

            // Seems to be the path for a co_await expr
        } else {
            InsertArg(temporary);
        }
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CoreturnStmt* stmt)
{
    mOutputFormatHelper.Append("co_return ");
    InsertArg(stmt->getOperand());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const FunctionDecl* stmt)
{
    //    LAMBDA_SCOPE_HELPER(VarDecl);

    if(const auto* deductionGuide = dyn_cast_or_null<CXXDeductionGuideDecl>(stmt)) {
        InsertArg(deductionGuide);
    } else if(const auto* ctor = dyn_cast_or_null<CXXConstructorDecl>(stmt)) {
        InsertArg(ctor);
    } else {
        // skip a case at least in lambdas with a templated conversion operator which is not used and has auto
        // return type. This is hard to build with using.
        if(isa<CXXConversionDecl>(stmt) && not stmt->hasBody()) {
            return;
        }

        InsertTemplateGuardBegin(stmt);
        InsertAccessModifierAndNameWithReturnType(*stmt, SkipAccess::Yes);

        if(not InsertLambdaStaticInvoker(dyn_cast_or_null<CXXMethodDecl>(stmt))) {
            if(stmt->doesThisDeclarationHaveABody()) {
                mOutputFormatHelper.AppendNewLine();
                InsertArg(stmt->getBody());
                mOutputFormatHelper.AppendNewLine();
            } else {
                mOutputFormatHelper.AppendSemiNewLine();
            }
        }

        InsertTemplateGuardEnd(stmt);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertTemplateParameters(const TemplateParameterList& list)
{
    mOutputFormatHelper.Append("template<");

    OnceFalse needsComma{};
    for(const auto* param : list) {
        mOutputFormatHelper.AppendComma(needsComma);

        const auto& typeName = GetName(*param);

        if(const auto* tt = dyn_cast_or_null<TemplateTypeParmDecl>(param)) {
            if(tt->wasDeclaredWithTypename()) {
                mOutputFormatHelper.Append("typename ");
            } else {
                mOutputFormatHelper.Append("class ");
            }

            if(tt->isParameterPack()) {
                mOutputFormatHelper.Append("... ");
            }

            if(0 == typeName.size() || tt->isImplicit() /* fixes class container:auto*/) {
                mOutputFormatHelper.Append("type_parameter_", tt->getDepth(), "_", tt->getIndex());
            } else {
                mOutputFormatHelper.Append(typeName);
            }

            if(tt->hasDefaultArgument()) {
                mOutputFormatHelper.Append(" = ", GetName(tt->getDefaultArgument()));
            }
        } else if(const auto* nonTmplParam = dyn_cast_or_null<NonTypeTemplateParmDecl>(param)) {

            mOutputFormatHelper.Append(GetName(nonTmplParam->getType()), " ");
            if(nonTmplParam->isParameterPack()) {
                mOutputFormatHelper.Append("... ");
            }

            mOutputFormatHelper.Append(typeName);

            if(nonTmplParam->hasDefaultArgument()) {
                mOutputFormatHelper.Append(" = ");
                InsertArg(nonTmplParam->getDefaultArgument());
            }
        } else if(const auto* tmplTmplParam = dyn_cast_or_null<TemplateTemplateParmDecl>(param)) {
            mOutputFormatHelper.Append("template <typename> typename ", typeName);

            if(tmplTmplParam->hasDefaultArgument()) {
                mOutputFormatHelper.Append(" = ");
                InsertTemplateArg(tmplTmplParam->getDefaultArgument().getArgument());
            }
        }
    }

    mOutputFormatHelper.AppendNewLine(">");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ClassTemplateDecl* stmt)
{
    InsertTemplateParameters(*stmt->getTemplateParameters());
    InsertArg(stmt->getTemplatedDecl());

    for(const auto* spec : stmt->specializations()) {
        // Explicit specializations will appear later in the AST as dedicated node. Don't generate code for them
        // now, otherwise they are there twice.
        if(TSK_ExplicitSpecialization != spec->getSpecializationKind()) {
            InsertArg(spec);
        }
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ParenListExpr* stmt)
{
    OnceFalse needsComma{};

    for(const auto& expr : stmt->children()) {
        mOutputFormatHelper.AppendComma(needsComma);

        InsertArg(expr);
    }
}
//-----------------------------------------------------------------------------

/// Fill the values of a constant array.
///
/// This is either called by \c InitListExpr (which may contain an offset, as the user already provided certain
/// values) or by \c GetValueOfValueInit.
std::string
CodeGenerator::FillConstantArray(const ConstantArrayType* ct, const std::string& value, const uint64_t startAt)
{
    OutputFormatHelper ret{};

    if(ct) {
        const auto size{std::clamp(ct->getSize().getZExtValue(), uint64_t{0}, MAX_FILL_VALUES_FOR_ARRAYS)};

        OnceFalse needsComma{uint64_t{0} != startAt};
        for_each(startAt, size, [&](auto) {
            ret.AppendComma(needsComma);
            ret.Append(value);
        });
    }

    return ret.GetString();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const InitListExpr* stmt)
{
    WrapInCurlys([&]() {
        mOutputFormatHelper.IncreaseIndent();

        ForEachArg(stmt->inits(), [&](const auto& init) { InsertArg(init); });

        // If we have a filler, fill the rest of the array with the filler expr.
        if(const auto* filler = stmt->getArrayFiller()) {
            OutputFormatHelper ofm{};
            CodeGenerator      codeGenerator{ofm};
            codeGenerator.InsertArg(filler);

            const auto ret = FillConstantArray(dyn_cast_or_null<ConstantArrayType>(stmt->getType().getTypePtrOrNull()),
                                               ofm.GetString(),
                                               stmt->getNumInits());

            mOutputFormatHelper.Append(ret);
        }
    });

    mOutputFormatHelper.DecreaseIndent();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXDefaultInitExpr* stmt)
{
    const auto* subExpr = stmt->getExpr();

    InsertCurlysIfRequired(subExpr);
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXDeleteExpr* stmt)
{
    mOutputFormatHelper.Append("delete");

    if(stmt->isArrayForm()) {
        mOutputFormatHelper.Append("[]");
    }

    mOutputFormatHelper.Append(' ');

    InsertArg(stmt->getArgument());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXConstructExpr* stmt)
{
    mOutputFormatHelper.Append(GetName(GetDesugarType(stmt->getType()), Unqualified::Yes));

    const BraceKind braceKind = [&]() {
        if(stmt->isListInitialization()) {
            return BraceKind::Curlys;
        }
        return BraceKind::Parens;
    }();

    WrapInParensOrCurlys(braceKind, [&]() {
        if(stmt->getNumArgs()) {
            ForEachArg(stmt->arguments(), [&](const auto& arg) { InsertArg(arg); });
        }
    });
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXUnresolvedConstructExpr* stmt)
{
    mOutputFormatHelper.Append(GetName(GetDesugarType(stmt->getType()), Unqualified::Yes));

    const BraceKind braceKind = [&]() {
        if(stmt->isListInitialization()) {
            return BraceKind::Curlys;
        }
        return BraceKind::Parens;
    }();

    WrapInParensOrCurlys(braceKind, [&]() {
        if(stmt->arg_size()) {
            ForEachArg(stmt->arguments(), [&](const auto& arg) { InsertArg(arg); });
        }
    });
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const UnresolvedMemberExpr* stmt)
{
    // InsertArg(stmt->getBase());
    const std::string op{};  // stmt->isArrow() ? "->" : "."};

    mOutputFormatHelper.Append(op, stmt->getMemberNameInfo().getAsString());

    if(stmt->getNumTemplateArgs()) {
        InsertTemplateArgs(*stmt);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const PackExpansionExpr* stmt)
{
    InsertArg(stmt->getPattern());
    mOutputFormatHelper.Append("... ");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXFoldExpr* stmt)
{
    const char* operatorStr = [&]() {
        switch(stmt->getOperator()) {
#define BINARY_OPERATION(Name, Spelling)                                                                               \
    case BO_##Name:                                                                                                    \
        return Spelling;

#include "clang/AST/OperationKinds.def"
#undef BINARY_OPERATION
        }

        return "";
    }();

    WrapInParens([&] {
        // We have a binary NNN fold. If init is nullptr, then it is a unary NNN fold.
        const auto* init = stmt->getInit();

        if(stmt->isLeftFold()) {
            if(init) {
                InsertArg(init);
                mOutputFormatHelper.Append(" ", operatorStr, " ");
            }

            mOutputFormatHelper.Append("... ", operatorStr, " ");
        }

        InsertArg(stmt->getPattern());

        if(stmt->isRightFold()) {
            mOutputFormatHelper.Append(" ", operatorStr, " ...");

            if(init) {
                mOutputFormatHelper.Append(" ", operatorStr, " ");
                InsertArg(init);
            }
        }
    });
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXInheritedCtorInitExpr* stmt)
{
    const auto& constructorDecl = *stmt->getConstructor();

    mOutputFormatHelper.Append(GetName(GetDesugarType(stmt->getType()), Unqualified::Yes));
    WrapInParens([&]() {
        mOutputFormatHelper.AppendParameterList(constructorDecl.parameters(), OutputFormatHelper::NameOnly::Yes);
    });
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXMemberCallExpr* stmt)
{
    LAMBDA_SCOPE_HELPER(MemberCallExpr);

    InsertArg(stmt->getCallee());

    WrapInParens([&]() { ForEachArg(stmt->arguments(), [&](const auto& arg) { InsertArg(arg); }); });
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ParenExpr* stmt)
{
    WrapInParens([&]() { InsertArg(stmt->getSubExpr()); });
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const UnaryOperator* stmt)
{
    const StringRef opCodeName = UnaryOperator::getOpcodeStr(stmt->getOpcode());
    const bool      insertBefore{!stmt->isPostfix()};

    if(insertBefore) {
        mOutputFormatHelper.Append(opCodeName);
    }

    InsertArg(stmt->getSubExpr());

    if(!insertBefore) {
        mOutputFormatHelper.Append(opCodeName);
    }
}
//-------------	----------------------------------------------------------------

void CodeGenerator::InsertArg(const StringLiteral* stmt)
{
    StringStream stream{};
    stmt->outputString(stream);

    mOutputFormatHelper.Append(stream.str());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ArrayInitIndexExpr* stmt)
{
    Error(stmt, "ArrayInitIndexExpr should not be reached in CodeGenerator");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ArraySubscriptExpr* stmt)
{
    if((not GetInsightsOptions().UseAltArraySubscriptionSyntax) || stmt->getLHS()->isLValue()) {
        InsertArg(stmt->getLHS());

        mOutputFormatHelper.Append('[');
        InsertArg(stmt->getRHS());
        mOutputFormatHelper.Append(']');
    } else {

        mOutputFormatHelper.Append("(*(");
        InsertArg(stmt->getLHS());
        mOutputFormatHelper.Append(" + ");

        InsertArg(stmt->getRHS());
        mOutputFormatHelper.Append("))");
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ArrayInitLoopExpr* stmt)
{
    WrapInCurlys([&]() {
        const uint64_t size = stmt->getArraySize().getZExtValue();

        ForEachArg(NumberIterator(size), [&](const auto& i) {
            ArrayInitCodeGenerator codeGenerator{mOutputFormatHelper, i};
            codeGenerator.InsertArg(stmt->getSubExpr());
        });
    });
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const OpaqueValueExpr* stmt)
{
    InsertArg(stmt->getSourceExpr());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CallExpr* stmt)
{
    LAMBDA_SCOPE_HELPER(CallExpr);
    UpdateCurrentPos();

    InsertArg(stmt->getCallee());

    if(isa<UserDefinedLiteral>(stmt)) {
        if(const auto* declRefExpr = dyn_cast_or_null<DeclRefExpr>(stmt->getCallee()->IgnoreImpCasts())) {
            if(const auto* fd = dyn_cast_or_null<FunctionDecl>(declRefExpr->getDecl())) {
                InsertTemplateArgs(*fd);
            }
        }
    }

    WrapInParens([&]() { ForEachArg(stmt->arguments(), [&](const auto& arg) { InsertArg(arg); }); });
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXNamedCastExpr* stmt)
{
    const QualType castDestType = stmt->getTypeAsWritten();
    const Expr*    subExpr      = stmt->getSubExpr();

    FormatCast(stmt->getCastName(), castDestType, subExpr, stmt->getCastKind());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ImplicitCastExpr* stmt)
{
    const Expr* subExpr  = stmt->getSubExpr();
    const auto  castKind = stmt->getCastKind();
    const bool  hideImplicitCasts{not GetInsightsOptions().ShowAllImplicitCasts};

    auto isMatchingCast = [](const CastKind kind, const bool hideImplicitCasts) {
        switch(kind) {
            case CastKind::CK_Dependent: [[fallthrough]];
            case CastKind::CK_IntegralCast: [[fallthrough]];
            case CastKind::CK_IntegralToBoolean: [[fallthrough]];
            case CastKind::CK_IntegralToPointer: [[fallthrough]];
            case CastKind::CK_PointerToIntegral: [[fallthrough]];
            case CastKind::CK_BitCast: [[fallthrough]];
            case CastKind::CK_UncheckedDerivedToBase: [[fallthrough]];
            case CastKind::CK_ToUnion: [[fallthrough]];
            case CastKind::CK_UserDefinedConversion: [[fallthrough]];
            case CastKind::CK_AtomicToNonAtomic: [[fallthrough]];
            case CastKind::CK_DerivedToBase: [[fallthrough]];
            case CastKind::CK_FloatingCast: [[fallthrough]];
            case CastKind::CK_IntegralToFloating: [[fallthrough]];
            case CastKind::CK_FloatingToIntegral: [[fallthrough]];
            case CastKind::CK_NonAtomicToAtomic: return true;
            default:
                // Show this casts only if ShowAllImplicitCasts is turned on.
                if(not hideImplicitCasts) {
                    switch(kind) {
                        case CastKind::CK_NullToPointer: [[fallthrough]];
                        case CastKind::CK_NullToMemberPointer: [[fallthrough]];
                        /* these are implicit conversions. We get them right, but they may end up in a compiler internal
                         * type, which leads to compiler errors */
                        case CastKind::CK_NoOp: [[fallthrough]];
                        case CastKind::CK_ArrayToPointerDecay: return true;
                        default: break;
                    }
                }

                return false;
        }
    };

    if(not isMatchingCast(castKind, hideImplicitCasts)) {
        InsertArg(subExpr);
    } else if(isa<IntegerLiteral>(subExpr) && hideImplicitCasts) {
        InsertArg(stmt->IgnoreCasts());

        // If this is part of an explicit cast, for example a CStyleCast or static_cast, ignore it, because it belongs
        // to the cast written by the user.
    } else if(stmt->isPartOfExplicitCast()) {
        InsertArg(stmt->IgnoreCasts());

    } else {
        static const std::string castName{GetCastName(castKind)};
        const QualType           castDestType{stmt->getType().getCanonicalType()};

        FormatCast(castName, castDestType, subExpr, castKind);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const DeclRefExpr* stmt)
{
    const auto* ctx = stmt->getDecl()->getDeclContext();

    if(!ctx->isFunctionOrMethod() && not isa<NonTypeTemplateParmDecl>(stmt->getDecl())) {
        OutputFormatHelper ofm{};
        CodeGenerator      codeGenerator{ofm};

        codeGenerator.ParseDeclContext(ctx);

        mOutputFormatHelper.Append(ScopeHandler::RemoveCurrentScope(ofm.GetString()), GetPlainName(*stmt));

    } else {
        mOutputFormatHelper.Append(GetName(*stmt));
    }

    InsertTemplateArgs(*stmt);
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CompoundStmt* stmt)
{
    mOutputFormatHelper.OpenScope();

    HandleCompoundStmt(stmt);

    mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::Yes);
}
//-----------------------------------------------------------------------------

template<typename... Args>
static bool IsStmtRequieringSemi(const Stmt* stmt)
{
    return (... && !isa<Args>(stmt));
}
//-----------------------------------------------------------------------------

void CodeGenerator::HandleCompoundStmt(const CompoundStmt* stmt)
{
    for(const auto* item : stmt->body()) {
        InsertArg(item);

        if(IsStmtRequieringSemi<IfStmt, ForStmt, DeclStmt, WhileStmt, DoStmt, CXXForRangeStmt, SwitchStmt>(item)) {
            mOutputFormatHelper.AppendSemiNewLine();
        }
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const IfStmt* stmt)
{
    const std::string cexpr{stmt->isConstexpr() ? kwSpaceConstExpr : ""};
    const bool        hasInit{stmt->getInit() || stmt->getConditionVariable()};

    if(hasInit) {
        mOutputFormatHelper.OpenScope();

        if(const auto* conditionVar = stmt->getConditionVariable()) {
            InsertArg(conditionVar);
        }

        if(const auto* init = stmt->getInit()) {
            InsertArg(init);
        }
    }

    mOutputFormatHelper.Append("if", cexpr);

    WrapInParens([&]() { InsertArg(stmt->getCond()); }, AddSpaceAtTheEnd::Yes);

    const auto* body = stmt->getThen();

    InsertArg(body);

    const bool isBodyBraced = isa<CompoundStmt>(body);

    if(!isBodyBraced && !isa<NullStmt>(body)) {
        mOutputFormatHelper.AppendSemiNewLine();
    }

    // else
    if(const auto* elsePart = stmt->getElse()) {
        const std::string cexprElse{stmt->isConstexpr() ? StrCat("/* ", kwConstExprSpace, "*/ ") : ""};

        if(isBodyBraced) {
            mOutputFormatHelper.Append(' ');
        }

        mOutputFormatHelper.Append("else ", cexprElse);

        const bool needScope = isa<IfStmt>(elsePart);
        if(needScope) {
            mOutputFormatHelper.OpenScope();
        }

        InsertArg(elsePart);

        // an else with just a single statement seems not to carry a semi-colon at the end
        if(!needScope && !isa<CompoundStmt>(elsePart)) {
            mOutputFormatHelper.AppendSemiNewLine();
        }

        if(needScope) {
            mOutputFormatHelper.CloseScope();
        }
    }

    mOutputFormatHelper.AppendNewLine();

    if(hasInit) {
        mOutputFormatHelper.CloseScope();
        mOutputFormatHelper.AppendNewLine();
    }

    // one blank line after statement
    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ForStmt* stmt)
{
    // https://github.com/vtjnash/clang-ast-builder/blob/master/AstBuilder.cpp
    // http://clang-developers.42468.n3.nabble.com/Adding-nodes-to-Clang-s-AST-td4054800.html
    // https://stackoverflow.com/questions/30451485/how-to-clone-or-create-an-ast-stmt-node-of-clang/38899615

    if(GetInsightsOptions().UseAltForSyntax) {
        auto* rwStmt = const_cast<ForStmt*>(stmt);

        const auto&        ctx = GetGlobalAST();
        std::vector<Stmt*> bodyStmts{};

        AddBodyStmts(bodyStmts, rwStmt->getBody());
        AddStmt(bodyStmts, rwStmt->getInc());

        auto* condition = [&]() -> Expr* {
            if(rwStmt->getCond()) {
                return rwStmt->getCond();
            }

            return new(ctx) CXXBoolLiteralExpr(true, {}, stmt->getBeginLoc());
        }();

        ArrayRef<Stmt*> bodyStmtsRef{bodyStmts};
        auto*           outerBody = CompoundStmt::Create(ctx, bodyStmtsRef, stmt->getBeginLoc(), stmt->getEndLoc());
        auto*           whileStmt = WhileStmt::Create(ctx, nullptr, condition, outerBody, stmt->getBeginLoc());

        std::vector<Stmt*> outerScopeStmts{};
        AddStmt(outerScopeStmts, rwStmt->getInit());
        AddStmt(outerScopeStmts, whileStmt);

        ArrayRef<Stmt*> outerScopeStmtsRef{outerScopeStmts};
        auto* outerScopeBody = CompoundStmt::Create(ctx, outerScopeStmtsRef, stmt->getBeginLoc(), stmt->getEndLoc());

        InsertArg(outerScopeBody);
        mOutputFormatHelper.AppendNewLine();

    } else {

        {
            // We need to handle the case that a lambda is used in the init-statement of the for-loop.
            LAMBDA_SCOPE_HELPER(VarDecl);

            mOutputFormatHelper.Append("for");

            WrapInParens(
                [&]() {
                    if(const auto* init = stmt->getInit()) {
                        MultiStmtDeclCodeGenerator codeGenerator{mOutputFormatHelper, mLambdaStack};
                        codeGenerator.InsertArg(init);

                    } else {
                        mOutputFormatHelper.Append("; ");
                    }

                    InsertArg(stmt->getCond());
                    mOutputFormatHelper.Append("; ");

                    InsertArg(stmt->getInc());
                },
                AddSpaceAtTheEnd::Yes);
        }

        const auto* body = stmt->getBody();
        const bool  hasCompoundStmt{isa<CompoundStmt>(body)};

        if(hasCompoundStmt) {
            mOutputFormatHelper.AppendNewLine();
        }

        InsertArg(body);

        // Note: an empty for-loop carries a simi-colon at the end
        if(hasCompoundStmt) {
            mOutputFormatHelper.AppendNewLine();
        } else {
            if(!isa<CompoundStmt>(body) && !isa<NullStmt>(body)) {
                mOutputFormatHelper.AppendSemiNewLine();
            }
        }
    }

    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CStyleCastExpr* stmt)
{
    const auto        castKind     = stmt->getCastKind();
    const std::string castName     = GetCastName(castKind);
    const QualType    castDestType = stmt->getType().getCanonicalType();

    FormatCast(castName, castDestType, stmt->getSubExpr(), castKind);
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXNewExpr* stmt)
{
    mOutputFormatHelper.Append("new ");

    if(stmt->getNumPlacementArgs()) {
        /* we have a placement new */

        WrapInParens([&]() {
            ForEachArg(stmt->placement_arguments(), [&](const auto& placementArg) { InsertArg(placementArg); });
        });
    }

    if(const auto* ctorExpr = stmt->getConstructExpr()) {
        InsertArg(ctorExpr);

    } else {
        auto name = GetName(stmt->getAllocatedType());

        // Special handling for arrays. They differ from one to more dimensions.
        if(stmt->isArray()) {
            OutputFormatHelper ofm{};
            CodeGenerator      codeGenerator{ofm};

            ofm.Append("[");
#if IS_CLANG_NEWER_THAN(8)
            codeGenerator.InsertArg(stmt->getArraySize().getValue());
#else
            codeGenerator.InsertArg(stmt->getArraySize());
#endif
            ofm.Append(']');

            // In case of multi dimension the first dimension is the getArraySize() while the others are part of the
            // type included in GetName(...).
            if(Contains(name, "[")) {
                InsertBefore(name, "[", ofm.GetString());
            } else {
                // here we have the single dimension case, the dimension is not part of GetName, so add it.
                name.append(ofm.GetString());
            }
        }

        mOutputFormatHelper.Append(name);

        if(stmt->hasInitializer()) {
            InsertCurlysIfRequired(stmt->getInitializer());
        }
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const MaterializeTemporaryExpr* stmt)
{
    InsertArg(GetTemporary(stmt));
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXOperatorCallExpr* stmt)
{
    LAMBDA_SCOPE_HELPER(OperatorCallExpr);

    const auto* callee = dyn_cast_or_null<DeclRefExpr>(stmt->getCallee()->IgnoreImpCasts());
    const bool  isCXXMethod{callee && isa<CXXMethodDecl>(callee->getDecl())};

    if(2 == stmt->getNumArgs()) {
        auto getArg = [&](unsigned idx) {
            const auto* arg = stmt->getArg(idx);

            // In show all casts mode don't filter this. It shows how the compiler adds const to arguments, if the
            // argument is non-const but the parameter demands a const object
            if(not GetInsightsOptions().ShowAllImplicitCasts) {
                arg = arg->IgnoreImpCasts();
            }

            return dyn_cast_or_null<DeclRefExpr>(arg);
        };

        const auto* param1 = getArg(0);
        const auto* param2 = getArg(1);

        if(callee && param1 && param2) {

            const std::string replace = [&]() {
                if(isa<CXXMethodDecl>(callee->getDecl())) {
                    const std::string tmpl = FormatVarTemplateSpecializationDecl(param1->getDecl(), {});

                    return StrCat(GetName(*param1), tmpl, ".", GetName(*callee), "(", GetName(*param2), ")");
                } else {
                    return StrCat(GetName(*callee), "(", GetName(*param1), ", ", GetName(*param2), ")");
                }
            }();

            mOutputFormatHelper.Append(replace);

            return;
        }
    }

    auto        cb           = stmt->child_begin();
    const auto* fallbackArg0 = stmt->getArg(0);

    // arg0 := operator
    // skip arg0
    ++cb;

    const auto* arg1 = *cb;
    ++cb;

    // operators in a namespace but outside a class so operator goes first
    if(!isCXXMethod) {
        // happens for UnresolvedLooupExpr
        if(not callee) {
            if(const auto* adl = dyn_cast_or_null<UnresolvedLookupExpr>(stmt->getCallee())) {
                InsertArg(adl);
            }
        } else {
            mOutputFormatHelper.Append(GetName(*callee));
        }

        mOutputFormatHelper.Append("(");
    }

    // insert the arguments
    if(isa<DeclRefExpr>(fallbackArg0)) {
        InsertArgWithParensIfNeeded(fallbackArg0);

    } else {
        InsertArgWithParensIfNeeded(arg1);
    }

    // if it is a class operator the operator follows now
    if(isCXXMethod) {
        const OverloadedOperatorKind opKind = stmt->getOperator();

        mOutputFormatHelper.Append(".operator", getOperatorSpelling(opKind), "(");
    }

    // consume all remaining arguments
    const auto childRange = llvm::make_range(cb, stmt->child_end());

    // at least the call-operator can have more than 2 parameters
    ForEachArg(childRange, [&](const auto& child) {
        if(!isCXXMethod) {
            // in global operators we need to separate the two parameters by comma
            mOutputFormatHelper.Append(", ");
        }

        InsertArg(child);
    });

    mOutputFormatHelper.Append(')');
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const LambdaExpr* stmt)
{
    if(!mLambdaStack.empty()) {
        HandleLambdaExpr(stmt, mLambdaStack.back());
        mOutputFormatHelper.Append(GetLambdaName(*stmt));
    } else {
        LAMBDA_SCOPE_HELPER(LambdaExpr);
        HandleLambdaExpr(stmt, mLambdaStack.back());
    }

    if(!mLambdaStack.empty()) {
        mLambdaStack.back().insertInits(mOutputFormatHelper);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXThisExpr* stmt)
{
    DPrint("thisExpr: imlicit=%d %s\n", stmt->isImplicit(), GetName(GetDesugarType(stmt->getType())));

    mOutputFormatHelper.Append("this");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXBindTemporaryExpr* stmt)
{
    InsertArg(stmt->getSubExpr());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXFunctionalCastExpr* stmt)
{
    const bool isConstructor{isa<CXXConstructExpr>(stmt->getSubExpr())};
    const bool isStdListInit{isa<CXXStdInitializerListExpr>(stmt->getSubExpr())};
    const bool isListInitialization{[&]() { return stmt->getLParenLoc().isInvalid(); }()};
    const bool needsParens{!isConstructor && !isListInitialization && !isStdListInit};

    // If a constructor follows we do not need to insert the type name. This would insert it twice.
    if(!isConstructor && !isStdListInit) {
        mOutputFormatHelper.Append(GetName(stmt->getTypeAsWritten()));
    }

    WrapInParensIfNeeded(needsParens, [&] { InsertArg(stmt->getSubExpr()); });
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXBoolLiteralExpr* stmt)
{
    mOutputFormatHelper.Append(stmt->getValue() ? "true" : "false");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const GNUNullExpr* /*stmt*/)
{
    mOutputFormatHelper.Append("NULL");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CharacterLiteral* stmt)
{
    switch(stmt->getKind()) {
        case CharacterLiteral::Ascii: break;
        case CharacterLiteral::Wide: mOutputFormatHelper.Append('L'); break;
        case CharacterLiteral::UTF8: mOutputFormatHelper.Append("u8"); break;
        case CharacterLiteral::UTF16: mOutputFormatHelper.Append('u'); break;
        case CharacterLiteral::UTF32: mOutputFormatHelper.Append('U'); break;
    }

    switch(unsigned value = stmt->getValue()) {
        case '\\': mOutputFormatHelper.Append("'\\\\'"); break;
        case '\0': mOutputFormatHelper.Append("'\\0'"); break;
        case '\'': mOutputFormatHelper.Append("'\\''"); break;
        case '\a': mOutputFormatHelper.Append("'\\a'"); break;
        case '\b': mOutputFormatHelper.Append("'\\b'"); break;
        // FIXME: causes clang to report a non-standard escape sequence error
        // case '\e': mOutputFormatHelper.Append("'\\e'"); break;
        case '\f': mOutputFormatHelper.Append("'\\f'"); break;
        case '\n': mOutputFormatHelper.Append("'\\n'"); break;
        case '\r': mOutputFormatHelper.Append("'\\r'"); break;
        case '\t': mOutputFormatHelper.Append("'\\t'"); break;
        case '\v': mOutputFormatHelper.Append("'\\v'"); break;
        default:
            if((value & ~0xFFu) == ~0xFFu && stmt->getKind() == CharacterLiteral::Ascii) {
                value &= 0xFFu;
            }

            if(value < 256) {
                if(isPrintable(static_cast<unsigned char>(value))) {
                    const std::string v{static_cast<char>(value)};
                    mOutputFormatHelper.Append("'", v, "'");
                } else {
                    const std::string v{std::to_string(static_cast<unsigned char>(value))};
                    mOutputFormatHelper.Append(v);
                }
            }
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const PredefinedExpr* stmt)
{
    // Check if getFunctionName returns a valid StringLiteral. It does return a nullptr, if this PredefinedExpr is
    // in a UnresolvedLookupExpr. In that case, print the identifier, e.g. __func__.
    if(const auto* functionName = stmt->getFunctionName()) {
        InsertArg(functionName);
    } else {
        const auto name = PredefinedExpr::getIdentKindName(stmt->getIdentKind());

        mOutputFormatHelper.Append(name.str());
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ExprWithCleanups* stmt)
{
    InsertArg(stmt->getSubExpr());
}
//-----------------------------------------------------------------------------

std::string CodeGenerator::GetValueOfValueInit(const QualType& t)
{
    const QualType& type = t.getCanonicalType();

    if(type->isScalarType()) {
        switch(type->getScalarTypeKind()) {
            case Type::STK_CPointer:
            case Type::STK_BlockPointer:
            case Type::STK_ObjCObjectPointer:
            case Type::STK_MemberPointer: return "nullptr";

            case Type::STK_Bool: return "false";

            case Type::STK_Integral:
            case Type::STK_Floating:
                if(const auto* bt = type->getAs<BuiltinType>()) {
                    switch(bt->getKind()) {
                            // Type::STK_Integral
                        case BuiltinType::Char_U:
                        case BuiltinType::UChar:
                        case BuiltinType::Char_S:
                        case BuiltinType::SChar: return "'\\0'";
                        case BuiltinType::WChar_U:
                        case BuiltinType::WChar_S: return "L'\\0'";
                        case BuiltinType::Char16: return "u'\\0'";
                        case BuiltinType::Char32: return "U'\\0'";
                        // Type::STK_Floating
                        case BuiltinType::Half:
                        case BuiltinType::Float: return "0.0f";
                        default: break;
                    }
                }

                break;

            case Type::STK_FloatingComplex:
            case Type::STK_IntegralComplex:
                if(const auto* complexType = type->getAs<ComplexType>()) {
                    return GetValueOfValueInit(complexType->getElementType());
                }

                break;

            case Type::STK_FixedPoint: Error("STK_FixedPoint is not implemented"); break;
        }

    } else if(const auto* tt = dyn_cast_or_null<ConstantArrayType>(t.getTypePtrOrNull())) {
        const auto&       elementType{tt->getElementType()};
        const std::string elementTypeInitValue{GetValueOfValueInit(elementType)};

        return FillConstantArray(tt, elementTypeInitValue, uint64_t{0});
    }

    return "0";
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ImplicitValueInitExpr* stmt)
{
    mOutputFormatHelper.Append(GetValueOfValueInit(stmt->getType()));
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXScalarValueInitExpr* stmt)
{
    mOutputFormatHelper.Append(GetName(stmt->getType()), "()");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXTryStmt* stmt)
{
    mOutputFormatHelper.AppendNewLine("try ");

    InsertArg(stmt->getTryBlock());

    for(const auto& i : NumberIterator{stmt->getNumHandlers()}) {
        InsertArg(stmt->getHandler(i));
    }

    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXCatchStmt* stmt)
{
    mOutputFormatHelper.Append(" catch");

    WrapInParens(
        [&]() {
            if(!stmt->getCaughtType().isNull()) {
                mOutputFormatHelper.Append(
                    GetTypeNameAsParameter(stmt->getCaughtType(), stmt->getExceptionDecl()->getNameAsString()));
            } else {
                mOutputFormatHelper.Append("...");
            }
        },
        AddSpaceAtTheEnd::Yes);

    InsertArg(stmt->getHandlerBlock());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXThrowExpr* stmt)
{
    mOutputFormatHelper.Append("throw ");

    InsertArg(stmt->getSubExpr());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ConstantExpr* stmt)
{
    InsertArg(stmt->getSubExpr());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const TypeAliasDecl* stmt)
{
    mOutputFormatHelper.Append("using ", GetName(*stmt), " = ");
    const auto& underlyingType = stmt->getUnderlyingType();

    if(auto* templateSpecializationType = underlyingType->getAs<TemplateSpecializationType>()) {
        if(const auto elaboratedType = underlyingType->getAs<ElaboratedType>()) {
            InsertNamespace(elaboratedType->getQualifier());
        }

        StringStream stream{};
        templateSpecializationType->getTemplateName().dump(stream);

        mOutputFormatHelper.Append(stream.str());

        InsertTemplateArgs(*templateSpecializationType);
    } else if(auto* dependentTemplateSpecializationType =
                  underlyingType->getAs<DependentTemplateSpecializationType>()) {

        mOutputFormatHelper.Append(GetElaboratedTypeKeyword(dependentTemplateSpecializationType->getKeyword()));

        InsertNamespace(dependentTemplateSpecializationType->getQualifier());

        mOutputFormatHelper.Append("template ", dependentTemplateSpecializationType->getIdentifier()->getName());

        InsertTemplateArgs(*dependentTemplateSpecializationType);

    } else {
        mOutputFormatHelper.Append(GetName(underlyingType));
    }

    mOutputFormatHelper.AppendSemiNewLine();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const TypedefDecl* stmt)
{
    /* function pointer typedefs are special. Ease up things using "using" */
    //    outputFormatHelper.AppendNewLine("typedef ", GetName(stmt->getUnderlyingType()), " ", GetName(*stmt),
    //    ";");
    mOutputFormatHelper.AppendNewLine("using ", GetName(*stmt), " = ", GetName(stmt->getUnderlyingType()), ";");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertCXXMethodHeader(const CXXMethodDecl* stmt, OutputFormatHelper& initOutputFormatHelper)
{
    LAMBDA_SCOPE_HELPER(CXXMethodDecl);
    CXXConstructorDecl* cxxInheritedCtorDecl{nullptr};

    // Traverse the ctor inline init statements first to find a potential CXXInheritedCtorInitExpr. This carries the
    // name and the type. The CXXMethodDecl above knows only the type.
    if(const auto* ctor = dyn_cast_or_null<CXXConstructorDecl>(stmt)) {
        CodeGenerator codeGenerator{initOutputFormatHelper, mLambdaStack};
        codeGenerator.mCurrentPos                = mCurrentPos;
        codeGenerator.mCurrentFieldPos           = mCurrentFieldPos;
        codeGenerator.mOutputFormatHelperOutside = &mOutputFormatHelper;

        OnceTrue first{};

        for(const auto* init : ctor->inits()) {
            initOutputFormatHelper.AppendNewLine();
            if(first) {
                initOutputFormatHelper.Append(": ");
            } else {
                initOutputFormatHelper.Append(", ");
            }

            // in case of delegating or base initializer there is no member.
            if(const auto* member = init->getMember()) {
                initOutputFormatHelper.Append(member->getNameAsString());
                codeGenerator.InsertCurlysIfRequired(init->getInit());
            } else {
                const auto* inlineInit = init->getInit();
                bool        useCurlies{false};

                if(const auto* cxxInheritedCtorInitExpr = dyn_cast_or_null<CXXInheritedCtorInitExpr>(inlineInit)) {
                    cxxInheritedCtorDecl = cxxInheritedCtorInitExpr->getConstructor();

                    // Insert the base class name only, if it is not a CXXContructorExpr and not a
                    // CXXDependentScopeMemberExpr which already carry the type.
                } else if(init->isBaseInitializer() && not isa<CXXConstructExpr>(inlineInit)) {
                    initOutputFormatHelper.Append(GetUnqualifiedScopelessName(init->getBaseClass()));
                    useCurlies = true;
                }

                codeGenerator.WrapInCurliesIfNeeded(useCurlies, [&] { codeGenerator.InsertArg(inlineInit); });
            }
        }
    }

    InsertTemplateGuardBegin(stmt);
    InsertAccessModifierAndNameWithReturnType(*stmt, SkipAccess::Yes, cxxInheritedCtorDecl);

    if(stmt->isDeleted()) {
        mOutputFormatHelper.AppendNewLine(" = delete;");

    } else if(stmt->isDefaulted()) {
        mOutputFormatHelper.AppendNewLine(" = default;");
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertCXXMethodDecl(const CXXMethodDecl* stmt, SkipBody skipBody)
{
    OutputFormatHelper initOutputFormatHelper{};
    initOutputFormatHelper.SetIndent(mOutputFormatHelper, OutputFormatHelper::SkipIndenting::Yes);

    InsertCXXMethodHeader(stmt, initOutputFormatHelper);

    if(not stmt->isUserProvided()) {
        InsertTemplateGuardEnd(stmt);
        return;
    }

    mOutputFormatHelper.Append(initOutputFormatHelper.GetString());

    if(isa<CXXConversionDecl>(stmt)) {
        if(stmt->getParent()->isLambda() && not stmt->doesThisDeclarationHaveABody()) {
            mOutputFormatHelper.AppendNewLine();
            WrapInCurlys([&]() {
                mOutputFormatHelper.AppendNewLine();

                mOutputFormatHelper.AppendNewLine(
                    "  return ", stmt->getParent()->getLambdaStaticInvoker()->getNameAsString(), ";");
            });
        }
    }

    if((SkipBody::No == skipBody) && stmt->doesThisDeclarationHaveABody() && not stmt->isLambdaStaticInvoker()) {
        mOutputFormatHelper.AppendNewLine();
        InsertArg(stmt->getBody());
        mOutputFormatHelper.AppendNewLine();

    } else if(not InsertLambdaStaticInvoker(stmt) || (SkipBody::Yes == skipBody)) {
        mOutputFormatHelper.AppendSemiNewLine();
    }

    InsertTemplateGuardEnd(stmt);

    if(SkipBody::No == skipBody) {
        mOutputFormatHelper.AppendNewLine();
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXMethodDecl* stmt)
{
    // As per [special]/1: "Programs shall not define implicitly-declared special member functions." hide special
    // members which are not used and with that not fully evaluated. This also hopefully removes confusion about the
    // noexcept, which is not evaluated, if the special member is not used.
    if(not stmt->hasBody() && not stmt->isUserProvided() && not stmt->isExplicitlyDefaulted() &&
       not stmt->isDeleted()) {
        return;
    }

    InsertCXXMethodDecl(stmt, SkipBody::No);
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const EnumDecl* stmt)
{
    mOutputFormatHelper.Append("enum ");

    if(stmt->isScoped()) {
        if(stmt->isScopedUsingClassTag()) {
            mOutputFormatHelper.Append(kwClassSpace);
        } else {
            mOutputFormatHelper.Append("struct ");
        }
    }

    mOutputFormatHelper.Append(stmt->getNameAsString());

    if(stmt->isFixed()) {
        mOutputFormatHelper.Append(" : ", GetName(stmt->getIntegerType()));
    }

    mOutputFormatHelper.AppendNewLine();

    WrapInCurlys(
        [&]() {
            mOutputFormatHelper.IncreaseIndent();
            mOutputFormatHelper.AppendNewLine();

            ForEachArg(stmt->enumerators(), [&](const auto* value) { InsertArg(value); });

            InsertArg(stmt->getBody());

            mOutputFormatHelper.DecreaseIndent();
            mOutputFormatHelper.AppendNewLine();
        },
        AddSpaceAtTheEnd::No);

    mOutputFormatHelper.AppendSemiNewLine();
    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const EnumConstantDecl* stmt)
{
    mOutputFormatHelper.Append(stmt->getNameAsString());

    if(const auto* initExpr = stmt->getInitExpr()) {

        mOutputFormatHelper.Append(" = ");

        InsertArg(initExpr);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const FieldDecl* stmt)
{
    InsertAttributes(stmt->attrs());

    if(stmt->isMutable()) {
        mOutputFormatHelper.Append("mutable ");
    }

    if(const auto* cxxRecordDecl = dyn_cast_or_null<CXXRecordDecl>(stmt->getParent())) {
        std::string name{GetName(*stmt)};

        if(const auto fieldName = GetFieldDeclNameForLambda(*stmt, *cxxRecordDecl)) {
            name = std::move(fieldName.getValue());
        }

        mOutputFormatHelper.Append(GetTypeNameAsParameter(stmt->getType(), name));

        if(const auto* constantExpr = dyn_cast_or_null<ConstantExpr>(stmt->getBitWidth())) {
            mOutputFormatHelper.Append(':');
            InsertArg(constantExpr);
        }

        // Keep the inline init for aggregates, as we do not see it somewhere else.
        if(cxxRecordDecl->isAggregate()) {
            const auto* initializer = stmt->getInClassInitializer();
            if(stmt->hasInClassInitializer() && initializer) {
                const bool isConstructorExpr{isa<CXXConstructExpr>(initializer) || isa<ExprWithCleanups>(initializer)};
                if(ICIS_ListInit != stmt->getInClassInitStyle() || isConstructorExpr) {
                    mOutputFormatHelper.Append(" = ");
                }

                InsertArg(initializer);
            }
        }
    }

    mOutputFormatHelper.AppendNewLine(";");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const AccessSpecDecl* stmt)
{
    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.AppendNewLine(AccessToStringWithColon(stmt->getAccess()));
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const StaticAssertDecl* stmt)
{
    if(!stmt->isFailed()) {
        mOutputFormatHelper.Append("/* PASSED: ");
    } else {
        mOutputFormatHelper.Append("/* FAILED: ");
    }

    mOutputFormatHelper.Append("static_assert(");

    InsertArg(stmt->getAssertExpr());

    if(stmt->getMessage()) {
        mOutputFormatHelper.Append(", ");
        InsertArg(stmt->getMessage());
    }

    mOutputFormatHelper.AppendNewLine("); */");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const UsingDirectiveDecl* stmt)
{
    const auto& name = GetName(*stmt->getNominatedNamespace());

    // We need this due to a wired case in UsingDeclTest.cpp
    if(not name.empty()) {
        mOutputFormatHelper.AppendNewLine("using namespace ", name, ";");
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const NamespaceDecl* stmt)
{
    SCOPE_HELPER(stmt);

    if(stmt->isInline()) {
        mOutputFormatHelper.Append(kwInlineSpace);
    }

    mOutputFormatHelper.Append("namespace");

    if(not stmt->isAnonymousNamespace()) {
        mOutputFormatHelper.Append(" ", stmt->getNameAsString());
    }

    mOutputFormatHelper.AppendNewLine();

    mOutputFormatHelper.OpenScope();

    for(const auto* decl : stmt->decls()) {
        InsertArg(decl);
    }

    mOutputFormatHelper.CloseScope();
}
//-----------------------------------------------------------------------------

void CodeGenerator::ParseDeclContext(const DeclContext* ctx)
{
    mOutputFormatHelper.Append(GetDeclContext(ctx));
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const UsingDecl* stmt)
{
    OutputFormatHelper ofm{};
    ofm.SetIndent(mOutputFormatHelper, OutputFormatHelper::SkipIndenting::Yes);

    // Skip UsingDecl's which have ConstructorUsingShadowDecl attached. This means that we will create the
    // associated constructors from the base class later. Having this \c using still in the code prevents compiling
    // the transformed code.
    if(stmt->shadow_size()) {
        for(const auto* shadow : stmt->shadows()) {
            if(isa<ConstructorUsingShadowDecl>(shadow)) {
                return;
            } else if(const auto* shadowUsing = dyn_cast_or_null<UsingShadowDecl>(shadow)) {
                UsingCodeGenerator codeGenerator{ofm};
                codeGenerator.InsertArg(shadowUsing->getTargetDecl());
            }
        }
    }

    mOutputFormatHelper.Append("using ");

    InsertQualifierAndName(stmt->getDeclName(), stmt->getQualifier(), mOutputFormatHelper);

    mOutputFormatHelper.AppendNewLine(";");

    // Insert what a using declaration pulled into this scope.
    if(not ofm.GetString().empty()) {
        mOutputFormatHelper.AppendNewLine(ofm.GetString());
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const NamespaceAliasDecl* stmt)
{
    mOutputFormatHelper.AppendNewLine(
        "namespace ", stmt->getDeclName().getAsString(), " = ", GetName(*stmt->getAliasedNamespace()), ";");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const FriendDecl* stmt)
{
    if(const auto* typeInfo = stmt->getFriendType()) {
        mOutputFormatHelper.Append("friend ");
        mOutputFormatHelper.Append(GetName(typeInfo->getType()));
        mOutputFormatHelper.AppendSemiNewLine();

    } else {
        if(const auto* fd = dyn_cast_or_null<FunctionDecl>(stmt->getFriendDecl())) {
            InsertArg(fd);
        } else if(const auto* fdt = dyn_cast_or_null<FunctionTemplateDecl>(stmt->getFriendDecl())) {
            InsertArg(fdt);
        } else {
            std::string cls{};
            if(const auto* ctd = dyn_cast_or_null<ClassTemplateDecl>(stmt->getFriendDecl())) {
                InsertTemplateParameters(*ctd->getTemplateParameters());

                cls = GetClassOrStructTagName(*ctd->getTemplatedDecl());
            }

            mOutputFormatHelper.AppendNewLine("friend ", cls, GetName(*stmt->getFriendDecl()), ";");
        }
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXNoexceptExpr* stmt)
{
    mOutputFormatHelper.Append("noexcept(");

    if(stmt->getValue()) {
        mOutputFormatHelper.Append("true");

    } else {
        mOutputFormatHelper.Append("false");
    }

    mOutputFormatHelper.Append(") ");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXDeductionGuideDecl* stmt)
{
    if(stmt->isCopyDeductionCandidate()) {
        return;
    }

    const bool isSpecialization{stmt->isFunctionTemplateSpecialization()};
    const bool isImplicit{stmt->isImplicit()};

    if(isImplicit || isSpecialization) {
        InsertTemplateGuardBegin(stmt);
    }

    const auto* deducedTemplate = stmt->getDeducedTemplate();

    if(isSpecialization) {
        InsertTemplateSpecializationHeader();
    } else {
        InsertTemplateParameters(*deducedTemplate->getTemplateParameters());
    }

    mOutputFormatHelper.Append(GetName(*deducedTemplate));

    if(stmt->getNumParams()) {
        WrapInParens([&] { mOutputFormatHelper.AppendParameterList(stmt->parameters()); });
    }

    mOutputFormatHelper.Append(" -> ", GetName(stmt->getReturnType()));
    mOutputFormatHelper.AppendSemiNewLine();

    if(isImplicit || isSpecialization) {
        InsertTemplateGuardEnd(stmt);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const FunctionTemplateDecl* stmt)
{
    // InsertTemplateParameters(*stmt->getTemplateParameters());
    InsertArg(stmt->getTemplatedDecl());

    for(const auto* spec : stmt->specializations()) {
        mOutputFormatHelper.AppendNewLine();
        InsertArg(spec);
        mOutputFormatHelper.AppendNewLine();
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const TypeAliasTemplateDecl* stmt)
{
    InsertTemplateParameters(*stmt->getTemplateParameters());

    InsertArg(stmt->getTemplatedDecl());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const AttributedStmt* stmt)
{
    for(const auto& attr : stmt->getAttrs()) {
        InsertAttribute(*attr);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertAttributes(const Decl::attr_range& attrs)
{
    // attrs required for constinit
    for(const auto& attr : attrs) {
        InsertAttribute(*attr);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertAttribute(const Attr& attr)
{
    // skip this attribute. Clang seems to tag virtual methods with override
    if(attr::Override == attr.getKind()) {
        return;
    }

    StringStream   stream{};
    PrintingPolicy pp{LangOptions{}};
    pp.Alignof = true;

    attr.printPretty(stream, pp);

    // attributes start with a space, skip it as it is not required for the first attribute
    const char* start = stream.str().c_str() + 1;

    mOutputFormatHelper.Append(start, " ");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXRecordDecl* stmt)
{
    SCOPE_HELPER(stmt);

    // Prevent a case like in #205 where the lambda appears twice.
    if(stmt->isLambda() && (mLambdaStack.empty() || (nullptr == mLambdaExpr))) {
        return;
    }

    const auto* classTemplatePartialSpecializationDecl = dyn_cast_or_null<ClassTemplatePartialSpecializationDecl>(stmt);
    const auto* classTemplateSpecializationDecl        = dyn_cast_or_null<ClassTemplateSpecializationDecl>(stmt);

    // we require the if-guard only if it is a compiler generated specialization. If it is a hand-written variant it
    // should compile.
    const bool isClassTemplateSpecialization{classTemplatePartialSpecializationDecl || classTemplateSpecializationDecl};
    const bool tmplRequiresIfDef{[&] {
        if(classTemplatePartialSpecializationDecl) {
            return classTemplatePartialSpecializationDecl->isImplicit();

        } else if(classTemplateSpecializationDecl) {
            return not classTemplateSpecializationDecl->isExplicitInstantiationOrSpecialization();
        }

        return false;
    }()};

    if(isClassTemplateSpecialization) {
        if(tmplRequiresIfDef) {
            InsertInstantiationPoint(mOutputFormatHelper,
                                     GetSM(*classTemplateSpecializationDecl),
                                     classTemplateSpecializationDecl->getPointOfInstantiation());
            mOutputFormatHelper.AppendNewLine("#ifdef INSIGHTS_USE_TEMPLATE");
        }

        if(classTemplatePartialSpecializationDecl) {
            InsertTemplateParameters(*classTemplatePartialSpecializationDecl->getTemplateParameters());
        } else {
            InsertTemplateSpecializationHeader();
        }
    }

    mOutputFormatHelper.Append(GetClassOrStructTagName(*stmt));

    InsertAttributes(stmt->attrs());

    mOutputFormatHelper.Append(GetName(*stmt));

    // skip classes/struct's without a definition
    if(not stmt->hasDefinition() || not stmt->isCompleteDefinition()) {
        mOutputFormatHelper.AppendSemiNewLine();
        return;
    }

    if(classTemplateSpecializationDecl) {
        InsertTemplateArgs(*classTemplateSpecializationDecl);
    }

    if(stmt->getNumBases()) {
        mOutputFormatHelper.Append(" : ");

        ForEachArg(stmt->bases(), [&](const auto& base) {
            const std::string virtualKw{base.isVirtual() ? kwVirtualSpace : ""};

            mOutputFormatHelper.Append(
                AccessToString(base.getAccessSpecifier()), " ", virtualKw, GetName(base.getType()));
        });
    }

    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.OpenScope();

    mCurrentFieldPos = mOutputFormatHelper.CurrentPos();

    OnceTrue        firstRecordDecl{};
    OnceTrue        firstDecl{};
    Decl::Kind      formerKind{};
    AccessSpecifier lastAccess{stmt->isClass() ? AS_private : AS_public};
    for(const auto* d : stmt->decls()) {
        if(isa<CXXRecordDecl>(d) && firstRecordDecl) {
            continue;
        }

        // Insert a newline when the decl kind changes. This for example, inserts a newline when after a FieldDecl
        // we see a CXXMethodDecl.
        if(not firstDecl && (d->getKind() != formerKind)) {
            // mOutputFormatHelper.AppendNewLine();
        }

        if((stmt->isLambda() && isa<CXXDestructorDecl>(d))) {
            continue;
        }

        // Insert the access modifier, as at least some of the compiler generated classes do not contain an access
        // specifier which results in a default ctor being private if we do not insert the access modifier.
        if(lastAccess != d->getAccess()) {
            lastAccess = d->getAccess();

            // skip inserting an access specifier of our own, if there is a real one coming.
            if(not isa<AccessSpecDecl>(d)) {
                mOutputFormatHelper.AppendNewLine(AccessToStringWithColon(lastAccess));
            }
        }

        InsertArg(d);
        formerKind = d->getKind();
    }

    if(stmt->isLambda()) {
        const LambdaCallerType lambdaCallerType = mLambdaStack.back().callerType();
        const bool             ctorRequired{(stmt->captures_begin() != stmt->captures_end()) ||
                                stmt->lambdaIsDefaultConstructibleAndAssignable()};

        if(ctorRequired) {
            if(AS_public != lastAccess) {
                mOutputFormatHelper.AppendNewLine();
                mOutputFormatHelper.AppendNewLine("public:");
            }

            if(stmt->lambdaIsDefaultConstructibleAndAssignable()) {
                mOutputFormatHelper.Append("// ");

                if(stmt->hasConstexprDefaultConstructor()) {
                    mOutputFormatHelper.Append("/*constexpr */ ");
                }
            }

            mOutputFormatHelper.Append(GetName(*stmt), "(");
        }

        SmallVector<std::string, 5> ctorInitializerList{};
        std::string                 ctorArguments{"{"};
        OnceTrue                    firstCtorArgument{};

        auto addToInits =
            [&](std::string name, const FieldDecl* fd, bool isThis, const Expr* expr, bool /*useBraces*/) {
                if(firstCtorArgument) {
                } else {
                    mOutputFormatHelper.Append(", ");
                    ctorArguments.append(", ");
                }

                std::string refRef{};

                const auto& fieldName{StrCat(isThis ? "__" : "", name)};

                // Special handling for lambdas with init captures which contain a move. In such a case, copy the
                // initial move statement and make the variable a &&.
                if(const auto* cxxConstructExpr = dyn_cast_or_null<CXXConstructExpr>(expr);
                   cxxConstructExpr && cxxConstructExpr->getConstructor()->isMoveConstructor()) {

                    OutputFormatHelper             ofm{};
                    LambdaInitCaptureCodeGenerator codeGenerator{ofm, mLambdaStack, name};

                    if(cxxConstructExpr->getNumArgs()) {
                        ForEachArg(cxxConstructExpr->arguments(),
                                   [&](const auto& arg) { codeGenerator.InsertArg(arg); });
                    }

                    refRef = "&& ";

                    ctorInitializerList.push_back(StrCat(fieldName, "{", ofm.GetString(), "}"));
                } else {

                    ctorInitializerList.push_back(StrCat(fieldName, "{", "_", name, "}"));
                }

                if(not isThis && expr) {
                    OutputFormatHelper ofm{};
                    CodeGenerator      codeGenerator{ofm, mLambdaStack};
                    codeGenerator.InsertArg(expr);
                    ctorArguments.append(ofm.GetString());
                } else {
                    if(isThis && not fd->getType()->isPointerType()) {
                        ctorArguments.append("*");
                    }

                    ctorArguments.append(name);
                }

                mOutputFormatHelper.Append(GetTypeNameAsParameter(fd->getType(), StrCat(refRef, "_", name)));
            };

        llvm::DenseMap<const VarDecl*, FieldDecl*> captures{};
        FieldDecl*                                 thisCapture{};

        stmt->getCaptureFields(captures, thisCapture);

        // Check if it captures this
        if(thisCapture) {
            const auto* captureInit = mLambdaExpr->capture_init_begin();

            addToInits("this", thisCapture, true, *captureInit, false);
        } else {
            // Find the corresponding capture in the DenseMap. The DenseMap seems to be change its order each time.
            // Hence we use \c captures() to keep the order stable. While using \c Captures to generate the code as
            // it carries the better type infos.
            for(const auto& [c, cinit] : zip(mLambdaExpr->captures(), mLambdaExpr->capture_inits())) {
                if(not c.capturesVariable()) {
                    continue;
                }

                const auto* capturedVar = c.getCapturedVar();
                if(const auto* value = captures[capturedVar]) {
                    addToInits(
                        GetName(*capturedVar), value, false, cinit, VarDecl::ListInit == capturedVar->getInitStyle());
                }
            }
        }

        ctorArguments.append("}");

        // generate the ctor only if it is required, i.e. we have captures. This is in fact a trick to get
        // compiling code out of it. The compiler itself does not generate a constructor in many many cases.
        if(ctorRequired) {
            mOutputFormatHelper.Append(")");

            if(stmt->lambdaIsDefaultConstructibleAndAssignable()) {
                mOutputFormatHelper.AppendNewLine(" = default;");

            } else {
                mOutputFormatHelper.AppendNewLine();

                OnceTrue firstCtorInitializer{};
                for(const auto& initializer : ctorInitializerList) {
                    if(firstCtorInitializer) {
                        mOutputFormatHelper.Append(": ");
                    } else {
                        mOutputFormatHelper.Append(", ");
                    }

                    mOutputFormatHelper.AppendNewLine(initializer);
                }

                mOutputFormatHelper.AppendNewLine("{}");
            }
        }

        // close the class scope
        mOutputFormatHelper.CloseScope();

        if((LambdaCallerType::VarDecl != lambdaCallerType) && (LambdaCallerType::CallExpr != lambdaCallerType)) {
            mOutputFormatHelper.Append(" ", GetLambdaName(*stmt), ctorArguments);
        } else {
            mLambdaStack.back().inits().append(ctorArguments);
        }

    } else {
        mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::Yes);
    }

    mOutputFormatHelper.AppendSemiNewLine();
    mOutputFormatHelper.AppendNewLine();

    if(tmplRequiresIfDef) {
        mOutputFormatHelper.AppendNewLine("#endif");
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const DeclStmt* stmt)
{
    for(const auto* decl : stmt->decls()) {
        InsertArg(decl);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const SubstNonTypeTemplateParmExpr* stmt)
{
    InsertArg(stmt->getReplacement());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const SizeOfPackExpr* stmt)
{
    if(stmt->isPartiallySubstituted()) {
        mOutputFormatHelper.Append(stmt->getPartialArguments().size());
    } else if(!stmt->isValueDependent()) {
        mOutputFormatHelper.Append(stmt->getPackLength());
    } else {
        mOutputFormatHelper.Append("sizeof...(", GetName(*stmt->getPack()), ")");
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ReturnStmt* stmt)
{
    LAMBDA_SCOPE_HELPER(ReturnStmt);
    mCurrentReturnPos = mOutputFormatHelper.CurrentPos();

    mOutputFormatHelper.Append("return");

    if(const auto* retVal = stmt->getRetValue()) {
        mOutputFormatHelper.Append(' ');
        InsertArg(retVal);
    }

    mCurrentReturnPos.reset();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const NullStmt* /*stmt*/)
{
    mOutputFormatHelper.AppendSemiNewLine();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXDefaultArgExpr* stmt)
{
    InsertArg(stmt->getExpr());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXStdInitializerListExpr* stmt)
{
    if(GetInsightsOptions().UseShowInitializerList) {
        if(not mCurrentPos.hasValue() && not mCurrentFieldPos.hasValue() && not mCurrentReturnPos.hasValue()) {
            return;
        }

        std::string modifiers{};
        size_t      variableInsertPos = mCurrentReturnPos.getValueOr(mCurrentPos.getValueOr(0));

        auto& ofmToInsert = [&]() -> decltype(auto) {
            if(not mCurrentPos.hasValue() && not mCurrentReturnPos.hasValue()) {
                variableInsertPos = mCurrentFieldPos.getValueOr(0);
                mCurrentPos       = variableInsertPos;
                modifiers         = kwStaticSpace;
                modifiers += kwInlineSpace;
                return (*mOutputFormatHelperOutside);
            }

            return (mOutputFormatHelper);
        }();

        OutputFormatHelper ofm{};
        ofm.SetIndent(ofmToInsert, OutputFormatHelper::SkipIndenting::Yes);

        const auto* mat  = dyn_cast<MaterializeTemporaryExpr>(stmt->getSubExpr());
        const auto  size = [&]() -> size_t {
            if(const auto* list = dyn_cast_or_null<InitListExpr>(GetTemporary(mat))) {
                return list->getNumInits();
            }

            return 0;
        }();

        auto internalListName = BuildInternalVarName("list");
        internalListName.append(std::to_string(variableInsertPos));

        ofm.Append(modifiers, GetTypeNameAsParameter(mat->getType(), internalListName));
        CodeGenerator codeGenerator{ofm};
        codeGenerator.InsertArg(stmt->getSubExpr());
        ofm.AppendSemiNewLine();

        ofmToInsert.InsertAt(variableInsertPos, ofm.GetString());

        // No qualifiers like const or volatile here. This appears in  function calls or operators as a parameter. CV's
        // are not allowed there.
        mOutputFormatHelper.Append(GetName(stmt->getType(), Unqualified::Yes), "{", internalListName, ", ", size, "}");

        if(mCurrentReturnPos.hasValue()) {
            mCurrentReturnPos = mCurrentReturnPos.getValue() + ofm.GetString().size();
        } else {
            mCurrentPos = mCurrentPos.getValue() + ofm.GetString().size();
        }

    } else {
        // No qualifiers like const or volatile here. This appears in  function calls or operators as a parameter. CV's
        // are not allowed there.
        mOutputFormatHelper.Append(GetName(stmt->getType(), Unqualified::Yes));
        InsertArg(stmt->getSubExpr());
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXNullPtrLiteralExpr* /*stmt*/)
{
    mOutputFormatHelper.Append("nullptr");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const LabelDecl* stmt)
{
    mOutputFormatHelper.Append(stmt->getNameAsString());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const Decl* stmt)
{
#define SUPPORTED_DECL(type)                                                                                           \
    if(isa<type>(stmt)) {                                                                                              \
        InsertArg(static_cast<const type*>(stmt));                                                                     \
        return;                                                                                                        \
    }

#define IGNORED_DECL SUPPORTED_DECL

#include "CodeGeneratorTypes.h"

    TODO(stmt, mOutputFormatHelper);
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const Stmt* stmt)
{
    if(!stmt) {
        DPrint("Null stmt\n");
        return;
    }

#define SUPPORTED_STMT(type)                                                                                           \
    if(isa<type>(stmt)) {                                                                                              \
        InsertArg(dyn_cast_or_null<type>(stmt));                                                                       \
        return;                                                                                                        \
    }

#define IGNORED_STMT SUPPORTED_STMT

#include "CodeGeneratorTypes.h"

    TODO(stmt, mOutputFormatHelper);
}
//-----------------------------------------------------------------------------

void CodeGenerator::FormatCast(const std::string castName,
                               const QualType&   castDestType,
                               const Expr*       subExpr,
                               const CastKind&   castKind)
{
    const bool        isCastToBase{((castKind == CK_DerivedToBase) || (castKind == CK_UncheckedDerivedToBase)) &&
                            castDestType->isRecordType()};
    const std::string castDestTypeText{
        StrCat(GetName(castDestType), ((isCastToBase && !castDestType->isAnyPointerType()) ? "&" : ""))};

    mOutputFormatHelper.Append(castName, "<", castDestTypeText, ">(");
    InsertArg(subExpr);
    mOutputFormatHelper.Append(')');
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArgWithParensIfNeeded(const Stmt* stmt)
{
    const bool needsParens = [&]() {
        if(const auto* expr = dyn_cast_or_null<Expr>(stmt))
            if(const auto* dest = dyn_cast_or_null<UnaryOperator>(expr->IgnoreImplicit())) {
                if(dest->getOpcode() == clang::UO_Deref) {
                    return true;
                }
            }

        return false;
    }();

    WrapInParensIfNeeded(needsParens, [&] { InsertArg(stmt); });
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertSuffix(const QualType& type)
{
    if(const auto* typePtr = type.getTypePtrOrNull()) {
        if(typePtr->isBuiltinType()) {
            if(const auto* bt = dyn_cast_or_null<BuiltinType>(typePtr)) {
                const auto kind = bt->getKind();

                mOutputFormatHelper.Append(GetBuiltinTypeSuffix(kind));
            }
        }
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertTemplateArgs(const ClassTemplateSpecializationDecl& clsTemplateSpe)
{
    if(const TypeSourceInfo* typeAsWritten = clsTemplateSpe.getTypeAsWritten()) {
        const TemplateSpecializationType* tmplSpecType = cast<TemplateSpecializationType>(typeAsWritten->getType());
        InsertTemplateArgs(*tmplSpecType);
    } else {
        InsertTemplateArgs(clsTemplateSpe.getTemplateArgs());
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::HandleTemplateParameterPack(const ArrayRef<TemplateArgument>& args)
{
    ForEachArg(args, [&](const auto& arg) { InsertTemplateArg(arg); });
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertTemplateArg(const TemplateArgument& arg)
{
    switch(arg.getKind()) {
        case TemplateArgument::Type: mOutputFormatHelper.Append(GetName(arg.getAsType())); break;
        case TemplateArgument::Declaration:
            // TODO: handle pointers
            mOutputFormatHelper.Append("&", arg.getAsDecl()->getQualifiedNameAsString());
            break;
        case TemplateArgument::NullPtr: mOutputFormatHelper.Append(GetName(arg.getNullPtrType())); break;
        case TemplateArgument::Integral:

            if(const auto& integral = arg.getAsIntegral(); arg.getIntegralType()->isCharType()) {
                const char c{static_cast<char>(integral.getZExtValue())};
                mOutputFormatHelper.Append("'", std::string{c}, "'");
            } else {
                mOutputFormatHelper.Append(integral);
            }

            break;
        case TemplateArgument::Expression: InsertArg(arg.getAsExpr()); break;
        case TemplateArgument::Pack: HandleTemplateParameterPack(arg.pack_elements()); break;
        case TemplateArgument::Template:
            mOutputFormatHelper.Append(GetName(*arg.getAsTemplate().getAsTemplateDecl()));
            break;
        case TemplateArgument::TemplateExpansion:
            mOutputFormatHelper.Append(GetName(*arg.getAsTemplateOrTemplatePattern().getAsTemplateDecl()));
            break;
        case TemplateArgument::Null: mOutputFormatHelper.Append("null"); break;
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::HandleLocalStaticNonTrivialClass(const VarDecl* stmt)
{
    mHaveLocalStatic = true;

    auto&      langOpts{GetLangOpts(*stmt)};
    const bool threadSafe{langOpts.ThreadsafeStatics && langOpts.CPlusPlus11 &&
                          (stmt->isLocalVarDecl() /*|| NonTemplateInline*/) && !stmt->getTLSKind()};

    const std::string internalVarName{BuildInternalVarName(GetName(*stmt))};
    const std::string compilerBoolVarName{StrCat(internalVarName, "Guard")};
    const std::string typeName{GetName(stmt->getType())};

    // insert compiler bool to track init state
    const std::string stateTrackingVarName{threadSafe ? "uint64_t" : "bool"};

    mOutputFormatHelper.AppendNewLine("static ", stateTrackingVarName, " ", compilerBoolVarName, ";");

    // insert compiler memory place holder
    mOutputFormatHelper.AppendNewLine(
        "alignas(", typeName, ") static char ", internalVarName, "[sizeof(", typeName, ")];");

    // insert compiler init if
    mOutputFormatHelper.AppendNewLine();

    mOutputFormatHelper.AppendNewLine("if( ! ", compilerBoolVarName, " )");
    mOutputFormatHelper.OpenScope();

    if(threadSafe) {
        mOutputFormatHelper.AppendNewLine("if( __cxa_guard_acquire(&", compilerBoolVarName, ") )");
        mOutputFormatHelper.OpenScope();
    }

    // try to find out whether this ctor can throw. If, then additional code needs to be generated for exception
    // handling.
    const bool canThrow{[&] {
        if(const auto* ctorExpr = dyn_cast_or_null<CXXConstructExpr>(stmt->getInit())) {
            const auto* ctor = ctorExpr->getConstructor();
            if(const auto* func = ctor->getType()->castAs<FunctionProtoType>()) {
                return CT_Cannot != func->canThrow();
            }
        }

        return false;
    }()};

    if(canThrow) {
        mOutputFormatHelper.AppendNewLine("try");
        mOutputFormatHelper.OpenScope();
    }

    mOutputFormatHelper.Append("new (&", internalVarName, ") ");
    // VarDecl of a static expression always have an initializer
    InsertArg(stmt->getInit());

    mOutputFormatHelper.AppendNewLine(';');
    mOutputFormatHelper.AppendNewLine(compilerBoolVarName, " = true;");

    if(canThrow) {
        mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::Yes);
        mOutputFormatHelper.AppendNewLine();
        mOutputFormatHelper.AppendNewLine("catch(...)");
        mOutputFormatHelper.OpenScope();

        mOutputFormatHelper.AppendNewLine("__cxa_guard_abort(&", compilerBoolVarName, ");");
        mOutputFormatHelper.AppendNewLine("throw;");
        mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::Yes);
        mOutputFormatHelper.AppendNewLine();
        mOutputFormatHelper.AppendNewLine();
    }

    if(threadSafe) {
        mOutputFormatHelper.AppendNewLine("__cxa_guard_release(&", compilerBoolVarName, ");");
        mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::Yes);
        mOutputFormatHelper.AppendNewLine();
    }

    mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::Yes);
    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

const char* CodeGenerator::GetKind(const UnaryExprOrTypeTraitExpr& uk)
{
    switch(uk.getKind()) {
        case UETT_SizeOf: return "sizeof";
        case UETT_AlignOf: return "alignof";
        default: return "unknown";
    };
}
//-----------------------------------------------------------------------------

const char* CodeGenerator::GetBuiltinTypeSuffix(const BuiltinType::Kind& kind)
{
#define CASE(K, retVal)                                                                                                \
    case BuiltinType::K: return retVal
    switch(kind) {
        CASE(UInt, "U");
        CASE(ULong, "UL");
        CASE(ULongLong, "ULL");
        CASE(UInt128, "ULLL");
        CASE(Long, "L");
        CASE(LongLong, "LL");
        CASE(Float, "F");
        CASE(LongDouble, "L");
        default: return "";
    }
#undef BTCASE
}
//-----------------------------------------------------------------------------

void CodeGenerator::HandleLambdaExpr(const LambdaExpr* lambda, LambdaHelper& lambdaHelper)
{
    OutputFormatHelper& outputFormatHelper = lambdaHelper.buffer();

    outputFormatHelper.AppendNewLine();
    LambdaCodeGenerator codeGenerator{outputFormatHelper, mLambdaStack};
    codeGenerator.mCapturedThisAsCopy = [&] {
        for(const auto& c : lambda->captures()) {
            const auto captureKind = c.getCaptureKind();

            if(c.capturesThis() && (captureKind == LCK_StarThis)) {
                return true;
            }
        }

        return false;
    }();

    codeGenerator.mLambdaExpr = lambda;
    codeGenerator.InsertArg(lambda->getLambdaClass());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertAccessModifierAndNameWithReturnType(const FunctionDecl&       decl,
                                                              const SkipAccess          skipAccess,
                                                              const CXXConstructorDecl* cxxInheritedCtorDecl)
{
    bool        isLambda{false};
    bool        isFirstCxxMethodDecl{true};
    const auto* methodDecl{dyn_cast_or_null<CXXMethodDecl>(&decl)};
    bool        isCXXMethodDecl{nullptr != methodDecl};
    const bool  isClassTemplateSpec{isCXXMethodDecl && isa<ClassTemplateSpecializationDecl>(methodDecl->getParent())};
    const bool  requiresComment{isCXXMethodDecl && not methodDecl->isUserProvided() &&
                               not methodDecl->isExplicitlyDefaulted()};

    if(methodDecl) {
        if(requiresComment) {
            mOutputFormatHelper.Append("// ");
        }

        isLambda             = methodDecl->getParent()->isLambda();
        isFirstCxxMethodDecl = (nullptr == methodDecl->getPreviousDecl());
    }

    if((isFirstCxxMethodDecl && (SkipAccess::No == skipAccess)) || cxxInheritedCtorDecl) {
        mOutputFormatHelper.Append(AccessToStringWithColon(decl));
    }

    // types of conversion decls can be invalid to type at this place. So introduce a using
    if(isa<CXXConversionDecl>(decl)) {
        mOutputFormatHelper.AppendNewLine(
            "using ", BuildRetTypeName(decl), " = ", GetName(GetDesugarReturnType(decl)), ";");
    }

    if(decl.isTemplated()) {
        if(decl.getDescribedTemplate()) {
            InsertTemplateParameters(*decl.getDescribedTemplate()->getTemplateParameters());
        }

    } else if(decl.isFunctionTemplateSpecialization()) {
        InsertTemplateSpecializationHeader();
    }

    InsertAttributes(decl.attrs());

    if(!decl.isFunctionTemplateSpecialization() || (isCXXMethodDecl && isFirstCxxMethodDecl)) {
        mOutputFormatHelper.Append(GetStorageClassAsStringWithSpace(decl.getStorageClass()));
    }

    if(Decl::FOK_None != decl.getFriendObjectKind()) {
        mOutputFormatHelper.Append("friend ");
    }

    if(decl.isInlined()) {
        mOutputFormatHelper.Append(kwInlineSpace);
    }

    if(methodDecl) {
        if(methodDecl->isVirtual()) {
            mOutputFormatHelper.Append(kwVirtualSpace);
        }

        if(const auto* ctorDecl = dyn_cast_or_null<CXXConstructorDecl>(methodDecl)) {
            if(isFirstCxxMethodDecl && ctorDecl->isExplicit()) {
                mOutputFormatHelper.Append("explicit ");
            }
        }
    }

    if(decl.isConstexpr()) {
        const bool skipConstexpr{isLambda};
        if(skipConstexpr) {
            mOutputFormatHelper.Append("/*");
        }

        mOutputFormatHelper.Append(kwConstExprSpace);

        if(skipConstexpr) {
            mOutputFormatHelper.Append("*/ ");
        }
    }

    // temporary output to be able to handle a return value of array reference
    OutputFormatHelper outputFormatHelper{};

    if(methodDecl) {
        if(not isFirstCxxMethodDecl || InsertNamespace()) {
            const auto* parent = methodDecl->getParent();
            outputFormatHelper.Append(parent->getNameAsString());

            outputFormatHelper.Append("::");
        }
    }

    if(!isa<CXXConversionDecl>(decl)) {
        if(isa<CXXConstructorDecl>(decl) || isa<CXXDestructorDecl>(decl)) {
            if(methodDecl) {
                if(isa<CXXDestructorDecl>(decl)) {
                    outputFormatHelper.Append('~');
                }

                outputFormatHelper.Append(GetName(*methodDecl->getParent()));
            }

        } else {
            outputFormatHelper.Append(GetName(decl));
        }

        if(!isLambda && isFirstCxxMethodDecl && decl.isFunctionTemplateSpecialization()) {
            CodeGenerator codeGenerator{outputFormatHelper};
            codeGenerator.InsertTemplateArgs(decl);
        }

        outputFormatHelper.Append("(");
    }

    // if a CXXInheritedCtorDecl was passed as a pointer us this to get the parameters from.
    if(cxxInheritedCtorDecl) {
        outputFormatHelper.AppendParameterList(cxxInheritedCtorDecl->parameters());
    } else {
        outputFormatHelper.AppendParameterList(decl.parameters());
    }

    if(decl.isVariadic()) {
        outputFormatHelper.Append(", ...");
    }

    outputFormatHelper.Append(")");

    if(!isa<CXXConstructorDecl>(decl) && !isa<CXXDestructorDecl>(decl)) {
        if(isa<CXXConversionDecl>(decl)) {
            mOutputFormatHelper.Append("operator ", BuildRetTypeName(decl), " (");
            mOutputFormatHelper.Append(outputFormatHelper.GetString());
        } else {
            const auto t = GetDesugarReturnType(decl);
            mOutputFormatHelper.Append(GetTypeNameAsParameter(t, outputFormatHelper.GetString()));
        }
    } else {
        mOutputFormatHelper.Append(outputFormatHelper.GetString());
    }

    mOutputFormatHelper.Append(GetConst(decl));

    if(methodDecl && methodDecl->isVolatile()) {
        mOutputFormatHelper.Append(kwSpaceVolatile);
    }

    switch(decl.getType()->getAs<FunctionProtoType>()->getRefQualifier()) {
        case RQ_None: break;
        case RQ_LValue: mOutputFormatHelper.Append(" &"); break;
        case RQ_RValue: mOutputFormatHelper.Append(" &&"); break;
    }

    mOutputFormatHelper.Append(GetNoExcept(decl));

    if(decl.isPure()) {
        mOutputFormatHelper.Append(" = 0");
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertCurlysIfRequired(const Stmt* stmt)
{
    const bool requiresCurlys{!isa<InitListExpr>(stmt) && !isa<ParenExpr>(stmt) && !isa<CXXDefaultInitExpr>(stmt)};

    if(requiresCurlys) {
        mOutputFormatHelper.Append('{');
    }

    InsertArg(stmt);

    if(requiresCurlys) {
        mOutputFormatHelper.Append('}');
    }
}
//-----------------------------------------------------------------------------

template<typename T>
void CodeGenerator::WrapInParensOrCurlys(const BraceKind braceKind, T&& lambda, const AddSpaceAtTheEnd addSpaceAtTheEnd)
{
    if(BraceKind::Curlys == braceKind) {
        mOutputFormatHelper.Append('{');
    } else {
        mOutputFormatHelper.Append('(');
    }

    lambda();

    if(BraceKind::Curlys == braceKind) {
        mOutputFormatHelper.Append('}');
    } else {
        mOutputFormatHelper.Append(')');
    }

    if(AddSpaceAtTheEnd::Yes == addSpaceAtTheEnd) {
        mOutputFormatHelper.Append(' ');
    }
}
//-----------------------------------------------------------------------------

template<typename T>
void CodeGenerator::WrapInParens(T&& lambda, const AddSpaceAtTheEnd addSpaceAtTheEnd)
{
    WrapInParensOrCurlys(BraceKind::Parens, std::forward<T>(lambda), addSpaceAtTheEnd);
}
//-----------------------------------------------------------------------------

template<typename T>
void CodeGenerator::WrapInParensIfNeeded(bool needsParens, T&& lambda, const AddSpaceAtTheEnd addSpaceAtTheEnd)
{
    if(needsParens) {
        WrapInParensOrCurlys(BraceKind::Parens, std::forward<T>(lambda), addSpaceAtTheEnd);
    } else {
        lambda();
    }
}
//-----------------------------------------------------------------------------

template<typename T>
void CodeGenerator::WrapInCurliesIfNeeded(bool needsParens, T&& lambda, const AddSpaceAtTheEnd addSpaceAtTheEnd)
{
    if(needsParens) {
        WrapInParensOrCurlys(BraceKind::Curlys, std::forward<T>(lambda), addSpaceAtTheEnd);
    } else {
        lambda();
    }
}
//-----------------------------------------------------------------------------

template<typename T>
void CodeGenerator::WrapInCurlys(T&& lambda, const AddSpaceAtTheEnd addSpaceAtTheEnd)
{
    WrapInParensOrCurlys(BraceKind::Curlys, std::forward<T>(lambda), addSpaceAtTheEnd);
}
//-----------------------------------------------------------------------------

static bool IsReference(const QualType& type)
{
    return GetDesugarType(type)->isLValueReferenceType();
}
//-----------------------------------------------------------------------------

static bool IsReference(const ValueDecl& valDecl)
{
    return IsReference(valDecl.getType());
}
//-----------------------------------------------------------------------------

void StructuredBindingsCodeGenerator::InsertDecompositionBindings(const DecompositionDecl& decompositionDeclStmt)
{
    for(const auto* bindingDecl : decompositionDeclStmt.bindings()) {
        if(const auto* binding = bindingDecl->getBinding()) {

            DPrint("sb name: %s\n", GetName(binding->getType()));

            const auto* holdingVarOrMemberExpr = [&]() -> const Expr* {
                if(const auto* holdingVar = bindingDecl->getHoldingVar()) {
                    return holdingVar->getAnyInitializer();
                }

                return dyn_cast_or_null<MemberExpr>(binding);
            }();

            const std::string refOrRefRef = [&]() -> std::string {
                const bool isRefToObject{IsReference(decompositionDeclStmt)};
                const bool isArrayBinding{isa<ArraySubscriptExpr>(binding) && isRefToObject};
                const bool isNotTemporary{holdingVarOrMemberExpr && !isa<ExprWithCleanups>(holdingVarOrMemberExpr)};
                const bool isTypeAlreadyCarryingRef{
                    bindingDecl->getType()
                        ->isLValueReferenceType()};  // In case of a lambda that captures variables
                                                     // and is expanded as a structured binding (#181), the
                                                     // type of GetName below already carries the "&".
                if((isArrayBinding || isNotTemporary) && not isTypeAlreadyCarryingRef) {
                    return "&";
                }

                return {};
            }();

            mOutputFormatHelper.Append(GetName(bindingDecl->getType()), refOrRefRef, " ", GetName(*bindingDecl), " = ");

            // tuple decomposition
            if(holdingVarOrMemberExpr) {
                InsertArg(holdingVarOrMemberExpr);

                // array decomposition
            } else if(const auto* arraySubscription = dyn_cast_or_null<ArraySubscriptExpr>(binding)) {
                InsertArg(arraySubscription);

            } else {
                TODO(bindingDecl, mOutputFormatHelper);
            }

            mOutputFormatHelper.AppendSemiNewLine();
        }
    }
}
//-----------------------------------------------------------------------------

void StructuredBindingsCodeGenerator::InsertArg(const DeclRefExpr* stmt)
{
    const auto name = GetName(*stmt);

    mOutputFormatHelper.Append(name);

    if(name.empty()) {
        mOutputFormatHelper.Append(mVarName);
    } else {
        InsertTemplateArgs(*stmt);
    }
}
//-----------------------------------------------------------------------------

void LambdaCodeGenerator::InsertArg(const CXXThisExpr* stmt)
{
    DPrint("thisExpr: imlicit=%d %s\n", stmt->isImplicit(), GetName(GetDesugarType(stmt->getType())));

    if(mCapturedThisAsCopy) {
        mOutputFormatHelper.Append("(&__this)");

    } else {
        mOutputFormatHelper.Append("__this");
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
