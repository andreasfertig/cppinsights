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
    matcher.addMatcher(
        cxxRecordDecl(hasDefinition(), hasThisTUParent, unless(anyOf(isLambda(), isTemplate))).bind("cxxRecordDecl"),
        this);

    matcher.addMatcher(namespaceDecl(hasThisTUParent).bind("namespaceDecl"), this);

    matcher.addMatcher(enumDecl(hasThisTUParent).bind("enumDecl"), this);

    matcher.addMatcher(typeAliasDecl(hasThisTUParent).bind("typeAliasDecl"), this);
}
//-----------------------------------------------------------------------------

void RecordDeclHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* cxxRecordDecl = result.Nodes.getNodeAs<CXXRecordDecl>("cxxRecordDecl")) {
        OutputFormatHelper outputFormatHelper{};

        CodeGenerator codeGenerator{outputFormatHelper};
        codeGenerator.InsertArg(cxxRecordDecl);

        if(auto sourceRange = cxxRecordDecl->getSourceRange(); not IsMacroLocation(sourceRange)) {
            if(IsAnonymousStructOrUnion(cxxRecordDecl)) {
                sourceRange.setEnd(sourceRange.getEnd().getLocWithOffset(2));  // 2 is just what worked
            }

            mRewrite.ReplaceText(GetSourceRangeAfterSemi(sourceRange, result, RequireSemi::Yes),
                                 outputFormatHelper.GetString());

        } else {
            // We're just interested in the start location, -1 work(s|ed)
            const auto startLoc =
                GetSourceRangeAfterSemi(sourceRange, result, RequireSemi::No).getBegin().getLocWithOffset(-1);

            InsertIndentedText(startLoc, outputFormatHelper);
        }

    } else if(const auto* namespaceDecl = result.Nodes.getNodeAs<NamespaceDecl>("namespaceDecl")) {
        OutputFormatHelper outputFormatHelper{};

        CodeGenerator codeGenerator{outputFormatHelper};
        codeGenerator.InsertArg(namespaceDecl);

        mRewrite.ReplaceText(GetSourceRangeAfterSemi(namespaceDecl->getSourceRange(), result, RequireSemi::No),
                             outputFormatHelper.GetString());

    } else if(const auto* enumDecl = result.Nodes.getNodeAs<EnumDecl>("enumDecl")) {
        OutputFormatHelper outputFormatHelper{};

        CodeGenerator codeGenerator{outputFormatHelper};
        codeGenerator.InsertArg(enumDecl);

        mRewrite.ReplaceText(GetSourceRangeAfterSemi(enumDecl->getSourceRange(), result, RequireSemi::No),
                             outputFormatHelper.GetString());

    } else if(const auto* typeAliasDecl = result.Nodes.getNodeAs<TypeAliasDecl>("typeAliasDecl")) {
        OutputFormatHelper outputFormatHelper{};

        CodeGenerator codeGenerator{outputFormatHelper};
        codeGenerator.InsertArg(typeAliasDecl);

        mRewrite.ReplaceText(GetSourceRangeAfterSemi(typeAliasDecl->getSourceRange(), result, RequireSemi::No),
                             outputFormatHelper.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
