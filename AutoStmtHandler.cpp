/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "AutoStmtHandler.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "InsightsStaticStrings.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

AutoStmtHandler::AutoStmtHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(varDecl(unless(anyOf(isExpansionInSystemHeader(),
                                            isMacroOrInvalidLocation(),
                                            /* don't substitute lambdas */
                                            /* exclude rangeStateFor and if with init for now*/
                                            hasAncestor(cxxForRangeStmt()),
                                            decompositionDecl(),
                                            hasAncestor(lambdaExpr()),
                                            /* don't replace auto in templates */
                                            isTemplate,
                                            hasAncestor(switchStmt()),
                                            hasAncestor(ifStmt()))),
                               anyOf(/* auto */
                                     hasType(autoType().bind("autoType")),
                                     hasType(qualType(hasDescendant(autoType().bind("autoType")))),
                                     /* decltype and decltype(auto) */
                                     hasType(decltypeType().bind("dt")),
                                     hasType(qualType(hasDescendant(decltypeType().bind("dt"))))))
                           .bind("autoDecl"),
                       this);

    matcher.addMatcher(functionDecl(allOf(hasTrailingReturn(),
                                          unless(anyOf(isExpansionInSystemHeader(),
                                                       /* don't replace auto in templates */
                                                       isTemplate,
                                                       returns(decltypeType().bind("dt")),
                                                       hasParent(cxxRecordDecl(isLambda()))))))
                           .bind("funcDecl"),
                       this);

    matcher.addMatcher(
        functionDecl(
            allOf(returns(autoType()),
                  unless(anyOf(isExpansionInSystemHeader(), isTemplate, hasParent(cxxRecordDecl(isLambda()))))))
            .bind("funcDecl"),
        this);
}
//-----------------------------------------------------------------------------

void AutoStmtHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* funcDecl = result.Nodes.getNodeAs<FunctionDecl>("funcDecl")) {
        const bool hasTrailingReturn = [&]() {
            if(const auto* functionProtoType = funcDecl->getType()->getAs<FunctionProtoType>()) {
                return functionProtoType->hasTrailingReturn();
            }

            return false;
        }();

        // XXX hack to remove trailing return type -> xxx
        if(hasTrailingReturn) {

            // DPrint("trialing\n");
            if(const Stmt* body = funcDecl->getBody()) {
                OutputFormatHelper outputFormatHelper{};
                GenerateFunctionPrototype(outputFormatHelper, *funcDecl);
                // DPrint("func: %s\n", outputFormatHelper.GetString());

                const SourceLocation bodyLoc{body->getLocStart()};
                const SourceRange    trailingReturnRange{funcDecl->getSourceRange().getBegin(),
                                                      bodyLoc.getLocWithOffset(-1)};

                mRewrite.ReplaceText(trailingReturnRange, outputFormatHelper.GetString());
                return;
            }
        }

        // decltype(auto) Function(params) { }
        // ^              ^                  ^
        // 1              2                  3
        // the getLocStart starts at (1) end ends at (3). The getLocation in the other hand starts at (2) and
        // ends at (3)
        const SourceRange sr{funcDecl->getLocStart(), funcDecl->getLocation().getLocWithOffset(-1)};

        const std::string fqn{StrCat((funcDecl->isConstexpr() ? kwConstExprSpace : ""),
                                     (funcDecl->isInlined() ? kwInlineSpace : ""),
                                     GetName(GetDesugarReturnType(*funcDecl)))};

        mRewrite.ReplaceText(sr /*FuncDecl->getReturnTypeSourceRange()*/, fqn);

        return;
    } else if(const auto* autoDecl = result.Nodes.getNodeAs<VarDecl>("autoDecl")) {
        const QualType type = [&]() {
            if(const auto* declType = result.Nodes.getNodeAs<DecltypeType>("dt")) {
                return declType->getUnderlyingType();
            }

            return autoDecl->getType();
        }();

        const std::string fqn{[&]() {
            if(autoDecl->getType()->isFunctionPointerType()) {
                const auto lineNo = result.SourceManager->getSpellingLineNumber(autoDecl->getSourceRange().getBegin());

                const std::string funcPtrName{StrCat("FuncPtr_", std::to_string(lineNo))};
                std::string       usingStr{StrCat("using ", funcPtrName, " = ", GetName(type), ";\n ", funcPtrName)};

                return StrCat((autoDecl->isConstexpr() ? kwConstExprSpace : ""), usingStr);

            } else {
                return StrCat((autoDecl->isConstexpr() ? kwConstExprSpace : ""), GetName(type));
            }
        }()};

        // constexpr int* x = 5;
        // ^              ^   ^
        // 1              2   3
        // the SourceRange starts at (1) end ends at (3). The Location in the other hand starts at (1) and ends
        // at (2)
        const SourceRange sr{autoDecl->getSourceRange().getBegin(), autoDecl->getLocation().getLocWithOffset(-1)};

        mRewrite.ReplaceText(sr, fqn);
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
