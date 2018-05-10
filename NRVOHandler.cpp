/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "NRVOHandler.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "InsightsStaticStrings.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

NRVOHandler::NRVOHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(varDecl(allOf(unless(isExpansionInSystemHeader()),
                                     unless(isMacroOrInvalidLocation()),
                                     unless(isTemplate),
                                     hasDescendant(cxxConstructExpr()),
                                     isNRVOVariable()))
                           .bind("varDecl"),
                       this);
}
//-----------------------------------------------------------------------------

void NRVOHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* vd = result.Nodes.getNodeAs<VarDecl>("varDecl")) {
        const auto        loc    = vd->getLocEnd();
        const auto        offset = Lexer::MeasureTokenLength(loc, mRewrite.getSourceMgr(), mRewrite.getLangOpts());
        const auto        end    = loc.getLocWithOffset(offset);
        const std::string comment{" /* NRVO variable */"};

        mRewrite.InsertText(end, comment, true, true);
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
