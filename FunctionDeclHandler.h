/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_FUNCTION_DECL_HANDLER_H
#define INSIGHTS_FUNCTION_DECL_HANDLER_H

#include "InsightsBase.h"                      // for InsightsBase
#include "clang/ASTMatchers/ASTMatchFinder.h"  // for MatchFinder, MatchFind...
namespace clang {
class Rewriter;
}
//-----------------------------------------------------------------------------

namespace clang::insights {

/// \brief Match function decls
///
/// To get a broader rewrite of statements match the entire function and rewrite it.
class FunctionDeclHandler final : public ast_matchers::MatchFinder::MatchCallback, public InsightsBase
{
public:
    FunctionDeclHandler(Rewriter& rewrite, ast_matchers::MatchFinder& matcher);
    void run(const ast_matchers::MatchFinder::MatchResult& result) override;
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_FUNCTION_DECL_HANDLER_H */
