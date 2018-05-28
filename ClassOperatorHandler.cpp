/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "ClassOperatorHandler.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "InsightsStaticStrings.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

//#define MATCH_CXX_MEM_CEXPR

#ifdef MATCH_CXX_MEM_CEXPR
static inline llvm::StringRef GetSourceText(const SourceManager& SM, const SourceRange& range)
{
    // Use LLVM's lexer to get source text.
    return Lexer::getSourceText(CharSourceRange::getCharRange(range), SM, LangOptions());
}
//-----------------------------------------------------------------------------
#endif

ClassOperatorHandler::ClassOperatorHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    const auto operatorMatcher = anyOf(isExpansionInSystemHeader(),
                                       isMacroOrInvalidLocation(),
                                       isTemplate,
                                       hasAncestor(ifStmt()),
                                       hasAncestor(switchStmt()),
                                       /* if we match the top-most CXXOperatorCallExpr we will see all
                                          descendants. So filter them here to avoid getting them multiple times */
                                       hasAncestor(cxxOperatorCallExpr()),
                                       hasLambdaAncestor,
                                       hasAncestor(implicitCastExpr(hasMatchingCast())),
                                       hasAncestor(userDefinedLiteral()),
                                       hasAncestor(decompositionDecl()),
                                       hasAncestor(cxxForRangeStmt())
#ifdef MATCH_CXX_MEM_CEXPR
                                           ,
                                       hasAncestor(cxxMemberCallExpr())
#endif
    );

    matcher.addMatcher(
        cxxOperatorCallExpr(
            unless(anyOf(operatorMatcher,
                         hasAncestor(unaryOperator(anyOf(hasOperatorName("++"), hasOperatorName("--")))))))
            .bind("operator"),
        this);

#ifdef MATCH_CXX_MEM_CEXPR
    matcher.addMatcher(
        cxxMemberCallExpr(
            unless(anyOf(isExpansionInSystemHeader(),
                         isTemplate,
                         isMacroOrInvalidLocation(),
                         hasAncestor(ifStmt()),
                         hasAncestor(switchStmt()),
                         hasAncestor(cxxOperatorCallExpr()),
                         hasAncestor(unaryOperator()),
                         hasAncestor(cxxMemberCallExpr()),
                         hasAncestor(implicitCastExpr(hasMatchingCast())),
                         hasAncestor(userDefinedLiteral()),
                         hasLambdaAncestor,
                         /*                                                   hasDescendant(cxxTemporaryObjectExpr()),*/
                         hasAncestor(cxxForRangeStmt()))))
            .bind("memberCallExpr"),
        this);
#endif
}
//-----------------------------------------------------------------------------

void ClassOperatorHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* callOperatorExpr = result.Nodes.getNodeAs<CXXOperatorCallExpr>("operator")) {
        OutputFormatHelper outputFormatHelper{};
        CodeGenerator      codeGenerator{outputFormatHelper};

        codeGenerator.InsertArg(callOperatorExpr);

        DPrint("replacement: %s\n", outputFormatHelper.GetString());
        mRewrite.ReplaceText(callOperatorExpr->getSourceRange(), outputFormatHelper.GetString());

#ifdef MATCH_CXX_MEM_CEXPR
    } else if(const auto* CxxMemberCallExpr = result.Nodes.getNodeAs<CXXMemberCallExpr>("memberCallExpr")) {
        DPrint("memexpr\n");

        if(CxxMemberCallExpr->getSourceRange().isInvalid()) {
            Error("2invalid range\n");
            return;
        }

        SKIP_IF_ALREADY_SEEN(CxxMemberCallExpr);

        Dump(CxxMemberCallExpr);

        OutputFormatHelper outputFormatHelper{};
        CodeGenerator      codeGenerator{outputFormatHelper};

        codeGenerator.InsertArg(CxxMemberCallExpr);

        const auto txt = GetSourceText(*result.SourceManager, CxxMemberCallExpr->getSourceRange());
        DPrint("s: %d %s\n", txt.size(), txt);

        SourceRange r{GetSourceRangeAfterToken(CxxMemberCallExpr->getSourceRange(), tok::r_paren, result)};

        DPrint("txt: %d %s\n", mRewrite.getRangeSize(r), outputFormatHelper.GetString());
        mRewrite.ReplaceText(CxxMemberCallExpr->getLocStart(), txt.size() + 1, outputFormatHelper.GetString());
#endif
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
