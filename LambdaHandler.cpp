/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "LambdaHandler.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "InsightsStaticStrings.h"
#include "InsightsStrCat.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

LambdaHandler::LambdaHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    const auto lambdaMatcher = anyOf(isTemplate,
                                     isExpansionInSystemHeader(),
                                     isMacroOrInvalidLocation(),
                                     hasAncestor(ifStmt()),
                                     hasAncestor(binaryOperator(hasOperatorName(","))),
                                     hasAncestor(cxxForRangeStmt()),
                                     hasAncestor(cxxOperatorCallExpr(hasAncestor(cxxOperatorCallExpr()))),
                                     hasAncestor(compoundStmt(hasAncestor(lambdaExpr()))));

    matcher.addMatcher(lambdaExpr(unless(lambdaMatcher), hasAncestor(varDecl().bind("varDecl"))).bind("lambdaExpr"),
                       this);

    matcher.addMatcher(
        callExpr(
            hasDescendant(lambdaExpr().bind("lambdaExpr")),
            unless(anyOf(lambdaMatcher, hasAncestor(returnStmt()), hasAncestor(varDecl()), hasAncestor(callExpr()))))
            .bind("callExpr"),
        this);

    // directly invoke lambda
    matcher.addMatcher(
        lambdaExpr(unless(anyOf(lambdaMatcher, hasAncestor(varDecl()), hasAncestor(returnStmt())))).bind("lambdaExpr"),
        this);

    // lambda in returnStmt;
    matcher.addMatcher(lambdaExpr(unless(lambdaMatcher), hasAncestor(returnStmt().bind("callExpr"))).bind("lambdaExpr"),
                       this);
}
//-----------------------------------------------------------------------------

void LambdaHandler::run(const MatchFinder::MatchResult& result)
{
    DPrint("---------------------------------\n");
    const auto* vd       = result.Nodes.getNodeAs<VarDecl>("varDecl");
    const auto* callExpr = result.Nodes.getNodeAs<Stmt>("callExpr");
    const auto* lambda   = result.Nodes.getNodeAs<LambdaExpr>("lambdaExpr");

    SKIP_IF_ALREADY_SEEN(lambda);
    DPrint("lambda: %d  %d %d\n", (vd != nullptr), (lambda != nullptr), (nullptr != callExpr));
    Dump(lambda);

    const SourceLocation locStart = [&]() {
        if(vd) {
            return vd->getLocStart();
        } else if(callExpr) {
            return callExpr->getLocStart();
        }

        return lambda->getLocStart();
    }();
    const auto columnNr = GetSM(result).getSpellingColumnNumber(locStart);

    OutputFormatHelper outputFormatHelper{columnNr};

    const auto sr = [&]() {
        CodeGenerator codeGenerator{outputFormatHelper};

        if(vd) {
            codeGenerator.InsertArg(vd);

            return GetSourceRangeAfterToken(vd->getSourceRange(), tok::semi, result);

        } else if(callExpr) {
            codeGenerator.InsertArg(callExpr);

            return callExpr->getSourceRange();

        } else {
            codeGenerator.InsertArg(lambda);

            return lambda->getSourceRange();
        }
    }();

    mRewrite.ReplaceText(sr, outputFormatHelper.GetString());
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
