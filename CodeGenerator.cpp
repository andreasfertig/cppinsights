/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "CodeGenerator.h"
#include <algorithm>
#include "ClangCompat.h"
#include "DPrint.h"
#include "InsightsBase.h"
#include "InsightsMatchers.h"
#include "InsightsOnce.h"
#include "InsightsStrCat.h"
#include "NumberIterator.h"
#include "clang/Frontend/CompilerInstance.h"
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
                case LambdaCallerType::BinaryOperator: return &l;
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
    InsertArg(stmt->getBase());
    const std::string op{stmt->isArrow() ? "->" : "."};

    mOutputFormatHelper.Append(op, stmt->getMemberNameInfo().getAsString());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXForRangeStmt* rangeForStmt)
{
    mOutputFormatHelper.OpenScope();

    auto&      langOpts{GetLangOpts(*rangeForStmt->getLoopVariable())};
    const bool onlyCpp11{not langOpts.CPlusPlus14};

#if IS_CLANG_NEWER_THAN(7)
    // C++20 init-statement
    InsertArg(rangeForStmt->getInit());
#endif

    // range statement
    InsertArg(rangeForStmt->getRangeStmt());

    if(not onlyCpp11) {
        InsertArg(rangeForStmt->getBeginStmt());
        InsertArg(rangeForStmt->getEndStmt());
    }

    // add blank line after the declarations
    mOutputFormatHelper.AppendNewLine();

    mOutputFormatHelper.Append("for( ");

    if(not onlyCpp11) {
        mOutputFormatHelper.Append("; ");
    } else {
        mUseCommaInsteadOfSemi = UseCommaInsteadOfSemi::Yes;

        InsertArg(rangeForStmt->getBeginStmt());

        mUseCommaInsteadOfSemi = UseCommaInsteadOfSemi::No;
        mSkipVarDecl           = SkipVarDecl::Yes;
        InsertArg(rangeForStmt->getEndStmt());
        mSkipVarDecl = SkipVarDecl::No;
    }

    InsertArg(rangeForStmt->getCond());

    mOutputFormatHelper.Append("; ");

    InsertArg(rangeForStmt->getInc());

    mOutputFormatHelper.AppendNewLine(" )");
    // open for loop scope
    mOutputFormatHelper.OpenScope();

    InsertArg(rangeForStmt->getLoopVariable());

    const auto* body         = rangeForStmt->getBody();
    const bool  isBodyBraced = isa<CompoundStmt>(body);

    /* we already opened a scope. Skip the initial one */
    if(!isBodyBraced) {
        InsertArg(body);
    } else {
        HandleCompoundStmt(dyn_cast_or_null<CompoundStmt>(body));
    }

    if(!isBodyBraced && !isa<NullStmt>(body)) {
        mOutputFormatHelper.AppendSemiNewLine();
    }

    // close range-for scope in for
    mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::Yes);

    // close outer range-for scope
    mOutputFormatHelper.CloseScope();
    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const UnresolvedLookupExpr* stmt)
{
    mOutputFormatHelper.Append(stmt->getName().getAsString());
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
    mOutputFormatHelper.Append("while");
    WrapInParens([&]() { InsertArg(stmt->getCond()); }, AddSpaceAtTheEnd::Yes);

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
        // Handle a special case where we have a lambda static invoke operator. In that case use the appropriate using
        // retType as return type
        if(const auto* m = dyn_cast_or_null<CXXMethodDecl>(meDecl)) {
            if(const auto* rd = m->getParent(); rd && rd->isLambda()) {
                skipTemplateArgs = true;

                return StrCat("operator ", GetLambdaName(*rd), "::retType");
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
            if(const auto* tmplArgs = cxxMethod->getAsFunction()->getTemplateSpecializationArgs()) {
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

    InsertArg(stmt->getLHS());
    mOutputFormatHelper.Append(" ", stmt->getOpcodeStr(), " ");
    InsertArg(stmt->getRHS());
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

/*
 * Go deep in a Stmt if necessary and look to all childs for a DeclRefExpr.
 */
static const DeclRefExpr* FindDeclRef(const Stmt* stmt)
{
    if(const auto* dref = dyn_cast_or_null<DeclRefExpr>(stmt)) {
        return dref;
    } else if(const auto* arrayInitExpr = dyn_cast_or_null<ArrayInitLoopExpr>(stmt)) {
        const auto* srcExpr = arrayInitExpr->getCommonExpr()->getSourceExpr();

        if(const auto* arrayDeclRefExpr = dyn_cast_or_null<DeclRefExpr>(srcExpr)) {
            return arrayDeclRefExpr;
        }
    } else if(const auto func = dyn_cast_or_null<CXXFunctionalCastExpr>(stmt)) {
        //        TODO(stmt, "");
    }

    if(stmt) {
        for(const auto* child : stmt->children()) {
            if(const auto* childRef = FindDeclRef(child)) {
                return childRef;
            }
        }
    }

    return nullptr;
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const DecompositionDecl* decompositionDeclStmt)
{
    const auto baseVarName{[&]() {
        if(const auto* declName = FindDeclRef(decompositionDeclStmt->getInit())) {
            std::string name = GetPlainName(*declName);

            const std::string operatorName{"operator"};
            if(Contains(name, operatorName)) {
                return operatorName;
            }

            return name;
        }

        // We approached an unnamed decl. This happens for example like this: auto& [x, y] = Point{};
        return std::string{""};
    }()};

    const std::string tmpVarName{
        BuildInternalVarName(baseVarName, GetBeginLoc(decompositionDeclStmt), GetSM(*decompositionDeclStmt))};

    mOutputFormatHelper.Append(GetTypeNameAsParameter(decompositionDeclStmt->getType(), tmpVarName), " = ");

    InsertArg(decompositionDeclStmt->getInit());

    mOutputFormatHelper.AppendSemiNewLine();

    for(const auto* bindingDecl : decompositionDeclStmt->bindings()) {
        if(const auto* binding = bindingDecl->getBinding()) {

            DPrint("sb name: %s\n", GetName(binding->getType()));

            const auto* holdingVarOrMemberExpr = [&]() -> const Expr* {
                if(const auto* holdingVar = bindingDecl->getHoldingVar()) {
                    return holdingVar->getAnyInitializer();
                }

                return dyn_cast_or_null<MemberExpr>(binding);
            }();

            const std::string refOrRefRef = [&]() -> std::string {
                const bool isRefToObject{IsReference(*decompositionDeclStmt)};
                const bool isArrayBinding{isa<ArraySubscriptExpr>(binding) && isRefToObject};
                const bool isNotTemporary{holdingVarOrMemberExpr && !isa<ExprWithCleanups>(holdingVarOrMemberExpr)};
                if(isArrayBinding || isNotTemporary) {
                    return "&";
                }

                return "";
            }();

            mOutputFormatHelper.Append(GetName(bindingDecl->getType()), refOrRefRef, " ", GetName(*bindingDecl), " = ");

            // tuple decomposition
            if(holdingVarOrMemberExpr) {
                DPrint("4444\n");

                StructuredBindingsCodeGenerator codeGenerator{mOutputFormatHelper, tmpVarName};
                codeGenerator.InsertArg(holdingVarOrMemberExpr);

                // array decomposition
            } else if(const auto* arraySubscription = dyn_cast_or_null<ArraySubscriptExpr>(binding)) {
                mOutputFormatHelper.Append(tmpVarName);

                InsertArg(arraySubscription);

            } else {
                TODO(bindingDecl, mOutputFormatHelper);
            }

            mOutputFormatHelper.AppendSemiNewLine();
        }
    }
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

void CodeGenerator::InsertArg(const VarDecl* stmt)
{
    LAMBDA_SCOPE_HELPER(VarDecl);

    const bool isTemplateSpecialization{isa<VarTemplateSpecializationDecl>(stmt)};

    if(isTemplateSpecialization) {
        mOutputFormatHelper.AppendNewLine("template<>");
    }

    if(const auto* decompDecl = dyn_cast_or_null<DecompositionDecl>(stmt)) {
        InsertArg(decompDecl);
    } else if(IsTrivialStaticClassVarDecl(*stmt)) {
        HandleLocalStaticNonTrivialClass(stmt);

    } else {
        if(SkipVarDecl::No == mSkipVarDecl) {
            mOutputFormatHelper.Append(GetQualifiers(*stmt));

            if(const auto type = stmt->getType(); type->isFunctionPointerType()) {
                const auto        lineNo = GetSM(*stmt).getSpellingLineNumber(stmt->getSourceRange().getBegin());
                const std::string funcPtrName{StrCat("FuncPtr_", lineNo, " ")};

                mOutputFormatHelper.AppendNewLine("using ", funcPtrName, "= ", GetName(type), ";");
                mOutputFormatHelper.Append(funcPtrName, GetName(*stmt));
            } else {
                const auto varName = [&]() {
                    std::string name{GetName(*stmt)};

                    if(const auto* tvd = dyn_cast_or_null<VarTemplateSpecializationDecl>(stmt)) {
                        OutputFormatHelper outputFormatHelper;
                        CodeGenerator      codeGenerator{outputFormatHelper};

                        codeGenerator.InsertTemplateArgs(tvd->getTemplateArgs().asArray());

                        name += outputFormatHelper.GetString();
                    }

                    return name;
                }();

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

        if(UseCommaInsteadOfSemi::No == mUseCommaInsteadOfSemi) {
            mOutputFormatHelper.AppendSemiNewLine();
        } else {
            mOutputFormatHelper.Append(',');
        }
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const FunctionDecl* stmt)
{
    //    LAMBDA_SCOPE_HELPER(VarDecl);

    if(const auto* ctor = dyn_cast_or_null<CXXConstructorDecl>(stmt)) {
        InsertArg(ctor);
    } else {
        InsertAccessModifierAndNameWithReturnType(*stmt, SkipAccess::Yes);

        if(const auto* md = dyn_cast_or_null<CXXMethodDecl>(stmt); (md && md->isLambdaStaticInvoker())) {
            mOutputFormatHelper.AppendNewLine();
            const auto* Lambda = md->getParent();
            const auto* CallOp = Lambda->getLambdaCallOperator();
            if(Lambda->isGenericLambda() && md->isFunctionTemplateSpecialization()) {

                const TemplateArgumentList* TAL            = md->getTemplateSpecializationArgs();
                FunctionTemplateDecl*       CallOpTemplate = CallOp->getDescribedFunctionTemplate();
                void*                       InsertPos      = nullptr;
                FunctionDecl*               CorrespondingCallOpSpecialization =
                    CallOpTemplate->findSpecialization(TAL->asArray(), InsertPos);
                CallOp = cast<CXXMethodDecl>(CorrespondingCallOpSpecialization);
            }

            InsertArg(CallOp->getBody());
            mOutputFormatHelper.AppendNewLine();
        } else if(stmt->doesThisDeclarationHaveABody()) {
            mOutputFormatHelper.AppendNewLine();
            InsertArg(stmt->getBody());
            mOutputFormatHelper.AppendNewLine();
        } else {
            mOutputFormatHelper.AppendSemiNewLine();
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

void CodeGenerator::InsertArg(const InitListExpr* stmt)
{
    WrapInCurlys([&]() {
        mOutputFormatHelper.IncreaseIndent();

        ForEachArg(stmt->inits(), [&](const auto& init) { InsertArg(init); });

        // If we have a filler, fill the rest of the array with the filler expr.
        if(const auto* filler = stmt->getArrayFiller()) {
            const auto fullWidth = [&]() -> uint64_t {
                if(const auto* ct = dyn_cast_or_null<ConstantArrayType>(stmt->getType().getTypePtrOrNull())) {
                    const auto v = ct->getSize().getZExtValue();

                    // clamp here to survive large arrays.
                    return std::clamp(v, uint64_t(0), uint64_t(100));
                }

                return 0;
            }();

            // now fill the remaining array slots.
            OnceFalse needsComma{0 != stmt->getNumInits()};
            for_each(static_cast<uint64_t>(stmt->getNumInits()), fullWidth, [&](auto) {
                mOutputFormatHelper.AppendComma(needsComma);

                InsertArg(filler);
            });
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
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const StringLiteral* stmt)
{
    std::string              data{};
    llvm::raw_string_ostream stream{data};
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
    InsertArg(stmt->getLHS());

    mOutputFormatHelper.Append('[');
    InsertArg(stmt->getRHS());
    mOutputFormatHelper.Append(']');
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

    InsertArg(stmt->getCallee());

    if(isa<UserDefinedLiteral>(stmt)) {
        if(const auto* DRE = cast<DeclRefExpr>(stmt->getCallee()->IgnoreImpCasts())) {
            if(const TemplateArgumentList* Args = cast<FunctionDecl>(DRE->getDecl())->getTemplateSpecializationArgs()) {
                if(1 != Args->size()) {
                    InsertTemplateArgs(Args->asArray());
                } else {
                    mOutputFormatHelper.Append('<');

                    const TemplateArgument& Pack = Args->get(0);

                    ForEachArg(Pack.pack_elements(), [&](const auto& arg) {
                        const char C{static_cast<char>(arg.getAsIntegral().getZExtValue())};
                        mOutputFormatHelper.Append("'", std::string{C}, "'");
                    });

                    mOutputFormatHelper.Append('>');
                }
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

    if(!clang::ast_matchers::IsMatchingCast(castKind)) {
        InsertArg(subExpr);
    } else if(isa<IntegerLiteral>(subExpr)) {
        InsertArg(stmt->IgnoreCasts());

    } else {
        static const std::string castName{GetCastName(castKind)};
        const QualType           castDestType{stmt->getType().getCanonicalType()};

        FormatCast(castName, castDestType, subExpr, castKind, AsComment::No);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const DeclRefExpr* stmt)
{
    const auto* Ctx = stmt->getDecl()->getDeclContext();
    if(!Ctx->isFunctionOrMethod()) {
        ParseDeclContext(Ctx);

        mOutputFormatHelper.Append(GetPlainName(*stmt));

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
    mOutputFormatHelper.Append("for");

    WrapInParens(
        [&]() {
            if(const auto* init = stmt->getInit()) {
                InsertArg(init);

                // the init-stmt carries a ; at the end plus a newline. Remove and replace it with
                // a space
                mOutputFormatHelper.RemoveIndentIncludingLastNewLine();
            } else {
                mOutputFormatHelper.Append("; ");
            }

            InsertArg(stmt->getCond());
            mOutputFormatHelper.Append("; ");

            InsertArg(stmt->getInc());
        },
        AddSpaceAtTheEnd::Yes);

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

    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CStyleCastExpr* stmt)
{
    const auto        castKind     = stmt->getCastKind();
    const std::string castName     = GetCastName(castKind);
    const QualType    castDestType = stmt->getType().getCanonicalType();

    FormatCast(castName, castDestType, stmt->getSubExpr(), castKind, AsComment::No);
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
            codeGenerator.InsertArg(stmt->getArraySize());
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
    InsertArg(stmt->getTemporary());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXOperatorCallExpr* stmt)
{
    LAMBDA_SCOPE_HELPER(OperatorCallExpr);

    const auto* callee = dyn_cast_or_null<DeclRefExpr>(stmt->getCallee()->IgnoreImpCasts());
    const bool  isCXXMethod{callee && isa<CXXMethodDecl>(callee->getDecl())};

    if(2 == stmt->getNumArgs()) {
        const auto* param1 = dyn_cast_or_null<DeclRefExpr>(stmt->getArg(0)->IgnoreImpCasts());
        const auto* param2 = dyn_cast_or_null<DeclRefExpr>(stmt->getArg(1)->IgnoreImpCasts());

        if(callee && param1 && param2) {

            const std::string replace = [&]() {
                if(isa<CXXMethodDecl>(callee->getDecl())) {
                    const std::string tmpl = [&] {
                        if(const auto* tvd = dyn_cast_or_null<VarTemplateSpecializationDecl>(param1->getDecl())) {
                            OutputFormatHelper outputFormatHelper{};
                            CodeGenerator      codeGenerator{outputFormatHelper};

                            codeGenerator.InsertTemplateArgs(tvd->getTemplateArgs().asArray());

                            return outputFormatHelper.GetString();
                        }

                        return std::string{};
                    }();

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
        if(callee) {
            mOutputFormatHelper.Append(GetName(*callee));
        } else {
            InsertArg(stmt->getCallee()->IgnoreImpCasts());
        }

        mOutputFormatHelper.Append('(');
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
#if IS_CLANG_NEWER_THAN(7)
        const auto name = PredefinedExpr::getIdentKindName(stmt->getIdentKind());
#else
        const auto name = PredefinedExpr::getIdentTypeName(stmt->getIdentType());
#endif
        mOutputFormatHelper.Append(name.str());
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ExprWithCleanups* stmt)
{
    InsertArg(stmt->getSubExpr());
}
//-----------------------------------------------------------------------------

static std::string getValueOfValueInit(const QualType& t)
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
                    return getValueOfValueInit(complexType->getElementType());
                }

                break;

#if IS_CLANG_NEWER_THAN(7)
            case Type::STK_FixedPoint: Error("STK_FixedPoint is not implemented"); break;
#endif
        }

    } else if(const auto* tt = dyn_cast_or_null<ConstantArrayType>(t.getTypePtrOrNull())) {
        const auto&       elementType{tt->getElementType()};
        const std::string elementTypeInitValue{getValueOfValueInit(elementType)};
        const auto        size{std::clamp(tt->getSize().getZExtValue(), uint64_t(0), uint64_t(100))};
        std::string       ret{};

        OnceFalse needsComma{};
        for_each(static_cast<uint64_t>(0), size, [&](auto) {
            if(needsComma) {
                ret.append(", ");
            }

            ret.append(elementTypeInitValue);
        });

        return ret;
    }

    return "0";
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ImplicitValueInitExpr* stmt)
{
    mOutputFormatHelper.Append(getValueOfValueInit(stmt->getType()));
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

#if IS_CLANG_NEWER_THAN(7)
void CodeGenerator::InsertArg(const ConstantExpr* stmt)
{
    InsertArg(stmt->getSubExpr());
}
#endif
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const TypeAliasDecl* stmt)
{
    mOutputFormatHelper.Append("using ", GetName(*stmt), " = ");

    if(auto* templateSpecializationType = stmt->getUnderlyingType()->getAs<TemplateSpecializationType>()) {
        std::string              name{};
        llvm::raw_string_ostream stream(name);
        templateSpecializationType->getTemplateName().dump(stream);

        mOutputFormatHelper.Append(stream.str(), "<");

        ForEachArg(templateSpecializationType->template_arguments(), [&](const auto& arg) {
            if(arg.getKind() == TemplateArgument::Expression) {
                InsertArg(arg.getAsExpr());
            } else {
                InsertTemplateArg(arg);
            }
        });

        mOutputFormatHelper.Append(">");
    } else {
        mOutputFormatHelper.Append(GetName(stmt->getUnderlyingType()));
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

void CodeGenerator::InsertArg(const CXXMethodDecl* stmt)
{
    OutputFormatHelper initOutputFormatHelper{};
    initOutputFormatHelper.SetIndent(mOutputFormatHelper, OutputFormatHelper::SkipIndenting::Yes);
    CXXConstructorDecl* cxxInheritedCtorDecl{nullptr};

    if(not stmt->isUserProvided()) {
        mOutputFormatHelper.Append("// ");
    }

    // travers the ctor inline init statements first to find a potential CXXInheritedCtorInitExpr. This carries the name
    // and the type. The CXXMethodDecl above knows only the type.
    if(const auto* ctor = dyn_cast_or_null<CXXConstructorDecl>(stmt)) {
        CodeGenerator codeGenerator{initOutputFormatHelper};
        OnceTrue      first{};

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

                if(const auto* cxxInheritedCtorInitExpr = dyn_cast_or_null<CXXInheritedCtorInitExpr>(inlineInit)) {
                    cxxInheritedCtorDecl = cxxInheritedCtorInitExpr->getConstructor();
                }

                codeGenerator.InsertArg(inlineInit);
            }
        }
    }

    InsertAccessModifierAndNameWithReturnType(*stmt, SkipAccess::Yes, cxxInheritedCtorDecl);

    if(stmt->isDefaulted()) {
        mOutputFormatHelper.AppendNewLine(" = default;");
    } else if(stmt->isDeleted()) {
        mOutputFormatHelper.AppendNewLine(" = delete;");
    }

    if(!stmt->isUserProvided()) {
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

    if(stmt->doesThisDeclarationHaveABody() && not stmt->isLambdaStaticInvoker()) {
        mOutputFormatHelper.AppendNewLine();
        InsertArg(stmt->getBody());
        mOutputFormatHelper.AppendNewLine();

    } else if(stmt->isLambdaStaticInvoker()) {
        mOutputFormatHelper.AppendNewLine();
        const auto* lambda = stmt->getParent();
        const auto* callOp = lambda->getLambdaCallOperator();
        if(lambda->isGenericLambda() && stmt->isFunctionTemplateSpecialization()) {
            const TemplateArgumentList* tal            = stmt->getTemplateSpecializationArgs();
            FunctionTemplateDecl*       callOpTemplate = callOp->getDescribedFunctionTemplate();
            void*                       InsertPos      = nullptr;
            FunctionDecl*               CorrespondingCallOpSpecialization =
                callOpTemplate->findSpecialization(tal->asArray(), InsertPos);
            callOp = cast<CXXMethodDecl>(CorrespondingCallOpSpecialization);
        }

        InsertArg(callOp->getBody());
        mOutputFormatHelper.AppendNewLine();

    } else {
        mOutputFormatHelper.AppendSemiNewLine();
    }

    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const EnumDecl* stmt)
{
    mOutputFormatHelper.Append("enum ");

    if(stmt->isScoped()) {
        if(stmt->isScopedUsingClassTag()) {
            mOutputFormatHelper.Append("class ");
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
    if(stmt->isMutable()) {
        mOutputFormatHelper.Append("mutable ");
    }

    if(const auto* cxxRecordDecl = dyn_cast_or_null<CXXRecordDecl>(stmt->getParent())) {
        std::string name{GetName(*stmt)};
        if(cxxRecordDecl->isLambda()) {
            llvm::DenseMap<const VarDecl*, FieldDecl*> Captures{};
            FieldDecl*                                 ThisCapture{};

            cxxRecordDecl->getCaptureFields(Captures, ThisCapture);

            if(stmt == ThisCapture) {
                name = "__this";
            } else {
                for(const auto& [key, value] : Captures) {
                    if(value == stmt) {
                        name = GetName(*key);
                        break;
                    }
                }
            }
        }

        mOutputFormatHelper.Append(GetTypeNameAsParameter(stmt->getType(), name));

        // Keep the inline init for aggregates, as we do not see it somewhere else.
        if(cxxRecordDecl->isAggregate()) {
            const auto* initializer = stmt->getInClassInitializer();
            if(stmt->hasInClassInitializer() && initializer) {
                if(ICIS_ListInit != stmt->getInClassInitStyle()) {
                    mOutputFormatHelper.Append(" = ");
                }

                InsertArg(initializer);
            }
        }
    } else {
        mOutputFormatHelper.Append(GetTypeNameAsParameter(stmt->getType(), GetName(*stmt)));
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
    std::string ns{};

    if(const auto* qualifier = stmt->getQualifier()) {
        ns = GetName(*qualifier->getAsNamespace());
    }

    if(!ns.empty()) {
        ns.append("::");
    }

    mOutputFormatHelper.AppendNewLine("using namespace ", ns, GetName(*stmt->getNominatedNamespace()), ";");
}
//-----------------------------------------------------------------------------

void CodeGenerator::print(const NestedNameSpecifier* stmt)
{
    if(!stmt) {
        return;
    }

    if(const auto* prefix = stmt->getPrefix()) {
        print(prefix);
    }

    switch(stmt->getKind()) {
        case NestedNameSpecifier::Identifier: mOutputFormatHelper.Append(stmt->getAsIdentifier()->getName()); break;

        case NestedNameSpecifier::Namespace:
            if(stmt->getAsNamespace()->isAnonymousNamespace()) {
                return;
            }

            mOutputFormatHelper.Append(stmt->getAsNamespace()->getName());
            break;

        case NestedNameSpecifier::NamespaceAlias:
            mOutputFormatHelper.Append(stmt->getAsNamespaceAlias()->getName());
            break;

        case NestedNameSpecifier::TypeSpecWithTemplate:
        case NestedNameSpecifier::TypeSpec: {

            const Type* T = stmt->getAsType();
            mOutputFormatHelper.Append(GetName(QualType(T, 0)));

            if(const auto* SpecType = dyn_cast<TemplateSpecializationType>(T)) {
                InsertTemplateArgs(SpecType->template_arguments());
                //            } else if(const auto* subs = dyn_cast_or_null<SubstTemplateTypeParmType>(T)) {
                //                mOutputFormatHelper.Append(GetName(subs->getReplacementType()));
            }
        } break;

        default: break;
    }

    mOutputFormatHelper.Append("::");
}
//-----------------------------------------------------------------------------

void CodeGenerator::ParseDeclContext(const DeclContext* Ctx)
{
    using ContextsTy = SmallVector<const DeclContext*, 8>;
    ContextsTy Contexts;

    while(Ctx) {
        if(isa<NamedDecl>(Ctx)) {
            Contexts.push_back(Ctx);
        }
        Ctx = Ctx->getParent();
    }

    for(const auto* DC : llvm::reverse(Contexts)) {
        if(const auto* Spec = dyn_cast<ClassTemplateSpecializationDecl>(DC)) {
            mOutputFormatHelper.Append(Spec->getName());
            InsertTemplateArgs(*Spec);

        } else if(const auto* ND = dyn_cast<NamespaceDecl>(DC)) {
            if(ND->isAnonymousNamespace() || ND->isInline()) {
                continue;
            }

            mOutputFormatHelper.Append(ND->getNameAsString());

        } else if(const auto* RD = dyn_cast<RecordDecl>(DC)) {
            if(!RD->getIdentifier()) {
                continue;
            }

            mOutputFormatHelper.Append(RD->getNameAsString());

        } else if(dyn_cast<FunctionDecl>(DC)) {
            continue;

        } else if(const auto* ED = dyn_cast<EnumDecl>(DC)) {
            if(!ED->isScoped()) {
                continue;
            }

            mOutputFormatHelper.Append(ED->getNameAsString());

        } else {
            mOutputFormatHelper.Append(cast<NamedDecl>(DC)->getNameAsString());
        }

        mOutputFormatHelper.Append("::");
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const UsingDecl* stmt)
{
    mOutputFormatHelper.Append("using ");

    // own implementation due to lambdas
    if(const DeclContext* Ctx = stmt->getDeclContext()) {
        if(Ctx->isFunctionOrMethod()) {
            print(stmt->getQualifier());

        } else {
            ParseDeclContext(Ctx);
        }
    }

    mOutputFormatHelper.AppendNewLine(stmt->getNameAsString(), ";");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const NamespaceAliasDecl* stmt)
{
    mOutputFormatHelper.Append("namespace ", stmt->getDeclName().getAsString(), " = ");

    print(stmt->getQualifier());

    mOutputFormatHelper.AppendNewLine(GetName(*stmt->getAliasedNamespace()), ";");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const FriendDecl* stmt)
{
    mOutputFormatHelper.Append("friend ");

    if(const auto* typeInfo = stmt->getFriendType()) {
        mOutputFormatHelper.Append(GetName(typeInfo->getType()));

    } else {
        if(const auto* fd = stmt->getFriendDecl()) {
            if(const auto* functionDecl = dyn_cast_or_null<FunctionDecl>(fd)) {
                InsertArg(functionDecl);

            } else {
                TODO(stmt, mOutputFormatHelper);
            }
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

void CodeGenerator::InsertArg(const FunctionTemplateDecl* stmt)
{
    for(const auto spec : stmt->specializations()) {
        InsertArg(spec->getAsFunction());
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const TypeAliasTemplateDecl* stmt)
{
    InsertArg(stmt->getTemplatedDecl());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXRecordDecl* stmt)
{
    if(dyn_cast_or_null<ClassTemplateSpecializationDecl>(stmt)) {
        mOutputFormatHelper.AppendNewLine("template<>");
    }

    if(stmt->isClass()) {
        mOutputFormatHelper.Append(kwClassSpace);

    } else {
        mOutputFormatHelper.Append("struct ");
    }

    mOutputFormatHelper.Append(GetName(*stmt));

    // skip classes/struct's without a definition
    if(!stmt->hasDefinition()) {
        mOutputFormatHelper.AppendSemiNewLine();
        return;
    }

    if(const auto* clsTmpl = dyn_cast_or_null<ClassTemplateSpecializationDecl>(stmt)) {
        InsertTemplateArgs(*clsTmpl);
    }

    if(stmt->getNumBases()) {
        mOutputFormatHelper.Append(" : ");

        ForEachArg(stmt->bases(), [&](const auto& base) {
            mOutputFormatHelper.Append(AccessToString(base.getAccessSpecifier()), " ", GetName(base.getType()));
        });
    }

    mOutputFormatHelper.AppendNewLine();
    mOutputFormatHelper.OpenScope();

    OnceTrue   firstRecordDecl{};
    OnceTrue   firstDecl{};
    Decl::Kind formerKind{};
    for(const auto* d : stmt->decls()) {
        if(isa<CXXRecordDecl>(d) && firstRecordDecl) {
            continue;
        }

        // Insert a newline when the decl kind changes. This for example, inserts a newline when after a FieldDecl we
        // see a CXXMethodDecl.
        if(not firstDecl && (d->getKind() != formerKind)) {
            // mOutputFormatHelper.AppendNewLine();
        }

        if((stmt->isLambda() && isa<CXXDestructorDecl>(d))) {
            continue;
        }

        InsertArg(d);
        formerKind = d->getKind();
    }

    if(stmt->isLambda()) {
        const LambdaCallerType lambdaCallerType = mLambdaStack.back().callerType();
        const bool             ctorRequired{stmt->captures_begin() != stmt->captures_end()};

        if(ctorRequired) {
            mOutputFormatHelper.Append("public: ", GetName(*stmt), "(");
        }

        std::string ctorInitializerList{};
        std::string ctorInitializers{"{"};

        OnceTrue bFirst{};

        auto addToInits = [&](std::string name, const FieldDecl* fd, bool isThis, const Expr* expr) {
            if(bFirst) {
                ctorInitializerList.append(": ");
            } else {
                mOutputFormatHelper.Append(", ");
                ctorInitializerList.append("\n, ");
                ctorInitializers.append(", ");
            }

            if(isThis) {
                ctorInitializerList.append(StrCat("__", name, "{", "_", name, "}\n"));

            } else {
                ctorInitializerList.append(StrCat(name, "{", "_", name, "}"));
            }

            if(not isThis && expr) {
                OutputFormatHelper ofm{};
                CodeGenerator      codeGenerator{ofm, mLambdaStack};
                codeGenerator.InsertArg(expr);
                ctorInitializers.append(ofm.GetString());
            } else {
                if(isThis && not fd->getType()->isPointerType()) {
                    ctorInitializers.append("*");
                }

                ctorInitializers.append(name);
            }

            mOutputFormatHelper.Append(GetTypeNameAsParameter(fd->getType(), StrCat("_", name)));
        };

        llvm::DenseMap<const VarDecl*, FieldDecl*> Captures{};
        FieldDecl*                                 ThisCapture{};

        stmt->getCaptureFields(Captures, ThisCapture);

        const auto* captureInits = mLambdaExpr->capture_init_begin();
        const auto* captureInit  = *captureInits;

        // Check if it captures this
        if(ThisCapture) {
            addToInits("this", ThisCapture, true, captureInit);
        } else {
            // Find the corresponding capture in the DenseMap. The DenseMap seems to be change its order each time.
            // Hence we use \c captures() to keep the order stable. While using \c Captures to generate the code as
            // it carries the better type infos.
            for(const auto& c : mLambdaExpr->captures()) {
                captureInit = *captureInits;
                ++captureInits;

                if(not c.capturesVariable()) {
                    continue;
                }

                for(const auto& [key, value] : Captures) {
                    if(key == c.getCapturedVar()) {
                        addToInits(GetName(*key), value, false, captureInit);
                        break;
                    }
                }
            }
        }

        ctorInitializers.append("}");

        // generate the ctor only if it is required, i.e. we have captures. This is in fact a trick to get compiliing
        // code out of it. The compiler itself does not generate a constructor in many many cases.
        if(ctorRequired) {
            mOutputFormatHelper.AppendNewLine(")");
            mOutputFormatHelper.AppendNewLine(ctorInitializerList);
            mOutputFormatHelper.AppendNewLine("{}");
        }

        // close the class scope
        mOutputFormatHelper.CloseScope();

        if((LambdaCallerType::VarDecl != lambdaCallerType) && (LambdaCallerType::CallExpr != lambdaCallerType)) {
            mOutputFormatHelper.Append(" ", GetLambdaName(*stmt), ctorInitializers);
        } else {
            mLambdaStack.back().inits().append(ctorInitializers);
        }

    } else {
        mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::Yes);
    }

    mOutputFormatHelper.AppendSemiNewLine();
    mOutputFormatHelper.AppendNewLine();
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
    mOutputFormatHelper.Append(stmt->getPackLength());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ReturnStmt* stmt)
{
    LAMBDA_SCOPE_HELPER(ReturnStmt);

    mOutputFormatHelper.Append("return");

    if(const auto* retVal = stmt->getRetValue()) {
        mOutputFormatHelper.Append(' ');
        InsertArg(retVal);
    }
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
    // No qualifiers like const or volatile here. This appears in  function calls or operators as a parameter. CV's
    // are not allowed there.
    mOutputFormatHelper.Append(GetName(stmt->getType(), Unqualified::Yes));
    InsertArg(stmt->getSubExpr());
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
                               const CastKind&   castKind,
                               const AsComment)
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
        InsertTemplateArgs(tmplSpecType->template_arguments());
    } else {
        InsertTemplateArgs(clsTemplateSpe.getTemplateArgs().asArray());
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertTemplateArgs(const DeclRefExpr& stmt)
{
    if(stmt.getNumTemplateArgs()) {
        mOutputFormatHelper.Append('<');

        ForEachArg(stmt.template_arguments(), [&](const auto& arg) {
            const auto& targ = arg.getArgument();

            InsertTemplateArg(targ);
        });

        mOutputFormatHelper.Append('>');
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertTemplateArgs(const ArrayRef<TemplateArgument>& array)
{
    mOutputFormatHelper.Append('<');

    ForEachArg(array, [&](const auto& arg) { InsertTemplateArg(arg); });

    /* put as space between to closing brackets: >> -> > > */
    if(mOutputFormatHelper.GetString().back() == '>') {
        mOutputFormatHelper.Append(' ');
    }

    mOutputFormatHelper.Append('>');
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
        case TemplateArgument::Integral: mOutputFormatHelper.Append(arg.getAsIntegral()); break;
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
    const auto*       cxxRecordDecl = stmt->getType()->getAsCXXRecordDecl();
    const std::string internalVarName{BuildInternalVarName(GetName(*stmt))};
    const std::string compilerBoolVarName{StrCat(internalVarName, "B")};
    const std::string typeName{GetName(*cxxRecordDecl)};

    // insert compiler bool to track init state
    mOutputFormatHelper.AppendNewLine("static bool ", compilerBoolVarName, ";");

    // insert compiler memory place holder
    mOutputFormatHelper.AppendNewLine("static char ", internalVarName, "[sizeof(", typeName, ")];");

    // insert compiler init if
    mOutputFormatHelper.AppendNewLine();

    mOutputFormatHelper.AppendNewLine("if( ! ", compilerBoolVarName, " )");
    mOutputFormatHelper.OpenScope();

    mOutputFormatHelper.AppendNewLine("new (&", internalVarName, ") ", typeName, ";");

    mOutputFormatHelper.AppendNewLine(compilerBoolVarName, " = true;");
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
        CASE(UInt, "u");
        CASE(ULong, "ul");
        CASE(ULongLong, "ull");
        CASE(UInt128, "ulll");
        CASE(Long, "l");
        CASE(LongLong, "ll");
        CASE(Float, "f");
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

    if(methodDecl) {
        if(not methodDecl->isUserProvided()) {
            mOutputFormatHelper.Append("// ");
        }

        isLambda             = methodDecl->getParent()->isLambda();
        isFirstCxxMethodDecl = (nullptr == methodDecl->getPreviousDecl());

#if 0
        if(not methodDecl->isUserProvided()) {
            mOutputFormatHelper.Append("// ");
        }
#endif
    }

    if((isFirstCxxMethodDecl && (SkipAccess::No == skipAccess)) || isLambda || cxxInheritedCtorDecl) {
        mOutputFormatHelper.Append(AccessToStringWithColon(decl));
    }

    if(!isLambda && ((isFirstCxxMethodDecl && decl.isFunctionTemplateSpecialization()) ||
                     (!isFirstCxxMethodDecl && isClassTemplateSpec))) {
        mOutputFormatHelper.AppendNewLine("template<>");
    }

    // types of conversion decls can be invalid to type at this place. So introduce a using
    if(isa<CXXConversionDecl>(decl)) {
        mOutputFormatHelper.AppendNewLine("using retType = ", GetName(GetDesugarReturnType(decl)), ";");
    }

    if(!decl.isFunctionTemplateSpecialization() || (isCXXMethodDecl && isFirstCxxMethodDecl)) {
        mOutputFormatHelper.Append(GetStorageClassAsStringWithSpace(decl.getStorageClass()));
    }

    if(methodDecl) {
        if(methodDecl->getPreviousDecl()) {
            if(const auto* ct = methodDecl->getParent()->getDescribedClassTemplate()) {
                mOutputFormatHelper.Append("template<");

                OutputFormatHelper::ForEachArg(
                    ct->getTemplateParameters()->asArray(), mOutputFormatHelper, [&](const auto* pm) {
                        if(const auto* vd = dyn_cast_or_null<ValueDecl>(pm)) {
                            mOutputFormatHelper.Append(GetName(vd->getType()), " ");
                        }

                        mOutputFormatHelper.Append(GetName(*pm));
                    });

                mOutputFormatHelper.AppendNewLine(">");
            }
        }
    }

    if(decl.isInlined()) {
        mOutputFormatHelper.Append(kwInlineSpace);
    }

    if(methodDecl) {
        if(methodDecl->isVirtual()) {
            mOutputFormatHelper.Append(kwVirtualSpace);
        }

        if(methodDecl->isVolatile()) {
            mOutputFormatHelper.Append(kwVolatileSpace);
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
        if(!isFirstCxxMethodDecl) {
            const auto* parent = methodDecl->getParent();
            outputFormatHelper.Append(parent->getNameAsString());

            /* Handle a templated CXXMethod outside class which is _not_ specialized. */
            if(const auto* ct = parent->getDescribedClassTemplate()) {
                outputFormatHelper.Append("<");

                OutputFormatHelper::ForEachArg(ct->getTemplateParameters()->asArray(),
                                               outputFormatHelper,
                                               [&](const auto* pm) { outputFormatHelper.Append(GetName(*pm)); });

                outputFormatHelper.Append(">");

                /* Handle an explicit specialization of a single CXXMethod outside the class definition. */
            } else if(const auto* clsTmpl = dyn_cast_or_null<ClassTemplateSpecializationDecl>(parent)) {
                CodeGenerator codeGenerator{outputFormatHelper, mLambdaStack};
                codeGenerator.InsertTemplateArgs(*clsTmpl);
            }

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
            mOutputFormatHelper.Append("operator retType (");
            mOutputFormatHelper.Append(outputFormatHelper.GetString());
        } else {
            const auto t = GetDesugarReturnType(decl);
            mOutputFormatHelper.Append(GetTypeNameAsParameter(t, outputFormatHelper.GetString()));
        }
    } else {
        mOutputFormatHelper.Append(outputFormatHelper.GetString());
    }

    mOutputFormatHelper.Append(GetConst(decl));

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
    WrapInParensOrCurlys(BraceKind::Parens, std::move(lambda), addSpaceAtTheEnd);
}
//-----------------------------------------------------------------------------

template<typename T>
void CodeGenerator::WrapInParensIfNeeded(bool needsParens, T&& lambda, const AddSpaceAtTheEnd addSpaceAtTheEnd)
{
    if(needsParens) {
        WrapInParensOrCurlys(BraceKind::Parens, std::move(lambda), addSpaceAtTheEnd);
    } else {
        lambda();
    }
}
//-----------------------------------------------------------------------------

template<typename T>
void CodeGenerator::WrapInCurlys(T&& lambda, const AddSpaceAtTheEnd addSpaceAtTheEnd)
{
    WrapInParensOrCurlys(BraceKind::Curlys, std::move(lambda), addSpaceAtTheEnd);
}
//-----------------------------------------------------------------------------

void StructuredBindingsCodeGenerator::InsertArg(const DeclRefExpr* stmt)
{
    const auto name = GetName(*stmt);
    mOutputFormatHelper.Append(name);

    if(name.empty() || EndsWith(name, "::")) {
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
