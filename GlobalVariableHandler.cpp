/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include <algorithm>

#include "CodeGenerator.h"
#include "GlobalVariableHandler.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::ast_matchers {
const internal::VariadicDynCastAllOfMatcher<Decl, VarTemplateSpecializationDecl> varTemplateSpecDecl;  // NOLINT
}  // namespace clang::ast_matchers

namespace clang::insights {

constexpr auto idVar{"varDevl"sv};
//-----------------------------------------------------------------------------

GlobalVariableHandler::GlobalVariableHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(
        varDecl(hasThisTUParent,
                unless(anyOf(
                    varTemplateSpecDecl(),
                    // A DecompositionDecl in global scope is different in the AST than one in a function for example.
                    // Try to find out whether this VarDecl is the result of a DecompositionDecl, if so bail out.
                    hasInitializer(ignoringImpCasts(
                        callExpr(hasAnyArgument(ignoringParenImpCasts(declRefExpr(to(decompositionDecl()))))))))))
            .bind(idVar),
        this);
}
//-----------------------------------------------------------------------------

void GlobalVariableHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* matchedDecl = result.Nodes.getNodeAs<VarDecl>(idVar)) {
        OutputFormatHelper outputFormatHelper{};

        // Take a preceding attribute into account
        const auto sr = [&] {
            auto tmpSr = GetSourceRangeAfterSemi(matchedDecl->getSourceRange(), result, RequireSemi::Yes);

            const auto&    attrs = matchedDecl->attrs();
            SourceLocation start = tmpSr.getBegin();

            std::for_each(
                attrs.begin(), attrs.end(), [&](const auto* attr) { start = std::min(attr->getLocation(), start); });

            return SourceRange{start, tmpSr.getEnd()};
        }();

        // Check whether we already have rewritten this location. If so, insert the text after the location. This is the
        // case for:
        // - an anonymous struct declared with TU as root
        // - a out-of-line static member variable of a class template
        if(not IsMacroLocation(matchedDecl->getSourceRange()) and (mRewrite.buffer_begin() != mRewrite.buffer_end()) &&
           (isTemplateInstantiation(matchedDecl->getTemplateSpecializationKind()) or
            IsAnonymousStructOrUnion(matchedDecl->getType()->getAsCXXRecordDecl()))) {
            if(auto&& text = mRewrite.getRewrittenText(sr); not text.empty()) {
                const char* data = result.SourceManager->getCharacterData(matchedDecl->getSourceRange().getBegin());

                StringRef sref{data, static_cast<size_t>(std::distance(text.begin(), text.end()))};

                if(not std::equal(text.begin(), text.end(), sref.begin(), sref.end())) {
                    outputFormatHelper.Append(std::move(text));
                }
            }
        }

        CodeGenerator codeGenerator{outputFormatHelper};
        codeGenerator.InsertArg(matchedDecl);

        if(IsMacroLocation(sr)) {
            // Special case for AnonymousStructInMacroTest.cpp (#290) where a macro gets expanded
            mRewrite.ReplaceText(GetSM(result).getImmediateExpansionRange(matchedDecl->getSourceRange().getBegin()),
                                 outputFormatHelper);

        } else {
            mRewrite.ReplaceText(sr, outputFormatHelper);
        }
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
