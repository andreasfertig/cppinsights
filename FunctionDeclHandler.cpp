/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "FunctionDeclHandler.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

FunctionDeclHandler::FunctionDeclHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(
        functionDecl(unless(anyOf(cxxMethodDecl(anyOf(
                                      unless(isUserProvided()), isDefaulted(), hasParent(cxxRecordDecl(isLambda())))),
                                  isExpansionInSystemHeader(),
                                  isTemplate,
                                  hasAncestor(friendDecl()),    // friendDecl has functionDecl as child
                                  hasAncestor(functionDecl()),  // prevent forward declarations
                                  isMacroOrInvalidLocation())))
            .bind("funcDecl"),
        this);

    static const auto hasTemplateDescendant = anyOf(hasDescendant(classTemplateDecl()),
                                                    hasDescendant(functionTemplateDecl()),
                                                    hasDescendant(classTemplateSpecializationDecl()));

    matcher.addMatcher(
        friendDecl(unless(anyOf(cxxMethodDecl(anyOf(
                                    unless(isUserProvided()), isDefaulted(), hasParent(cxxRecordDecl(isLambda())))),
                                isExpansionInSystemHeader(),
                                isTemplate,
                                hasTemplateDescendant,
                                hasAncestor(functionDecl()),  // prevent forward declarations
                                isMacroOrInvalidLocation())))
            .bind("friendDecl"),
        this);
}
//-----------------------------------------------------------------------------

void FunctionDeclHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* funcDecl = result.Nodes.getNodeAs<FunctionDecl>("funcDecl")) {
        const auto         columnNr = GetSM(result).getSpellingColumnNumber(funcDecl->getLocStart()) - 1;
        OutputFormatHelper outputFormatHelper{columnNr};
        CodeGenerator      codeGenerator{outputFormatHelper};

        codeGenerator.InsertArg(funcDecl);

        // DPrint("fd rw: %s\n", outputFormatHelper.GetString());
        mRewrite.ReplaceText(funcDecl->getSourceRange(), outputFormatHelper.GetString());

    } else if(const auto* friendDecl = result.Nodes.getNodeAs<FriendDecl>("friendDecl")) {
        if(const auto* fd = friendDecl->getFriendDecl()) {
            if(dyn_cast_or_null<FunctionTemplateDecl>(fd)) {
                // skip template definition
                return;
            }
        }

        const auto         columnNr = GetSM(result).getSpellingColumnNumber(friendDecl->getLocStart()) - 1;
        OutputFormatHelper outputFormatHelper{columnNr};
        CodeGenerator      codeGenerator{outputFormatHelper};

        codeGenerator.InsertArg(friendDecl);

        DPrint("fd rw: %s\n", outputFormatHelper.GetString());
        mRewrite.ReplaceText(friendDecl->getSourceRange(), outputFormatHelper.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
