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
#if IS_CLANG_NEWER_THAN(11)
#else
// Clang 12 has decompositionDecl but with different type
const internal::VariadicDynCastAllOfMatcher<Decl, DecompositionDecl> decompositionDecl;  // NOLINT
#endif

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

        const auto sr = GetSourceRangeAfterSemi(matchedDecl->getSourceRange(), result, RequireSemi::Yes);

        // Check whether we already have rewritten this location. If so, insert the text after the location. This is the
        // case for an anonymous struct declared with TU as root.
        if(not IsMacroLocation(matchedDecl->getSourceRange()) && (mRewrite.buffer_begin() != mRewrite.buffer_end()) &&
           IsAnonymousStructOrUnion(matchedDecl->getType()->getAsCXXRecordDecl())) {
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
                                 outputFormatHelper.GetString());

        } else {
            mRewrite.ReplaceText(sr, outputFormatHelper.GetString());
        }
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
