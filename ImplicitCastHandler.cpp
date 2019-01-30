/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "ImplicitCastHandler.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "InsightsMatchers.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

ImplicitCastHandler::ImplicitCastHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    static const auto implicitCastMatch =
        anyOf(isExpansionInSystemHeader(),
              isMacroOrInvalidLocation(),
              isTemplate,
              isAutoAncestor,
              hasAncestor(functionDecl()),
              hasAncestor(userDefinedLiteral()),
              hasAncestor(implicitCastExpr(hasMatchingCast())), /* will be catch by the walk down */
              /* we do not like to introduce casts in parameter decls. This happens when
                 we have a template parameter with a base class. CharLiteralTest.cpp */
              hasAncestor(parmVarDecl()));

    matcher.addMatcher(
        implicitCastExpr(unless(anyOf(implicitCastMatch, hasAncestor(varDecl(hasParent(translationUnitDecl()))))),
                         hasMatchingCast())
            .bind("implicitCast"),
        this);
}
//-----------------------------------------------------------------------------

void ImplicitCastHandler::run(const MatchFinder::MatchResult& result)
{
    const auto* implCastExpr = result.Nodes.getNodeAs<ImplicitCastExpr>("implicitCast");

    // we get the same ImplictCast matched twice. Unknown why. This give a little possible background:
    // https://lists.llvm.org/pipermail/cfe-dev/2017-June/054205.html
    // FIXME for now keep track of what we already have seen and bail out of something appears twice
    SKIP_IF_ALREADY_SEEN(implCastExpr);

    OutputFormatHelper outputFormatHelper{};
    CodeGenerator      codeGenerator{outputFormatHelper};

    codeGenerator.InsertArg(implCastExpr);

    if(!outputFormatHelper.GetString().empty()) {
        DPrint("repllacement: %s\n", outputFormatHelper.GetString());
        mRewrite.ReplaceText(implCastExpr->getSourceRange(), outputFormatHelper.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
