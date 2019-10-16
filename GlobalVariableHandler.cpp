/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "GlobalVariableHandler.h"
#include "CodeGenerator.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::ast_matchers {
const internal::VariadicDynCastAllOfMatcher<Decl, VarTemplateSpecializationDecl> varTemplateSpecDecl;  // NOLINT
const internal::VariadicDynCastAllOfMatcher<Decl, DecompositionDecl>             decompositionDecl;    // NOLINT
// XXX: recent clang source has a declType matcher. Try to figure out a migration path.
const internal::VariadicDynCastAllOfMatcher<Type, DecltypeType> myDecltypeType;  // NOLINT
}  // namespace clang::ast_matchers

namespace clang::insights {

GlobalVariableHandler::GlobalVariableHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(
        varDecl(unless(anyOf(
                    isExpansionInSystemHeader(),
                    isInvalidLocation(),
                    hasAncestor(varTemplateDecl()),
                    hasAncestor(functionDecl()),
                    hasAncestor(cxxRecordDecl()),
                    hasAncestor(namespaceDecl()),
                    hasAncestor(typeAliasDecl()),
                    hasAncestor(cxxMethodDecl()),
                    parmVarDecl(),
                    // don't match a VarDecl within a VarDecl. Happens for example in lambdas.
                    hasAncestor(varDecl()),
                    varTemplateSpecDecl(),
                    // A DecompositionDecl in global scope is different in the AST than one in a function for example.
                    // Try to find out whether this VarDecl is the result of a DecompositionDecl, if so bail out.
                    hasInitializer(ignoringImpCasts(
                        callExpr(hasAnyArgument(ignoringParenImpCasts(declRefExpr(to(decompositionDecl()))))))),
                    // don't replace anything in templates
                    isTemplate)))
            .bind("varDecl"),
        this);
}
//-----------------------------------------------------------------------------

void GlobalVariableHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* matchedDecl = result.Nodes.getNodeAs<VarDecl>("varDecl")) {
        OutputFormatHelper outputFormatHelper{};
        CodeGenerator      codeGenerator{outputFormatHelper};
        codeGenerator.InsertArg(matchedDecl);

        const auto sr = GetSourceRangeAfterSemi(matchedDecl->getSourceRange(), result, RequireSemi::Yes);

        mRewrite.ReplaceText(sr, outputFormatHelper.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
