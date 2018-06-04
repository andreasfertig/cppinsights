/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "RangeForStmtHandler.h"
#include "CodeGenerator.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

RangeForStmtHandler::RangeForStmtHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(cxxForRangeStmt(unless(anyOf(isExpansionInSystemHeader(),
                                                    isTemplate,
                                                    isMacroOrInvalidLocation(),
                                                    hasAncestor(lambdaExpr()),
                                                    hasAncestor(compoundStmt(hasParent(lambdaExpr()))))))
                           .bind("forRange"),
                       this);
}
//-----------------------------------------------------------------------------

void RangeForStmtHandler::run(const MatchFinder::MatchResult& result)
{
    const auto* rangeForStmt = result.Nodes.getNodeAs<CXXForRangeStmt>("forRange");
    const auto& sm           = GetSM(result);
    const auto  columnNr     = sm.getSpellingColumnNumber(rangeForStmt->getLocStart());

    OutputFormatHelper outputFormatHelper{columnNr};
    CodeGenerator      codeGenerator{outputFormatHelper};

    codeGenerator.InsertArg(rangeForStmt);

    mRewrite.ReplaceText(rangeForStmt->getSourceRange(), outputFormatHelper.GetString());
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
