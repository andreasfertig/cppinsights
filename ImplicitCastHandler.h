/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_IMPLICIT_CAST_HANDLER_H
#define INSIGHTS_IMPLICIT_CAST_HANDLER_H

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "InsightsBase.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

/// \brief Show implicit casts.
///
/// Reveal all the places where the compiler need to do an implicit cast. This can be a conversion of a derived class to
/// its based, integer promotions or more.
class ImplicitCastHandler final : public ast_matchers::MatchFinder::MatchCallback, public InsightsBase
{
public:
    ImplicitCastHandler(Rewriter& rewrite, ast_matchers::MatchFinder& matcher);
    void run(const ast_matchers::MatchFinder::MatchResult& result) override;
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_IMPLICIT_CAST_HANDLER_H */
