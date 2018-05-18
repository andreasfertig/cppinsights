/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "StructuredBindingsHandler.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "InsightsMatchers.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

StructuredBindingsHandler::StructuredBindingsHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(decompositionDecl(unless(anyOf(isExpansionInSystemHeader(),
                                                      isTemplate,
                                                      isMacroOrInvalidLocation(),
                                                      hasAncestor(lambdaExpr()),
                                                      hasAncestor(cxxForRangeStmt()))),
                                         has(expr(hasDescendant(declRefExpr().bind("dref"))).bind("arinit")))
                           .bind("decl"),
                       this);

    matcher.addMatcher(
        decompositionDecl(
            unless(anyOf(
                isExpansionInSystemHeader(), isTemplate, hasAncestor(lambdaExpr()), hasAncestor(cxxForRangeStmt()))),
            has(declRefExpr().bind("dref")))
            .bind("decl"),
        this);
}
//-----------------------------------------------------------------------------

void StructuredBindingsHandler::run(const MatchFinder::MatchResult& result)
{
    const auto*        decompositionDeclStmt = result.Nodes.getNodeAs<DecompositionDecl>("decl");
    const auto&        sm                    = GetSM(result);
    const auto         columnNr              = sm.getSpellingColumnNumber(decompositionDeclStmt->getLocStart()) - 1;
    OutputFormatHelper outputFormatHelper{columnNr};

    CodeGenerator codeGenerator{outputFormatHelper};
    codeGenerator.InsertArg(decompositionDeclStmt);

    const SourceRange sourceRange =
        GetSourceRangeAfterToken(decompositionDeclStmt->getSourceRange(), tok::semi, result);

    mRewrite.ReplaceText(sourceRange, outputFormatHelper.GetString());
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
