/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "CodeGenerator.h"
#include "InsightsMatchers.h"
#include "InsightsStrCat.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

class ArrayInitCodeGenerator final : public CodeGenerator
{
    const uint64_t mIndex;

public:
    ArrayInitCodeGenerator(OutputFormatHelper& _outputFormatHelper, const uint64_t index)
    : CodeGenerator{_outputFormatHelper}
    , mIndex{index}
    {
    }

    void InsertArg(const Stmt* stmt) override { CodeGenerator::InsertArg(stmt); }
    void InsertArg(const ArrayInitIndexExpr*) override { mOutputFormatHelper.Append(std::to_string(mIndex)); }
};
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXForRangeStmt* rangeForStmt)
{
    mOutputFormatHelper.OpenScope();

    InsertArg(rangeForStmt->getRangeStmt());
    InsertArg(rangeForStmt->getBeginStmt());
    InsertArg(rangeForStmt->getEndStmt());

    // add blank line after the declarations
    mOutputFormatHelper.AppendNewLine();

    mOutputFormatHelper.Append("for( ; ");

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
        mOutputFormatHelper.AppendNewLine(";");
    }

    // close range-for scope in for
    mOutputFormatHelper.CloseScope(OutputFormatHelper::NoNewLineBefore::Yes);

    // close outer range-for scope
    mOutputFormatHelper.CloseScope();
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
        mOutputFormatHelper.Append(" ");
    }

    mOutputFormatHelper.Append("while( ");
    InsertArg(stmt->getCond());
    mOutputFormatHelper.Append(" )");
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

    mOutputFormatHelper.Append("switch(");

    InsertArg(stmt->getCond());

    mOutputFormatHelper.Append(") ");

    InsertArg(stmt->getBody());

    if(hasInit) {
        mOutputFormatHelper.CloseScope();
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const WhileStmt* stmt)
{
    mOutputFormatHelper.Append("while(");
    InsertArg(stmt->getCond());
    mOutputFormatHelper.Append(") ");

    InsertArg(stmt->getBody());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const MemberExpr* stmt)
{
    InsertArg(stmt->getBase());

    const std::string op{stmt->isArrow() ? "->" : "."};
    mOutputFormatHelper.Append(op, stmt->getMemberNameInfo().getName().getAsString());

    const auto* meDecl = stmt->getMemberDecl();

    if(const auto cxxMethod = dyn_cast_or_null<CXXMethodDecl>(meDecl)) {
        InsertTemplateArgs(*cxxMethod->getAsFunction());
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const UnaryExprOrTypeTraitExpr* stmt)
{
    mOutputFormatHelper.Append(GetKind(*stmt));

    if(!stmt->isArgumentType()) {
        InsertArg(stmt->getArgumentExpr());
    } else {
        mOutputFormatHelper.Append("(", GetName(stmt->getTypeOfArgument()), ")");
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
    mOutputFormatHelper.Append("typeid(");
    InsertArg(stmt->getExprOperand());
    mOutputFormatHelper.Append(")");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const BinaryOperator* stmt)
{
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

static std::string GetReferenceOrRValueReferenceOrEmpty(const QualType& type)
{
    if(IsReference(type)) {
        return "&";
    }

    return "";
}
//-----------------------------------------------------------------------------

static std::string GetReferenceOrRValueReferenceOrEmpty(const DeclRefExpr* declRefExpr)
{
    if(const auto* varDecl = GetVarDeclFromDeclRefExpr(declRefExpr)) {
        return GetReferenceOrRValueReferenceOrEmpty(varDecl->getType());
    }

    return "";
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
    }

    for(const auto* child : stmt->children()) {
        if(const auto* childRef = FindDeclRef(child)) {
            return childRef;
        }
    }

    return nullptr;
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const DecompositionDecl* decompositionDeclStmt)
{
    const auto* declName = FindDeclRef(decompositionDeclStmt->getInit());
    const auto  baseVarName{[&]() {
        if(declName) {
            std::string name = GetPlainName(*declName);

            const std::string operatorName{"operator"};
            if(name.find(operatorName) != std::string::npos) {
                return operatorName;
            }

            return name;
        }

        Error(decompositionDeclStmt, "unknown decl\n");
        return std::string{""};
    }()};

    const std::string tmpVarName = [&]() {
        if(declName && declName->getDecl()) {
            return BuildInternalVarName(baseVarName, decompositionDeclStmt->getLocStart(), GetSM(*declName->getDecl()));
        }

        return BuildInternalVarName(baseVarName);
    }();

    mOutputFormatHelper.Append(GetTypeNameAsParameter(decompositionDeclStmt->getType(), tmpVarName), " = ");

    InsertArg(decompositionDeclStmt->getInit());

    mOutputFormatHelper.AppendNewLine(";");

    const bool isRefToObject = IsReference(*decompositionDeclStmt);

    for(const auto* bindingDecl : decompositionDeclStmt->bindings()) {
        const auto* binding = bindingDecl->getBinding();

        if(!binding) {
            Error(bindingDecl, "binding null\n");
            return;
        }

        DPrint("sb name: %s\n", GetName(binding->getType()));

        const auto* bindingDeclRefExpr = dyn_cast_or_null<DeclRefExpr>(binding);

        const std::string refOrRefRef = [&]() {
            if(isa<ArraySubscriptExpr>(binding) && isRefToObject) {
                return std::string{"&"};
            }

            return GetReferenceOrRValueReferenceOrEmpty(bindingDeclRefExpr);
        }();

        mOutputFormatHelper.Append(GetName(bindingDecl->getType()), refOrRefRef, " ", GetName(*bindingDecl), " = ");

        const auto* holdingVarOrMemberExpr = [&]() -> const Expr* {
            if(const auto* holdingVar = bindingDecl->getHoldingVar()) {
                return holdingVar->getAnyInitializer();
            }

            return dyn_cast_or_null<MemberExpr>(binding);
        }();

        // tuple decomposition
        if(holdingVarOrMemberExpr) {
            DPrint("4444\n");

            StructuredBindingsCodeGenerator structuredBindingsCodeGenerator{mOutputFormatHelper, tmpVarName};
            CodeGenerator&                  codeGenerator = structuredBindingsCodeGenerator;
            codeGenerator.InsertArg(holdingVarOrMemberExpr);

            // array decomposition
        } else if(const auto* arraySubscription = dyn_cast_or_null<ArraySubscriptExpr>(binding)) {
            mOutputFormatHelper.Append(tmpVarName);

            InsertArg(arraySubscription);

        } else {
            TODO(bindingDecl, mOutputFormatHelper);
        }

        mOutputFormatHelper.AppendNewLine(";");
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const VarDecl* stmt)
{
    if(const auto* decompDecl = dyn_cast_or_null<DecompositionDecl>(stmt)) {
        InsertArg(decompDecl);
    } else if(IsTrivialStaticClassVarDecl(*stmt)) {
        HandleLocalStaticNonTrivialClass(stmt);

    } else {
        mOutputFormatHelper.Append(GetTypeNameAsParameter(stmt->getType(), GetName(*stmt)));

        if(stmt->hasInit()) {
            mOutputFormatHelper.Append(" = ");

            InsertArg(stmt->getInit());
        };

        if(stmt->isNRVOVariable()) {
            mOutputFormatHelper.Append(" /* NRVO variable */");
        }

        mOutputFormatHelper.AppendNewLine(';');
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const InitListExpr* stmt)
{
    mOutputFormatHelper.Append("{ ");
    mOutputFormatHelper.IncreaseIndent();

    ForEachArg(stmt->inits(), [&](const auto& init) { InsertArg(init); });

    mOutputFormatHelper.Append(" }");
    mOutputFormatHelper.DecreaseIndent();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXConstructExpr* stmt)
{
    mOutputFormatHelper.Append(GetName(GetDesugarType(stmt->getType()), Unqualified::Yes), "(");

    if(stmt->getNumArgs()) {
        ForEachArg(stmt->arguments(), [&](const auto& arg) { InsertArg(arg); });
    }

    mOutputFormatHelper.Append(')');
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXMemberCallExpr* stmt)
{
    InsertArg(stmt->getCallee());
    mOutputFormatHelper.Append('(');

    ForEachArg(stmt->arguments(), [&](const auto& arg) { InsertArg(arg); });

    mOutputFormatHelper.Append(')');
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ParenExpr* stmt)
{
    mOutputFormatHelper.Append('(');

    InsertArg(stmt->getSubExpr());

    mOutputFormatHelper.Append(')');
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const UnaryOperator* stmt)
{
    const char* opCodeName = GetOpcodeName(stmt->getOpcode());
    const bool  insertBefore{!stmt->isPostfix()};

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

    mOutputFormatHelper.Append("[");
    InsertArg(stmt->getRHS());
    mOutputFormatHelper.Append("]");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ArrayInitLoopExpr* stmt)
{
    mOutputFormatHelper.Append("{ ");

    const uint64_t size = stmt->getArraySize().getZExtValue();
    bool           first{true};

    for(uint64_t i = 0; i < size; ++i) {
        if(!first) {
            mOutputFormatHelper.Append(", ");
        } else {
            first = false;
        }

        ArrayInitCodeGenerator codeGenerator{mOutputFormatHelper, i};
        codeGenerator.InsertArg(stmt->getSubExpr());
    }

    mOutputFormatHelper.Append(" }");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const OpaqueValueExpr* stmt)
{
    InsertArg(stmt->getSourceExpr());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CallExpr* stmt)
{
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
                        char C{static_cast<char>(arg.getAsIntegral().getZExtValue())};
                        mOutputFormatHelper.Append("'", std::string{C}, "'");
                    });

                    mOutputFormatHelper.Append('>');
                }
            }
        }
    }

    mOutputFormatHelper.Append('(');

    ForEachArg(stmt->arguments(), [&](const auto& arg) { InsertArg(arg); });

    mOutputFormatHelper.Append(')');
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXNamedCastExpr* stmt)
{
    const QualType castDestType = stmt->getType().getCanonicalType();
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
        return;
    }

    if(isa<IntegerLiteral>(subExpr)) {
        InsertArg(stmt->IgnoreCasts());

    } else {
        const bool               isReinterpretCast{castKind == CastKind::CK_BitCast};
        static const std::string castName{isReinterpretCast ? "reinterpret_cast" : "static_cast"};
        const QualType           castDestType{stmt->getType().getCanonicalType()};
        const AsComment asComment = (!isReinterpretCast && isa<CXXThisExpr>(subExpr)) ? AsComment::Yes : AsComment::No;

        FormatCast(castName, castDestType, subExpr, castKind, asComment);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const DeclRefExpr* stmt)
{
    mOutputFormatHelper.Append(GetName(*stmt));
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

void CodeGenerator::HandleCompoundStmt(const CompoundStmt* stmt)
{
    for(const auto* item : stmt->body()) {
        InsertArg(item);

        if(!isa<IfStmt>(item) && !isa<ForStmt>(item) && !isa<DeclStmt>(item)) {
            mOutputFormatHelper.AppendNewLine(";");
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

    mOutputFormatHelper.Append("if", cexpr, "( ");

    InsertArg(stmt->getCond());

    mOutputFormatHelper.Append(" ) ");

    const auto* body = stmt->getThen();

    InsertArg(body);

    const bool isBodyBraced = isa<CompoundStmt>(body);

    if(!isBodyBraced && !isa<NullStmt>(body)) {
        mOutputFormatHelper.AppendNewLine(';');
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

        if(needScope) {
            mOutputFormatHelper.CloseScope();
        }
    }

    mOutputFormatHelper.AppendNewLine();

    if(hasInit) {
        mOutputFormatHelper.CloseScope();
        mOutputFormatHelper.AppendNewLine();
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ForStmt* stmt)
{
    mOutputFormatHelper.Append("for(");

    if(const auto* init = stmt->getInit()) {
        // the init-stmt carries a ; at the end
        InsertArg(init);
    } else {
        mOutputFormatHelper.Append("; ");
    }

    InsertArg(stmt->getCond());
    mOutputFormatHelper.Append("; ");

    InsertArg(stmt->getInc());

    mOutputFormatHelper.AppendNewLine(')');

    InsertArg(stmt->getBody());
    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CStyleCastExpr* stmt)
{
    static const std::string castName{"reinterpret_cast"};
    const QualType           castDestType = stmt->getType().getCanonicalType();

    FormatCast(castName, castDestType, stmt->getSubExpr(), stmt->getCastKind(), AsComment::No);
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXNewExpr* stmt)
{
    mOutputFormatHelper.Append("new ");

    if(stmt->getNumPlacementArgs()) {
        /* we have a placement new */

        mOutputFormatHelper.Append('(');

        ForEachArg(stmt->placement_arguments(), [&](const auto& placementArg) { InsertArg(placementArg); });

        mOutputFormatHelper.Append(')');
    }

    Dump(stmt);
    Dump(stmt->getOperatorNew());

    if(const auto* ctorExpr = stmt->getConstructExpr()) {
        InsertArg(ctorExpr);

    } else if(stmt->isArray()) {
        mOutputFormatHelper.Append(GetName(stmt->getAllocatedType()), "[");

        InsertArg(stmt->getArraySize());
        mOutputFormatHelper.Append(']');

        if(stmt->hasInitializer()) {
            InsertArg(stmt->getInitializer());
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
    DPrint("args: %d\n", stmt->getNumArgs());

    Dump(stmt);

    const auto* callee      = dyn_cast_or_null<DeclRefExpr>(stmt->getCallee()->IgnoreImpCasts());
    const bool  isCXXMethod = [&]() { return (callee && isa<CXXMethodDecl>(callee->getDecl())); }();

    if(2 == stmt->getNumArgs()) {
        const auto* param1 = dyn_cast_or_null<DeclRefExpr>(stmt->getArg(0)->IgnoreImpCasts());
        const auto* param2 = dyn_cast_or_null<DeclRefExpr>(stmt->getArg(1)->IgnoreImpCasts());

        if(callee && param1 && param2) {

            const std::string replace = [&]() {
                if(isa<CXXMethodDecl>(callee->getDecl())) {
                    return StrCat(GetName(*param1), ".", GetName(*callee), "(", GetName(*param2), ")");
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
    const auto* arg0 = cb->IgnoreImplicit();
    ++cb;

    const auto* arg1 = *cb;
    ++cb;

    if(const auto* dd = dyn_cast_or_null<DeclRefExpr>(arg0)) {
        const auto* decl = dd->getDecl();
        // at least std::cout boils down to a FunctionDecl at this point
        if(!isa<CXXMethodDecl>(decl) && !isa<FunctionDecl>(decl)) {
            // we have a global function not a member function operator. Skip this.
            return;
        }
    }

    // operators in a namespace but outside a class so operator goes first
    if(!isCXXMethod) {
        mOutputFormatHelper.Append(GetName(*callee), "(");
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
    mOutputFormatHelper.Append(GetLambdaName(*stmt));
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
    const bool isListInitialization{[&]() { return stmt->getLParenLoc().isInvalid(); }()};
    const bool needsParens{!isConstructor && !isListInitialization};

    // If a constructor follows we do not need to insert the type name. This would insert it twice.
    if(!isConstructor) {
        mOutputFormatHelper.Append(GetName(stmt->getTypeAsWritten()));
    }

    if(needsParens) {
        mOutputFormatHelper.Append('(');
    }

    InsertArg(stmt->getSubExpr());

    if(needsParens) {
        mOutputFormatHelper.Append(')');
    }
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
    HandleCharacterLiteral(*stmt);
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const PredefinedExpr* stmt)
{
    InsertArg(stmt->getFunctionName());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ExprWithCleanups* stmt)
{
    InsertArg(stmt->getSubExpr());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const TypeAliasDecl* stmt)
{
    mOutputFormatHelper.AppendNewLine("using ", GetName(*stmt), " = ", GetName(stmt->getUnderlyingType()), ";");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const TypedefDecl* stmt)
{
    /* function pointer typedefs are special. Ease up things using "using" */
    //    outputFormatHelper.AppendNewLine("typedef ", GetName(stmt->getUnderlyingType()), " ", GetName(*stmt), ";");
    mOutputFormatHelper.AppendNewLine("using ", GetName(*stmt), " = ", GetName(stmt->getUnderlyingType()), ";");
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

void CodeGenerator::InsertArg(const ReturnStmt* stmt)
{
    mOutputFormatHelper.Append("return");

    if(const auto* retVal = stmt->getRetValue()) {
        mOutputFormatHelper.Append(' ');
        InsertArg(retVal);
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const NullStmt* /*stmt*/)
{
    mOutputFormatHelper.AppendNewLine(';');
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXDefaultArgExpr* stmt)
{
    InsertArg(stmt->getExpr());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXStdInitializerListExpr* stmt)
{
    // No qualifiers like const or volatile here. This appears in  function calls or operators as a parameter. CV's are
    // not allowed there.
    mOutputFormatHelper.Append(GetName(stmt->getType(), Unqualified::Yes));
    InsertArg(stmt->getSubExpr());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const ExplicitCastExpr* stmt)
{
    InsertArg(stmt->getSubExpr());
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const CXXNullPtrLiteralExpr* /*stmt*/)
{
    mOutputFormatHelper.Append("nullptr");
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArg(const Decl* stmt)
{
#define SUPPORTED_DECL(type)                                                                                           \
    if(isa<type>(stmt)) {                                                                                              \
        InsertArg(static_cast<const type*>(stmt));                                                                     \
        return;                                                                                                        \
    }

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

#include "CodeGeneratorTypes.h"

    // PackExpansionExpr

    TODO(stmt, mOutputFormatHelper);
}
//-----------------------------------------------------------------------------

void CodeGenerator::HandleCharacterLiteral(const CharacterLiteral& stmt)
{
    switch(stmt.getKind()) {
        case CharacterLiteral::Ascii: break;
        case CharacterLiteral::Wide: mOutputFormatHelper.Append('L'); break;
        case CharacterLiteral::UTF8: mOutputFormatHelper.Append("u8"); break;
        case CharacterLiteral::UTF16: mOutputFormatHelper.Append('u'); break;
        case CharacterLiteral::UTF32: mOutputFormatHelper.Append('U'); break;
    }

    switch(unsigned value = stmt.getValue()) {
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
            if((value & ~0xFFu) == ~0xFFu && stmt.getKind() == CharacterLiteral::Ascii) {
                value &= 0xFFu;
            }

            if(value < 256 && isPrintable(static_cast<unsigned char>(value))) {
                const std::string v{static_cast<char>(value)};
                mOutputFormatHelper.Append("'", v, "'");
            }
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::FormatCast(const std::string castName,
                               const QualType&   castDestType,
                               const Expr*       subExpr,
                               const CastKind&   castKind,
                               const AsComment   comment)
{
    const bool        isCastToBase{((castKind == CK_DerivedToBase) || (castKind == CK_UncheckedDerivedToBase)) &&
                            castDestType->isRecordType()};
    const std::string castDestTypeText{
        StrCat(GetName(castDestType), ((isCastToBase && !castDestType->isAnyPointerType()) ? "&" : ""))};

    if(AsComment::Yes == comment) {
        mOutputFormatHelper.Append("/*");
    }
    mOutputFormatHelper.Append(StrCat(castName, "<", castDestTypeText, ">("));
    InsertArg(subExpr);
    mOutputFormatHelper.Append(')');
    if(AsComment::Yes == comment) {
        mOutputFormatHelper.Append("*/");
    }
}
//-----------------------------------------------------------------------------

void CodeGenerator::InsertArgWithParensIfNeeded(const Stmt* stmt)
{
    const bool needParens = [&]() {
        if(const auto* dest = dyn_cast_or_null<UnaryOperator>(stmt->IgnoreImplicit())) {
            if(dest->getOpcode() == clang::UO_Deref) {
                return true;
            }
        }

        return false;
    }();

    if(needParens) {
        mOutputFormatHelper.Append('(');
    }

    InsertArg(stmt);

    if(needParens) {
        mOutputFormatHelper.Append(')');
    }
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
        case TemplateArgument::Declaration: InsertArg(arg.getAsDecl()); break;
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
        default: break;
    };

    return "unknown";
}
//-----------------------------------------------------------------------------

const char* CodeGenerator::GetOpcodeName(const int kind)
{
    switch(kind) {
        default: return "???";

#define UNARY_OPERATION(Name, Spelling)                                                                                \
    case UO_##Name:                                                                                                    \
        return Spelling;

#include "clang/AST/OperationKinds.def"

#undef UNARY_OPERATION
    }
    // llvm_unreachable("Not an overloaded allocation operator");
}
//-----------------------------------------------------------------------------

const char* CodeGenerator::GetBuiltinTypeSuffix(const BuiltinType& type)
{
#define CASE(K, retVal)                                                                                                \
    case BuiltinType::K: return retVal
    switch(type.getKind()) {
        CASE(Bool, "");
        CASE(Char_U, "");
        CASE(UChar, "");
        CASE(Char16, "");
        CASE(Char32, "");
        CASE(UShort, "");
        CASE(UInt, "u");
        CASE(ULong, "ul");
        CASE(ULongLong, "ull");
        CASE(UInt128, "ulll");
        CASE(Char_S, "");
        CASE(SChar, "");
        CASE(Short, "");
        CASE(Int, "");
        CASE(Long, "l");
        CASE(LongLong, "ll");
        CASE(Int128, "");
        CASE(Float, "f");
        CASE(Double, "");
        CASE(LongDouble, "L");
        CASE(WChar_S, "");
        CASE(WChar_U, "");
        default: return "";
    }
#undef BTCASE
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

    mOutputFormatHelper.Append("__this");
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
