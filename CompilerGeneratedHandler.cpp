/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "CompilerGeneratedHandler.h"
#include "ClangCompat.h"
#include "CodeGenerator.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

CompilerGeneratedHandler::CompilerGeneratedHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(cxxRecordDecl(hasDefinition(),
                                     unless(anyOf(isLambda(),
                                                  hasAncestor(functionDecl()),
                                                  hasAncestor(cxxRecordDecl()),
                                                  isTemplate,
                                                  isExpansionInSystemHeader(),
                                                  isMacroOrInvalidLocation())))
                           .bind("cxxRecordDecl"),
                       this);
}
//-----------------------------------------------------------------------------

void CompilerGeneratedHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* cxxRecordDecl = result.Nodes.getNodeAs<CXXRecordDecl>("cxxRecordDecl")) {
        OutputFormatHelper outputFormatHelper{};

        CodeGenerator codeGenerator{outputFormatHelper};
        codeGenerator.InsertArg(cxxRecordDecl);

        mRewrite.ReplaceText(GetSourceRangeAfterSemi(cxxRecordDecl->getSourceRange(), result),
                             outputFormatHelper.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
