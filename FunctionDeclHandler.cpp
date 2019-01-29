/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "FunctionDeclHandler.h"
#include "ClangCompat.h"
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

    // Match the using Base::Base statement and remove it.
    matcher.addMatcher(usingDecl(hasParent(cxxRecordDecl())).bind("using"), this);
}
//-----------------------------------------------------------------------------

void FunctionDeclHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* funcDecl = result.Nodes.getNodeAs<FunctionDecl>("funcDecl")) {
        const auto         columnNr = GetSM(result).getSpellingColumnNumber(GetBeginLoc(funcDecl)) - 1;
        OutputFormatHelper outputFormatHelper{columnNr};
        CodeGenerator      codeGenerator{outputFormatHelper};

        codeGenerator.InsertArg(funcDecl);

        // Find the correct ending of the source range. In case of a declaration we need to find the ending semi,
        // otherwise the provided source range is correct.
        const auto sr = [&]() {
            if(!funcDecl->doesThisDeclarationHaveABody()) {
                return GetSourceRangeAfterSemi(funcDecl->getSourceRange(), result);
            }

            return funcDecl->getSourceRange();
        }();

        // DPrint("fd rw:  %d %s\n", (sr.getBegin() == sr.getEnd()), outputFormatHelper.GetString());

        if(sr.getBegin() != sr.getEnd()) {
            mRewrite.ReplaceText(sr, outputFormatHelper.GetString());
        } else {
            // special handling for at least using Base::Base for inheriting ctors from Base class.
            const auto ssr = GetSourceRangeAfterSemi(funcDecl->getSourceRange(), result);
            InsertIndentedText(ssr.getEnd(), outputFormatHelper);
        }

    } else if(const auto* friendDecl = result.Nodes.getNodeAs<FriendDecl>("friendDecl")) {
        if(const auto* fd = friendDecl->getFriendDecl()) {
            if(dyn_cast_or_null<FunctionTemplateDecl>(fd)) {
                // skip template definition
                return;
            }
        }

        const auto         columnNr = GetSM(result).getSpellingColumnNumber(GetBeginLoc(friendDecl)) - 1;
        OutputFormatHelper outputFormatHelper{columnNr};
        CodeGenerator      codeGenerator{outputFormatHelper};

        codeGenerator.InsertArg(friendDecl);

        DPrint("fd rw: %s\n", outputFormatHelper.GetString());
        mRewrite.ReplaceText(friendDecl->getSourceRange(), outputFormatHelper.GetString());
    } else if(const auto* usingDecl = result.Nodes.getNodeAs<UsingDecl>("using")) {
        // Check if one of the shadows contains a ConstructorUsingShadowDecl which implies Base::Base.
        for(const auto& shadow : usingDecl->shadows()) {
            if(isa<ConstructorUsingShadowDecl>(shadow)) {
                // This is a base ctor using. Remove this using. The actual code is inserted by the FunctionDecl
                // matcher.
                const auto sr = GetSourceRangeAfterSemi(usingDecl->getSourceRange(), result);
                mRewrite.RemoveText(sr);
                break;
            }
        }
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
