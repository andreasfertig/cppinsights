/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "CommaOperatorHandler.h"
#include "CodeGenerator.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "InsightsStaticStrings.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

CommaOperatorHandler::CommaOperatorHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(binaryOperator(hasOperatorName(","),
                                      unless(anyOf(isExpansionInSystemHeader(),
                                                   isMacroOrInvalidLocation(),
                                                   hasAncestor(cxxForRangeStmt()),
                                                   hasAncestor(lambdaExpr()),
                                                   hasAncestor(switchStmt()),
                                                   isTemplate)))
                           .bind("commaOperator"),
                       this);
}
//-----------------------------------------------------------------------------

void CommaOperatorHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* commaOperator = result.Nodes.getNodeAs<BinaryOperator>("commaOperator")) {
        const auto         columnNr = GetSM(result).getSpellingColumnNumber(commaOperator->getLocStart());
        OutputFormatHelper outputFormatHelper{columnNr};
        CodeGenerator      codeGenerator{outputFormatHelper};

        codeGenerator.InsertArg(commaOperator);

        mRewrite.ReplaceText(commaOperator->getSourceRange(), outputFormatHelper.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
