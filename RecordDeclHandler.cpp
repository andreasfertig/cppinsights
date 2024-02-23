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

constexpr auto idRecordDecl{"rd"sv};
constexpr auto idDecl{"decl"sv};
//-----------------------------------------------------------------------------

RecordDeclHandler::RecordDeclHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(
        cxxRecordDecl(hasDefinition(), hasThisTUParent, unless(anyOf(isLambda(), isTemplate))).bind(idRecordDecl),
        this);

    matcher.addMatcher(namespaceDecl(hasThisTUParent).bind(idDecl), this);

    matcher.addMatcher(enumDecl(hasThisTUParent).bind(idDecl), this);

    matcher.addMatcher(typeAliasDecl(hasThisTUParent).bind(idDecl), this);

    matcher.addMatcher(staticAssertDecl(hasThisTUParent).bind(idDecl), this);
}
//-----------------------------------------------------------------------------

void RecordDeclHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* cxxRecordDecl = result.Nodes.getNodeAs<CXXRecordDecl>(idRecordDecl)) {
        OutputFormatHelper outputFormatHelper{};

        CodeGeneratorVariant codeGenerator{outputFormatHelper};
        codeGenerator->InsertArg(cxxRecordDecl);

        if(auto sourceRange = cxxRecordDecl->getSourceRange(); not IsMacroLocation(sourceRange)) {
            if(IsAnonymousStructOrUnion(cxxRecordDecl)) {
                sourceRange.setEnd(sourceRange.getEnd().getLocWithOffset(2));  // 2 is just what worked
            }

            mRewrite.ReplaceText(GetSourceRangeAfterSemi(sourceRange, result, RequireSemi::Yes), outputFormatHelper);

        } else {
            // We're just interested in the start location, -1 work(s|ed)
            const auto startLoc =
                GetSourceRangeAfterSemi(sourceRange, result, RequireSemi::No).getBegin().getLocWithOffset(-1);

            InsertIndentedText(startLoc, outputFormatHelper);
        }

        // Catch all for:
        // - NamespaceDecl
        // - EnumDecl
        // - TypeAliasDecl
    } else if(const auto* decl = result.Nodes.getNodeAs<Decl>(idDecl)) {
        OutputFormatHelper outputFormatHelper{};

        CodeGeneratorVariant codeGenerator{outputFormatHelper};
        codeGenerator->InsertArg(decl);

        mRewrite.ReplaceText(GetSourceRangeAfterSemi(decl->getSourceRange(), result), outputFormatHelper);
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
