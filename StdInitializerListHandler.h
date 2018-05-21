/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_STD_INITIALIZER_LIST_HANDLER_H
#define INSIGHTS_STD_INITIALIZER_LIST_HANDLER_H

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "InsightsBase.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

/// \brief Show where \c std::initializer_list is used.
///
/// With C++11 uniform initialization and  initializer lists we cannot
/// longer easily see whether we are calling a normal constructor or one
/// which takes a initializer_list. For example:
///
/// \begincode
/// struct V {
///   V(int x) {}
///   V(std::initializer_list<int> x) {}
/// };
///
/// V v{1};
/// V vv{1, 2};
/// \endcode
///
/// Surprisingly to some people, in both cases the second constructor, taking
/// a std::initializer_list, is used.
class StdInitializerListHandler final : public ast_matchers::MatchFinder::MatchCallback, public InsightsBase
{
public:
    StdInitializerListHandler(Rewriter& rewrite, ast_matchers::MatchFinder& matcher);
    void run(const ast_matchers::MatchFinder::MatchResult& result) override;
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_STD_INITIALIZER_LIST_HANDLER_H */
