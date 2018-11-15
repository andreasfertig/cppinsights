/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "StaticAssertHandler.h"
#include "CodeGenerator.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

StaticAssertHandler::StaticAssertHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(
        staticAssertDecl(
            unless(anyOf(
                isExpansionInSystemHeader(), isMacroOrInvalidLocation(), isTemplate, hasAncestor(functionDecl()))))
            .bind("static_assert"),
        this);
}
//-----------------------------------------------------------------------------

void StaticAssertHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* matchedDecl = result.Nodes.getNodeAs<StaticAssertDecl>("static_assert")) {
        OutputFormatHelper outputFormatHelper{};
        CodeGenerator      codeGenerator{outputFormatHelper};
        codeGenerator.InsertArg(matchedDecl);

        const auto sr = GetSourceRangeAfterSemi(matchedDecl->getSourceRange(), result);

        mRewrite.ReplaceText(sr, outputFormatHelper.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
