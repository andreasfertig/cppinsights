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

// XXX: recent clang source has a declType matcher. Try to figure out a migration path.
extern const internal::VariadicDynCastAllOfMatcher<Type, DecltypeType> myDecltypeType;
//-----------------------------------------------------------------------------

/* don't replace stuff in template definitions */
static const auto isTemplate = anyOf(hasAncestor(classTemplateDecl()),
                                     hasAncestor(functionTemplateDecl()),
                                     hasAncestor(classTemplateSpecializationDecl()));
//-----------------------------------------------------------------------------

static const auto isAutoAncestor =
    hasAncestor(varDecl(anyOf(hasType(autoType().bind("autoType")),
                              hasType(qualType(hasDescendant(autoType().bind("autoType")))),
                              /* decltype and decltype(auto) */
                              hasType(myDecltypeType().bind("dt")),
                              hasType(qualType(hasDescendant(myDecltypeType().bind("dt")))))));
//-----------------------------------------------------------------------------

/// \brief Shut up a unused variable warnings
#define SILENCE                                                                                                        \
    (void)Finder;                                                                                                      \
    (void)Builder

extern const internal::VariadicDynCastAllOfMatcher<Decl, VarTemplateDecl> varTemplateDecl;

// \brief Matches AST nodes of type \c FunctionDecl which is a template instantiation.
AST_MATCHER(FunctionDecl, isTemplateInstantiationPlain)
{
    SILENCE;
    return Node.isTemplateInstantiation();
}

AST_POLYMORPHIC_MATCHER(isMacroOrInvalidLocation, AST_POLYMORPHIC_SUPPORTED_TYPES(Decl, Stmt))
{
    SILENCE;

    return (insights::IsMacroLocation(insights::GetBeginLoc(Node)) ||
            insights::IsInvalidLocation(insights::GetBeginLoc(Node)));
}

AST_POLYMORPHIC_MATCHER(isInvalidLocation, AST_POLYMORPHIC_SUPPORTED_TYPES(Decl, Stmt))
{
    SILENCE;

    return insights::IsInvalidLocation(insights::GetBeginLoc(Node));
}

}  // namespace ast_matchers
}  // namespace clang
//-----------------------------------------------------------------------------

#endif /* INSIGHTS_MATCHERS_H */
