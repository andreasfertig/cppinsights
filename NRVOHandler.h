/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_NRVO_HANDLER_H
#define INSIGHTS_NRVO_HANDLER_H

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "InsightsBase.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

/// \brief Show places where NRVO happens.
///
/// Named Return Value Optimization kick in under a couple of circumstances. This matches uses the AST knowledge and
/// filters variable declarations which are NRVO.
class NRVOHandler final : public ast_matchers::MatchFinder::MatchCallback, public InsightsBase
{
public:
    NRVOHandler(Rewriter& rewrite, ast_matchers::MatchFinder& matcher);
    void run(const ast_matchers::MatchFinder::MatchResult& result) override;
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_NRVO_HANDLER_H */
