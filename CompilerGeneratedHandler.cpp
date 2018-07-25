/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "CompilerGeneratedHandler.h"
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
    static const auto compilerProvided = allOf(unless(anyOf(isUserProvided(),
                                                            isDeleted(),
                                                            isExpansionInSystemHeader(),
                                                            isTemplate,
                                                            hasAncestor(functionDecl()),
                                                            isMacroOrInvalidLocation())),
                                               hasParent(cxxRecordDecl(unless(isLambda())).bind("record")));

    matcher.addMatcher(cxxMethodDecl(compilerProvided).bind("method"), this);
}
//-----------------------------------------------------------------------------

void CompilerGeneratedHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* methodDecl = result.Nodes.getNodeAs<CXXMethodDecl>("method")) {
        OutputFormatHelper outputFormatHelper{};
        outputFormatHelper.Append("/* ");

        CodeGenerator codeGenerator{outputFormatHelper};
        codeGenerator.InsertAccessModifierAndNameWithReturnType(*methodDecl);

        outputFormatHelper.AppendNewLine("; */");

        // add all compiler generated methods at the end of the class
        const auto* recrodDecl = result.Nodes.getNodeAs<CXXRecordDecl>("record");
        const auto  loc        = recrodDecl->getLocEnd();

        mRewrite.InsertText(loc, outputFormatHelper.GetString(), true, true);
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
