/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_COMMA_OPERATOR_HANDLER_H
#define INSIGHTS_COMMA_OPERATOR_HANDLER_H

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "InsightsBase.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

/// \brief Transform statements which are separated by the comma operator.
///
/// For example:
/// \code
/// [&]() {}(), []() {} ();
/// \endcode
class CommaOperatorHandler final : public ast_matchers::MatchFinder::MatchCallback, public InsightsBase
{
public:
    CommaOperatorHandler(Rewriter& rewrite, ast_matchers::MatchFinder& matcher);
    void run(const ast_matchers::MatchFinder::MatchResult& result) override;
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_COMMA_OPERATOR_HANDLER_H */
