/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "StdInitializerListHandler.h"
#include "CodeGenerator.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "InsightsStaticStrings.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

StdInitializerListHandler::StdInitializerListHandler(Rewriter& rewrite, MatchFinder& matcher)
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
                                       hasAncestor(cxxForRangeStmt())
#ifdef MATCH_CXX_MEM_CEXPR
                                           ,
                                       hasAncestor(cxxMemberCallExpr())
#endif
    );

    matcher.addMatcher(
        cxxStdInitializerListExpr(unless(operatorMatcher), hasParent(expr().bind("expr"))).bind("initializer"), this);
}
//-----------------------------------------------------------------------------

void StdInitializerListHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* initList = result.Nodes.getNodeAs<CXXStdInitializerListExpr>("initializer")) {
        OutputFormatHelper outputFormatHelper{};
        CodeGenerator      codeGenerator{outputFormatHelper};

        const auto* expr = result.Nodes.getNodeAs<CXXConstructExpr>("expr");

        // If an initializer list appears in a constructor as a single argument we need to add braces.
        const bool isCtorInit{[&]() {
            if(!expr) {
                return false;
            }

            // if the first argument is a std::initializer-list and the second is a default argument we need to add
            // the braces.
            if(1 < expr->getNumArgs()) {
                // Skip the first argument: ++(expr->arg_begin())
                for(const auto& arg : llvm::make_range(++(expr->arg_begin()), expr->arg_end())) {
                    if(!arg->isDefaultArgument()) {
                        return false;
                    }
                }

                return true;
            }

            return (1 == expr->getNumArgs());
        }()};

        if(isCtorInit) {
            outputFormatHelper.Append("{ ");
        }

        codeGenerator.InsertArg(initList);

        if(isCtorInit) {
            outputFormatHelper.Append(" }");
        }

        DPrint("replacementInit: %s\n", outputFormatHelper.GetString());
        mRewrite.ReplaceText(initList->getSourceRange(), outputFormatHelper.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
