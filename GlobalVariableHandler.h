/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_GLOBAL_VARIABLE_HANDLER_H
#define INSIGHTS_GLOBAL_VARIABLE_HANDLER_H

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "InsightsBase.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

/// \brief Show the result of a static_assert.
class GlobalVariableHandler final : public ast_matchers::MatchFinder::MatchCallback, public InsightsBase
{
public:
    GlobalVariableHandler(Rewriter& rewrite, ast_matchers::MatchFinder& matcher);
    void run(const ast_matchers::MatchFinder::MatchResult& result) override;
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_GLOBAL_VARIABLE_HANDLER_H */
