/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "TemplateHandler.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "InsightsStaticStrings.h"
#include "OutputFormatHelper.h"

#include "llvm/Support/Path.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {
TemplateHandler::TemplateHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(functionDecl(allOf(unless(isExpansionInSystemHeader()),
                                          unless(isMacroOrInvalidLocation()),
                                          hasParent(functionTemplateDecl()),
                                          isTemplateInstantiationPlain()))
                           .bind("func"),
                       this);

    matcher.addMatcher(classTemplateSpecializationDecl(unless(isExpansionInSystemHeader()),
                                                       hasParent(classTemplateDecl().bind("decl")))
                           .bind("class"),
                       this);
}
//-----------------------------------------------------------------------------

void TemplateHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* functionDecl = result.Nodes.getNodeAs<FunctionDecl>("func")) {
        InsertInstantiatedTemplate(*functionDecl, result);

    } else if(const auto* clsTmplSpecDecl = result.Nodes.getNodeAs<ClassTemplateSpecializationDecl>("class")) {

        OutputFormatHelper outputFormatHelper{};
        outputFormatHelper.AppendNewLine();

        const auto& sm = GetSM(result);
        InsertInstantiationPoint(outputFormatHelper, sm, clsTmplSpecDecl->getPointOfInstantiation());

        outputFormatHelper.AppendNewLine("#ifdef INSIGHTS_USE_TEMPLATE");

        outputFormatHelper.Append(kwClassSpace, GetName(*clsTmplSpecDecl));

        CodeGenerator codeGenerator{outputFormatHelper};
        codeGenerator.InsertTemplateArgs(*clsTmplSpecDecl);

        outputFormatHelper.AppendNewLine();

        outputFormatHelper.OpenScope();

        outputFormatHelper.CloseScopeWithSemi();
        outputFormatHelper.AppendNewLine();

        outputFormatHelper.AppendNewLine();
        outputFormatHelper.AppendNewLine("#endif");

        const auto* clsTmplDecl = result.Nodes.getNodeAs<ClassTemplateDecl>("decl");
        const auto  endOfCond   = FindLocationAfterToken(clsTmplDecl->getLocEnd(), tok::semi, result);

        mRewrite.InsertText(endOfCond, outputFormatHelper.GetString(), true, true);
    }
}
//-----------------------------------------------------------------------------

void TemplateHandler::InsertInstantiatedTemplate(const FunctionDecl& funcDecl, const MatchFinder::MatchResult& result)
{
    if(const auto* body = funcDecl.getBody()) {
        OutputFormatHelper outputFormatHelper{};
        outputFormatHelper.AppendNewLine();
        outputFormatHelper.AppendNewLine();

        const auto& sm = GetSM(result);
        InsertInstantiationPoint(outputFormatHelper, sm, funcDecl.getPointOfInstantiation());
        outputFormatHelper.AppendNewLine("#ifdef INSIGHTS_USE_TEMPLATE");

        GenerateFunctionPrototype(outputFormatHelper, funcDecl);

        outputFormatHelper.AppendNewLine();

        CodeGenerator codeGenerator{outputFormatHelper};
        codeGenerator.InsertArg(body);

        outputFormatHelper.AppendNewLine();
        outputFormatHelper.AppendNewLine("#endif");

        const auto endOfCond = FindLocationAfterToken(funcDecl.getLocEnd(), tok::semi, result);

        mRewrite.InsertText(endOfCond.getLocWithOffset(1), outputFormatHelper.GetString(), true, true);
    }
}
//-----------------------------------------------------------------------------

void TemplateHandler::InsertInstantiationPoint(OutputFormatHelper&   outputFormatHelper,
                                               const SourceManager&  sm,
                                               const SourceLocation& instLoc)
{
    const auto  lineNr = sm.getSpellingLineNumber(instLoc);
    const auto& fileId = sm.getFileID(instLoc);
    const auto* file   = sm.getFileEntryForID(fileId);
    if(file) {
        const auto fileWithDirName = file->getName();
        const auto fileName        = llvm::sys::path::filename(fileWithDirName);

        outputFormatHelper.AppendNewLine("/* First instantiated from: ", fileName, ":", std::to_string(lineNr), " */");
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
