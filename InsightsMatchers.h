/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_MATCHERS_H
#define INSIGHTS_MATCHERS_H

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"

#include "ClangCompat.h"
#include "InsightsHelpers.h"
//-----------------------------------------------------------------------------

namespace clang {
namespace ast_matchers {

/* don't replace stuff in template definitions */
static const auto isTemplate = anyOf(hasAncestor(classTemplateDecl()),
                                     hasAncestor(functionTemplateDecl()),
                                     hasAncestor(classTemplateSpecializationDecl()));
//-----------------------------------------------------------------------------

/// \brief Shut up a unused variable warnings
#define SILENCE                                                                                                        \
    (void)Finder;                                                                                                      \
    (void)Builder
//-----------------------------------------------------------------------------

extern const internal::VariadicDynCastAllOfMatcher<Decl, VarTemplateDecl> varTemplateDecl;
//-----------------------------------------------------------------------------

// \brief Matches AST nodes of type \c FunctionDecl which is a template instantiation.
AST_MATCHER(FunctionDecl, isTemplateInstantiationPlain)
{
    SILENCE;
    return Node.isTemplateInstantiation();
}
//-----------------------------------------------------------------------------

AST_POLYMORPHIC_MATCHER(isMacroOrInvalidLocation, AST_POLYMORPHIC_SUPPORTED_TYPES(Decl, Stmt))
{
    SILENCE;

    return (insights::IsMacroLocation(Node.getBeginLoc()) or insights::IsInvalidLocation(Node.getBeginLoc()));
}
//-----------------------------------------------------------------------------

AST_POLYMORPHIC_MATCHER(isInvalidLocation, AST_POLYMORPHIC_SUPPORTED_TYPES(Decl, Stmt))
{
    SILENCE;

    return insights::IsInvalidLocation(Node.getBeginLoc());
}
//-----------------------------------------------------------------------------

inline const auto hasThisTUParent =
    allOf(unless(isExpansionInSystemHeader()), unless(isInvalidLocation()), hasParent(translationUnitDecl()));
//-----------------------------------------------------------------------------

}  // namespace ast_matchers
}  // namespace clang
//-----------------------------------------------------------------------------

#endif /* INSIGHTS_MATCHERS_H */
