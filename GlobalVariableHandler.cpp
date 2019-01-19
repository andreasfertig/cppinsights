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
const internal::VariadicDynCastAllOfMatcher<Decl, VarTemplateSpecializationDecl> varTemplateSpecDecl;
}

namespace clang::insights {

GlobalVariableHandler::GlobalVariableHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(varDecl(hasParent(translationUnitDecl()),
                               unless(anyOf(isExpansionInSystemHeader(),
                                            isMacroOrInvalidLocation(),
                                            hasDescendant(cxxRecordDecl(isLambda())),
                                            varTemplateSpecDecl(),
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

        const auto sr = GetSourceRangeAfterToken(matchedDecl->getSourceRange(), tok::semi, result);

        mRewrite.ReplaceText(sr, outputFormatHelper.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
