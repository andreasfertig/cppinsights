/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_GLOBAL_VARIABLE_HANDLER_H
#define INSIGHTS_GLOBAL_VARIABLE_HANDLER_H

#include "InsightsBase.h"                      // for InsightsBase
#include "clang/ASTMatchers/ASTMatchFinder.h"  // for MatchFinder, MatchFind...
namespace clang {
class Rewriter;
}
//-----------------------------------------------------------------------------

namespace clang::insights {

/// \brief Replace global variables to show e.g. implicit conversions.
class GlobalVariableHandler final : public ast_matchers::MatchFinder::MatchCallback, public InsightsBase
{
public:
    GlobalVariableHandler(Rewriter& rewrite, ast_matchers::MatchFinder& matcher);
    void run(const ast_matchers::MatchFinder::MatchResult& result) override;
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_GLOBAL_VARIABLE_HANDLER_H */
