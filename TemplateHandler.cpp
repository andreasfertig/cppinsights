/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "TemplateHandler.h"
#include <type_traits>
#include "ClangCompat.h"
#include "CodeGenerator.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "OutputFormatHelper.h"

#include "llvm/Support/Path.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::ast_matchers {
const internal::VariadicDynCastAllOfMatcher<Decl, VarTemplateDecl> varTemplateDecl;
}

namespace clang::insights {
/// \brief Inserts the instantiation point of a template.
//
// This reveals at which place the template is first used.
static void
InsertInstantiationPoint(OutputFormatHelper& outputFormatHelper, const SourceManager& sm, const SourceLocation& instLoc)
{
    const auto  lineNo = sm.getSpellingLineNumber(instLoc);
    const auto& fileId = sm.getFileID(instLoc);
    const auto* file   = sm.getFileEntryForID(fileId);
    if(file) {
        const auto fileWithDirName = file->getName();
        const auto fileName        = llvm::sys::path::filename(fileWithDirName);

        outputFormatHelper.AppendNewLine("/* First instantiated from: ", fileName, ":", lineNo, " */");
    }
}
//-----------------------------------------------------------------------------

/// \brief Insert the instantiated template with the resulting code.
template<typename T>
static OutputFormatHelper InsertInstantiatedTemplate(const T& decl, const MatchFinder::MatchResult& result)
{
    OutputFormatHelper outputFormatHelper{};
    outputFormatHelper.AppendNewLine();
    outputFormatHelper.AppendNewLine();

    const auto& sm = GetSM(result);

    if constexpr(not std::is_same_v<VarTemplateDecl, T>) {
        InsertInstantiationPoint(outputFormatHelper, sm, decl.getPointOfInstantiation());
    }

    outputFormatHelper.AppendNewLine("#ifdef INSIGHTS_USE_TEMPLATE");
    CodeGenerator codeGenerator{outputFormatHelper};

    if constexpr(std::is_same_v<VarTemplateDecl, T>) {
        for(const auto& spec : decl.specializations()) {
            InsertInstantiationPoint(outputFormatHelper, sm, spec->getPointOfInstantiation());
            codeGenerator.InsertArg(spec);
        }
    } else {
        codeGenerator.InsertArg(&decl);
    }

    outputFormatHelper.AppendNewLine("#endif");

    return outputFormatHelper;
}
//-----------------------------------------------------------------------------

TemplateHandler::TemplateHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(
        functionDecl(allOf(unless(isExpansionInSystemHeader()),
                           unless(isMacroOrInvalidLocation()),
                           hasParent(functionTemplateDecl(unless(hasParent(classTemplateSpecializationDecl())),
                                                          unless(hasParent(cxxRecordDecl(isLambda()))))),
                           isTemplateInstantiationPlain()))
            .bind("func"),
        this);

    matcher.addMatcher(classTemplateSpecializationDecl(unless(isExpansionInSystemHeader()),
                                                       hasParent(classTemplateDecl().bind("decl")))
                           .bind("class"),
                       this);

    matcher.addMatcher(
        varTemplateDecl(unless(isExpansionInSystemHeader()), unless(hasParent(classTemplateDecl()))).bind("vd"), this);
}
//-----------------------------------------------------------------------------

void TemplateHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* functionDecl = result.Nodes.getNodeAs<FunctionDecl>("func")) {
        if(not functionDecl->getBody()) {
            return;
        }

        OutputFormatHelper outputFormatHelper = InsertInstantiatedTemplate(*functionDecl, result);
        const auto         endOfCond          = FindLocationAfterToken(GetEndLoc(*functionDecl), tok::semi, result);

        InsertIndentedText(endOfCond.getLocWithOffset(1), outputFormatHelper);

    } else if(const auto* clsTmplSpecDecl = result.Nodes.getNodeAs<ClassTemplateSpecializationDecl>("class")) {
        // skip classes/struct's without a definition
        if(not clsTmplSpecDecl->hasDefinition()) {
            return;
        }

        OutputFormatHelper outputFormatHelper = InsertInstantiatedTemplate(*clsTmplSpecDecl, result);
        const auto*        clsTmplDecl        = result.Nodes.getNodeAs<ClassTemplateDecl>("decl");
        const auto         endOfCond          = FindLocationAfterToken(GetEndLoc(clsTmplDecl), tok::semi, result);

        InsertIndentedText(endOfCond, outputFormatHelper);

    } else if(const auto* vd = result.Nodes.getNodeAs<VarTemplateDecl>("vd")) {
        OutputFormatHelper outputFormatHelper = InsertInstantiatedTemplate(*vd, result);

        const auto endOfCond = FindLocationAfterToken(GetEndLoc(vd), tok::semi, result);
        InsertIndentedText(endOfCond, outputFormatHelper);
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
