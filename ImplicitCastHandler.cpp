/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "ImplicitCastHandler.h"
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

ImplicitCastHandler::ImplicitCastHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    static const auto implicitCastMatch = anyOf(
        isExpansionInSystemHeader(),
        isMacroOrInvalidLocation(),
        isTemplate,
        /* Exclude code in range-based for compiler generated part */
        hasAncestor(cxxForRangeStmt()),
        hasAncestor(userDefinedLiteral()),
        hasLambdaAncestor,
        /* implicit methods can have a body. See ClassOpInTemplateTest.cpp, the assignment operator is
           implicitly generated with a body. */
        hasAncestor(cxxMethodDecl(isImplicit())),
#ifdef MATCH_CXX_MEM_CEXPR
        hasAncestor(cxxMemberCallExpr()),
#endif
        hasAncestor(implicitCastExpr(hasMatchingCast())), /* will be catch by the walk down */
        /* we do not like to introduce casts in parameter decls. This happens when
           we have a template parameter with a base class. CharLiteralTest.cpp */
        hasAncestor(parmVarDecl()),
        /*                                          hasDescendant(declRefExpr(hasDeclaration(classTemplateSpecializationDecl(
                                                      hasAnyTemplateArgument(templateArgument()))))),*/
        /* FIXME: for some reason the above does not match. This is not as good
           but seems to do the job */
        hasParent(cxxConstructExpr()),
        /* skip a case where a bool cast is done with an decl if: if( auto X =
           something ). See: IfSwitchInitHandler5Test.cpp */
        // hasAncestor(ifStmt(hasDescendant(declStmt()))),
        hasAncestor(unaryOperator(hasDescendant(cxxOperatorCallExpr()))),
        /* exclude if/switch init, that will be handeled by them */
        hasAncestor(ifStmt()),
        hasAncestor(switchStmt()),
        hasAncestor(cxxOperatorCallExpr()));

    matcher.addMatcher(
        implicitCastExpr(unless(anyOf(implicitCastMatch, hasDescendant(cxxThisExpr()))), hasMatchingCast())
            .bind("implicitCast"),
        this);

    matcher.addMatcher(implicitCastExpr(unless(implicitCastMatch),
                                        allOf(hasMatchingCast(), hasDescendant(cxxThisExpr().bind("memberExpr"))))
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

        if(result.Nodes.getNodeAs<CXXThisExpr>("memberExpr")) {
            mRewrite.InsertText(implCastExpr->getLocStart(), outputFormatHelper.GetString());
        } else {
            mRewrite.ReplaceText(implCastExpr->getSourceRange(), outputFormatHelper.GetString());
        }
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
