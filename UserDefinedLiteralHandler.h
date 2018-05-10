/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_USER_DEFINED_LITERAL_HANDLER_H
#define INSIGHTS_USER_DEFINED_LITERAL_HANDLER_H
//-----------------------------------------------------------------------------

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "InsightsBase.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

/// \brief Show places where user-defined literals are used.
///
/// For example when it comes to the use of chrono:
/// \code
/// static constexpr const seconds_t TIMEOUT      {1_s};
/// \endcode
class UserDefinedLiteralHandler final : public ast_matchers::MatchFinder::MatchCallback, public InsightsBase
{
public:
    UserDefinedLiteralHandler(Rewriter& rewrite, ast_matchers::MatchFinder& matcher);
    void run(const ast_matchers::MatchFinder::MatchResult& result) override;
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_USER_DEFINED_LITERAL_HANDLER_H */
