/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "AutoStmtHandler.h"
#include "CodeGenerator.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "InsightsStaticStrings.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

// XXX: recent clang source has a declType matcher. Try to figure out a migration path.
const internal::VariadicDynCastAllOfMatcher<Type, DecltypeType> myDecltypeType;
//-----------------------------------------------------------------------------

namespace clang::insights {

AutoStmtHandler::AutoStmtHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{

    static const auto isAutoAncestor =
        hasAncestor(varDecl(anyOf(hasType(autoType().bind("autoType")),
                                  hasType(qualType(hasDescendant(autoType().bind("autoType")))),
                                  /* decltype and decltype(auto) */
                                  hasType(myDecltypeType().bind("dt")),
                                  hasType(qualType(hasDescendant(myDecltypeType().bind("dt")))))));

    matcher.addMatcher(varDecl(unless(anyOf(isExpansionInSystemHeader(),
                                            isMacroOrInvalidLocation(),
                                            isAutoAncestor,
                                            /* don't replace auto in templates */
                                            isTemplate,
                                            hasAncestor(functionDecl()))),
                               anyOf(/* auto */
                                     hasType(autoType().bind("autoType")),
                                     hasType(qualType(hasDescendant(autoType().bind("autoType")))),
                                     /* decltype and decltype(auto) */
                                     hasType(myDecltypeType().bind("dt")),
                                     hasType(qualType(hasDescendant(myDecltypeType().bind("dt"))))))
                           .bind("autoDecl"),
                       this);
}
//-----------------------------------------------------------------------------

void AutoStmtHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* autoDecl = result.Nodes.getNodeAs<VarDecl>("autoDecl")) {
        const auto&        sm       = GetSM(result);
        const auto         columnNr = sm.getSpellingColumnNumber(autoDecl->getLocStart()) - 1;
        OutputFormatHelper outputFormatHelper{columnNr};
        CodeGenerator      codeGenerator{outputFormatHelper};
        codeGenerator.InsertArg(autoDecl);

        // constexpr int* x = 5;
        // ^              ^   ^
        // 1              2   3
        // the SourceRange starts at (1) end ends at (3). The Location in the other hand starts at (1) and ends
        // at (2)
        const auto sr = GetSourceRangeAfterToken(autoDecl->getSourceRange(), tok::semi, result);

        mRewrite.ReplaceText(sr, outputFormatHelper.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
