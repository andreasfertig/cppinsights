/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "IfStmtHandler.h"
#include "CodeGenerator.h"
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

IfStmtHandler::IfStmtHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    static const auto unlessMatching = anyOf(isExpansionInSystemHeader(),
                                             isMacroOrInvalidLocation(),
                                             isTemplate,
                                             hasAncestor(cxxMethodDecl(hasAnyTemplateArgument(templateArgument()))),
                                             hasAncestor(lambdaExpr()),
                                             hasAncestor(cxxOperatorCallExpr()),
                                             hasAncestor(cxxForRangeStmt()),
                                             hasAncestor(ifStmt()),
                                             hasAncestor(implicitCastExpr(hasMatchingCast())),
                                             hasAncestor(userDefinedLiteral()));

    matcher.addMatcher(ifStmt(unless(unlessMatching)).bind("stmt"), this);

    matcher.addMatcher(switchStmt(unless(unlessMatching)).bind("stmt"), this);
}
//-----------------------------------------------------------------------------

void IfStmtHandler::run(const MatchFinder::MatchResult& result)
{
    const auto* stmt     = result.Nodes.getNodeAs<Stmt>("stmt");
    const auto& sm       = GetSM(result);
    const auto  columnNr = sm.getSpellingColumnNumber(stmt->getLocStart());

    OutputFormatHelper outputFormatHelper{columnNr};
    CodeGenerator      codeGenerator{outputFormatHelper};
    codeGenerator.InsertArg(stmt);

    mRewrite.ReplaceText(stmt->getSourceRange(), outputFormatHelper.GetString());
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
