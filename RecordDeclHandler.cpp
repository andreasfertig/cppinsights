/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "RecordDeclHandler.h"
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

RecordDeclHandler::RecordDeclHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(cxxRecordDecl(hasDefinition(),
                                     unless(anyOf(isLambda(),
                                                  hasAncestor(namespaceDecl()),
                                                  hasAncestor(functionDecl()),
                                                  hasAncestor(cxxRecordDecl()),
                                                  isTemplate,
                                                  isExpansionInSystemHeader(),
                                                  isMacroOrInvalidLocation())))
                           .bind("cxxRecordDecl"),
                       this);

    matcher.addMatcher(namespaceDecl(hasParent(translationUnitDecl()),
                                     unless(anyOf(isExpansionInSystemHeader(), isMacroOrInvalidLocation())))
                           .bind("namespaceDecl"),
                       this);
}
//-----------------------------------------------------------------------------

void RecordDeclHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* cxxRecordDecl = result.Nodes.getNodeAs<CXXRecordDecl>("cxxRecordDecl")) {
        OutputFormatHelper outputFormatHelper{};

        CodeGenerator codeGenerator{outputFormatHelper};
        codeGenerator.InsertArg(cxxRecordDecl);

        mRewrite.ReplaceText(GetSourceRangeAfterSemi(cxxRecordDecl->getSourceRange(), result),
                             outputFormatHelper.GetString());
    } else if(const auto* namespaceDecl = result.Nodes.getNodeAs<NamespaceDecl>("namespaceDecl")) {
        OutputFormatHelper outputFormatHelper{};

        CodeGenerator codeGenerator{outputFormatHelper};
        codeGenerator.InsertArg(namespaceDecl);

        mRewrite.ReplaceText(GetSourceRangeAfterSemi(namespaceDecl->getSourceRange(), result, RequireSemi::No),
                             outputFormatHelper.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
