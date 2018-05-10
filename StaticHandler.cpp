/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "StaticHandler.h"
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

StaticHandler::StaticHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    static const auto localStatic = varDecl(allOf(isStaticLocal(),
                                                  // if the type is trivial destructible we are fine
                                                  unless(hasType(cxxRecordDecl(hasTrivialDestructor()))),
                                                  unless(isExpansionInSystemHeader()),
                                                  unless(isMacroOrInvalidLocation()),
                                                  unless(isTemplate),
                                                  hasDescendant(cxxConstructExpr())))
                                        .bind("varDecl");

    matcher.addMatcher(
        functionDecl(hasDescendant(localStatic), unless(hasDescendant(returnStmt()))).bind("functionDecl"), this);

    matcher.addMatcher(functionDecl(hasDescendant(localStatic), forEachDescendant(returnStmt().bind("returnStmt")))
                           .bind("functionDecl"),
                       this);
}
//-----------------------------------------------------------------------------

void StaticHandler::run(const MatchFinder::MatchResult& result)
{
    const auto* vd       = result.Nodes.getNodeAs<VarDecl>("varDecl");
    auto&       sm       = GetSM(result);
    auto        columnNr = sm.getSpellingColumnNumber(vd->getLocStart()) - 1;

    OutputFormatHelper outputFormatHelper{columnNr};
    CodeGenerator      codeGenerator{outputFormatHelper};
    codeGenerator.InsertArg(vd);

    mRewrite.ReplaceText(vd->getSourceRange(), outputFormatHelper.GetString());

    if(const auto* returnStmt = result.Nodes.getNodeAs<ReturnStmt>("returnStmt")) {
        auto returnStmtColumnNr = sm.getSpellingColumnNumber(returnStmt->getLocStart()) - 1;

        OutputFormatHelper ofm{returnStmtColumnNr};
        CodeGenerator      codeGenerator{ofm};
        codeGenerator.InsertArg(returnStmt);

        mRewrite.ReplaceText(returnStmt->getSourceRange(), ofm.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
