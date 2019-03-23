/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_TEMPLATE_HANDLER_H
#define INSIGHTS_TEMPLATE_HANDLER_H

#include "InsightsBase.h"                      // for InsightsBase
#include "clang/ASTMatchers/ASTMatchFinder.h"  // for MatchFinder, MatchFind...
namespace clang {
class Rewriter;
}
//-----------------------------------------------------------------------------

namespace clang::insights {

/// \brief Reveal the resulting code of an instantiated template.
///
/// Currently only template functions are supported. Classes are not generated fully. As placing the resulting code is
/// not easy all generated functions are placed behind a \c ifdef \c INSIGHTS_USE_TEMPLATE.
class TemplateHandler final : public ast_matchers::MatchFinder::MatchCallback, public InsightsBase
{
public:
    TemplateHandler(Rewriter& rewrite, ast_matchers::MatchFinder& matcher);
    void run(const ast_matchers::MatchFinder::MatchResult& result) override;
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_TEMPLATE_HANDLER_H */
