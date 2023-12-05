/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "FunctionDeclHandler.h"
#include "CodeGenerator.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {
// This is a modified version based on LLVM: clang-tools-extra/clang-tidy/modernize/UseTrailingReturnTypeCheck.cpp
static SourceLocation findTrailingReturnTypeSourceLocation(const FunctionDecl&  func,
                                                           const ASTContext&    ctx,
                                                           const SourceManager& SM,
                                                           const LangOptions&   langOpts)
{
    const TypeSourceInfo* tsi = func.getTypeSourceInfo();
    if(not tsi) {
        return {};
    }

    FunctionTypeLoc ftl = tsi->getTypeLoc().IgnoreParens().getAs<FunctionTypeLoc>();
    if(not ftl) {
        return {};
    }

    // We start with the location of the closing parenthesis.
    if(const auto exceptionSpecRange{func.getExceptionSpecSourceRange()}; exceptionSpecRange.isValid()) {
        return Lexer::getLocForEndOfToken(exceptionSpecRange.getEnd(), 0, SM, langOpts);
    }

    // If the function argument list ends inside of a macro, it is dangerous to start lexing from here - bail out.
    const SourceLocation closingParen = ftl.getRParenLoc();
    if(closingParen.isMacroID()) {
        return {};
    }

    SourceLocation result = Lexer::getLocForEndOfToken(closingParen, 0, SM, langOpts);

    // Skip subsequent CV and ref qualifiers.
    const auto [fileId, loc] = SM.getDecomposedLoc(result);
    StringRef   file         = SM.getBufferData(fileId);
    const char* tokenBegin   = file.data() + loc;
    Lexer       Lexer(SM.getLocForStartOfFile(fileId), langOpts, file.begin(), tokenBegin, file.end());

    for(Token tok; not Lexer.LexFromRawLexer(tok);) {
        if(tok.is(tok::raw_identifier)) {
            IdentifierInfo& Info = ctx.Idents.get(StringRef(SM.getCharacterData(tok.getLocation()), tok.getLength()));
            tok.setIdentifierInfo(&Info);
            tok.setKind(Info.getTokenID());
        }

        if(tok.isOneOf(tok::amp, tok::ampamp, tok::kw_const, tok::kw_volatile, tok::kw_restrict)) {
            result = tok.getEndLoc();
            continue;
        }

        break;
    }

    return result;
}
//-----------------------------------------------------------------------------

constexpr auto idFunc{"func"sv};
//-----------------------------------------------------------------------------

FunctionDeclHandler::FunctionDeclHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    const auto hasThisTUParentNonTemplate = allOf(hasThisTUParent, unless(anyOf(cxxMethodDecl(), isTemplate)));

    matcher.addMatcher(functionDecl(hasThisTUParentNonTemplate).bind(idFunc), this);
}
//-----------------------------------------------------------------------------

void FunctionDeclHandler::run(const MatchFinder::MatchResult& result)
{
    if(const auto* funcDecl = result.Nodes.getNodeAs<FunctionDecl>(idFunc)) {
        RETURN_IF(isa<CXXDeductionGuideDecl>(funcDecl));

        const auto         columnNr = GetSM(result).getSpellingColumnNumber(funcDecl->getBeginLoc()) - 1;
        OutputFormatHelper outputFormatHelper{columnNr};
        CodeGenerator      codeGenerator{outputFormatHelper};

        codeGenerator.InsertArg(funcDecl);

        // Find the correct ending of the source range. In case of a declaration we need to find the ending semi,
        // otherwise the provided source range is correct.
        const auto sr = [&] {
            // get the end-location of a function with a trailing return type.
            const SourceLocation ll = [&] {
                if(not funcDecl->doesThisDeclarationHaveABody() and
                   funcDecl->getType()->getAs<FunctionProtoType>()->hasTrailingReturn() and
                   isa<DecltypeType>(funcDecl->getDeclaredReturnType().getTypePtrOrNull())) {
                    const auto retStr = funcDecl->getDeclaredReturnType().getAsString();
                    return findTrailingReturnTypeSourceLocation(
                               *funcDecl, *result.Context, GetSM(result), GetLangOpts(*funcDecl))
                        .getLocWithOffset(retStr.size());
                }

                return funcDecl->getSourceRange().getEnd();
            }();

            auto funcRange =
                GetSourceRangeAfterSemi({funcDecl->getSourceRange().getBegin(), ll},
                                        result,
                                        funcDecl->doesThisDeclarationHaveABody() ? RequireSemi::Yes : RequireSemi::Yes);

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

        mRewrite.ReplaceText(sr, outputFormatHelper);
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
