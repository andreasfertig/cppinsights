/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_LAMBDA_HANDLER_H
#define INSIGHTS_LAMBDA_HANDLER_H
//-----------------------------------------------------------------------------

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "InsightsBase.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

/// \brief Show the transformation of a lambda to a class.
///
/// Transforms a lambda to the internal class representation with its members, mostly as described here:
/// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3559.pdf
class LambdaHandler final : public ast_matchers::MatchFinder::MatchCallback, public InsightsBase
{
public:
    LambdaHandler(Rewriter& rewrite, ast_matchers::MatchFinder& matcher);

    void run(const ast_matchers::MatchFinder::MatchResult& result) override;

private:
    static void InsertMethod(const Decl*          d,
                             OutputFormatHelper&  outputFormatHelper,
                             const CXXMethodDecl& md,
                             bool /*skipConstexpr*/);
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_LAMBDA_HANDLER_H */
