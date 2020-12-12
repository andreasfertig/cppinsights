/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "FunctionDeclHandler.h"
#include "ClangCompat.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

FunctionDeclHandler::FunctionDeclHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(functionDecl(unless(anyOf(cxxMethodDecl(),
                                                 isExpansionInSystemHeader(),
                                                 isTemplate,
                                                 hasParent(linkageSpecDecl()),  // filter this out for coroutines
                                                 hasAncestor(friendDecl()),     // friendDecl has functionDecl as child
                                                 hasAncestor(functionDecl()),   // prevent forward declarations
                                                 hasAncestor(namespaceDecl()),
                                                 isInvalidLocation())))
                           .bind("funcDecl"),
                       this);

    static const auto hasTemplateDescendant = anyOf(hasDescendant(classTemplateDecl()),
                                                    hasDescendant(functionTemplateDecl()),
                                                    hasDescendant(classTemplateSpecializationDecl()));

    matcher.addMatcher(friendDecl(unless(anyOf(cxxMethodDecl(),
                                               hasAncestor(cxxRecordDecl()),
                                               hasAncestor(namespaceDecl()),
                                               isExpansionInSystemHeader(),
                                               isTemplate,
                                               hasTemplateDescendant,
                                               hasAncestor(functionDecl()),  // prevent forward declarations
                                               isInvalidLocation())))
                           .bind("friendDecl"),
                       this);
}
//-----------------------------------------------------------------------------

void FunctionDeclHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* funcDecl = result.Nodes.getNodeAs<FunctionDecl>("funcDecl")) {
        const auto         columnNr = GetSM(result).getSpellingColumnNumber(GetBeginLoc(funcDecl)) - 1;
        OutputFormatHelper outputFormatHelper{columnNr};
        CodeGenerator      codeGenerator{outputFormatHelper};

        codeGenerator.InsertArg(funcDecl);

        // Find the correct ending of the source range. In case of a declaration we need to find the ending semi,
        // otherwise the provided source range is correct.
        const auto sr = [&] {
            auto funcRange =
                GetSourceRangeAfterSemi(funcDecl->getSourceRange(),
                                        result,
                                        funcDecl->doesThisDeclarationHaveABody() ? RequireSemi::No : RequireSemi::Yes);

            // Adjust the begin location, if this decl has an attribute
            if(funcDecl->hasAttrs()) {
                // Find the first attribute with a valid source-location
                for(const auto& attr : funcDecl->attrs()) {
                    if(const auto location = attr->getLocation(); location.isValid()) {

                        // only use the begin location of the attribute, if it is before the one of the function decl.
                        if(funcRange.getBegin() > location) {
                            // the -3 are a guess that seems to work
                            funcRange.setBegin(location.getLocWithOffset(-3));
                        }

                        return funcRange;
                    }
                }
            }

            return funcRange;
        }();

        // DPrint("fd rw:  %d %s\n", (sr.getBegin() == sr.getEnd()), outputFormatHelper.GetString());

        mRewrite.ReplaceText(sr, outputFormatHelper.GetString());
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
