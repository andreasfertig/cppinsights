/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_HELPERS_H
#define INSIGHTS_HELPERS_H
//-----------------------------------------------------------------------------

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/Lexer.h"

#include <string>

#include "InsightsStrongTypes.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

static inline bool IsNewLine(const char c)
{
    return '\n' == c;
}
//-----------------------------------------------------------------------------

std::string BuildInternalVarName(const std::string& varName);
std::string BuildInternalVarName(const std::string& varName, const SourceLocation& loc, const SourceManager& SM);

SourceLocation FindLocationAfterSemi(const SourceLocation loc, const ast_matchers::MatchFinder::MatchResult& Result);
SourceRange    GetSourceRangeAfterSemi(const SourceRange range, const ast_matchers::MatchFinder::MatchResult& Result);
//-----------------------------------------------------------------------------

static inline bool IsMacroLocation(const SourceLocation& loc)
{
    return loc.isMacroID();
}
//-----------------------------------------------------------------------------

static inline bool IsMacroLocation(const SourceRange& range)
{
    return IsMacroLocation(range.getBegin()) || IsMacroLocation(range.getEnd());
}
//-----------------------------------------------------------------------------

template<typename T, typename... Args>
static inline bool IsMacroLocation(const T& t, Args... args)
{
    return (IsMacroLocation(t) || IsMacroLocation(args...));
}
//-----------------------------------------------------------------------------

static inline bool IsInvalidLocation(const SourceLocation& loc)
{
    return loc.isInvalid();
}
//-----------------------------------------------------------------------------

static inline bool IsInvalidLocation(const SourceRange& range)
{
    return IsInvalidLocation(range.getBegin()) || IsInvalidLocation(range.getEnd());
}
//-----------------------------------------------------------------------------

template<typename T, typename... Args>
static inline bool IsInvalidLocation(const T& t, Args... args)
{
    return (IsInvalidLocation(t) || IsInvalidLocation(args...));
}
//-----------------------------------------------------------------------------

#define SKIP_MACRO_LOCATION(...)                                                                                       \
    {                                                                                                                  \
        const bool isMacro{IsMacroLocation(__VA_ARGS__) || IsInvalidLocation(__VA_ARGS__)};                            \
        if(isMacro) {                                                                                                  \
            return;                                                                                                    \
        } else {                                                                                                       \
        }                                                                                                              \
    }
//-----------------------------------------------------------------------------

void InsertBefore(std::string& source, const std::string& find, const std::string& replace);
//-----------------------------------------------------------------------------

static inline const SourceManager& GetSM(const ast_matchers::MatchFinder::MatchResult& Result)
{
    return Result.Context->getSourceManager();
}
//-----------------------------------------------------------------------------

static inline const SourceManager& GetSM(const Decl& decl)
{
    return decl.getASTContext().getSourceManager();
}
//-----------------------------------------------------------------------------

static inline const LangOptions& GetLangOpts(const Decl& decl)
{
    return decl.getASTContext().getLangOpts();
}
//-----------------------------------------------------------------------------

std::string GetNameAsWritten(const QualType& t);

bool IsTrivialStaticClassVarDecl(const VarDecl& varDecl);
//-----------------------------------------------------------------------------

/*
 * Get the name of a DeclRefExpr without the namespace
 */
static inline std::string GetPlainName(const DeclRefExpr& DRE)
{
    return DRE.getNameInfo().getAsString();
}
//-----------------------------------------------------------------------------

std::string GetName(const DeclRefExpr& DRE);
std::string GetName(const VarDecl& VD);
//-----------------------------------------------------------------------------

static inline std::string GetName(const NamedDecl& ND)
{
    return ND.getNameAsString();
}
//-----------------------------------------------------------------------------

std::string GetName(const NamedDecl& namedDecl);
//-----------------------------------------------------------------------------

std::string GetNameAsFunctionPointer(const QualType& t);
//-----------------------------------------------------------------------------

std::string GetLambdaName(const CXXRecordDecl& lambda);

static inline std::string GetLambdaName(const LambdaExpr& lambda)
{
    return GetLambdaName(*lambda.getLambdaClass());
}
//-----------------------------------------------------------------------------

static inline std::string GetName(const CXXRecordDecl& RD)
{
    if(RD.isLambda()) {
        return GetLambdaName(RD);
    }

    return RD.getNameAsString();
}
//-----------------------------------------------------------------------------

static inline std::string GetName(const CXXMethodDecl& RD)
{
    if(RD.getParent()->isLambda()) {
        return GetLambdaName(*RD.getParent());
    }

    return RD.getNameAsString();
}
//-----------------------------------------------------------------------------

/// \brief Remove decltype from a QualType, if possible.
const QualType GetDesugarType(const QualType& QT);
// -----------------------------------------------------------------------------

static inline QualType GetDesugarReturnType(const FunctionDecl& FD)
{
    return GetDesugarType(FD.getReturnType());
}
//-----------------------------------------------------------------------------

STRONG_BOOL(Unqualified);

std::string GetName(const QualType& t, const Unqualified unqualified = Unqualified::No);
//-----------------------------------------------------------------------------

std::string
GetTypeNameAsParameter(const QualType& t, const std::string& varName, const Unqualified unqualified = Unqualified::No);
//-----------------------------------------------------------------------------

const std::string EvaluateAsFloat(const FloatingLiteral& expr);
const std::string GetNoExcept(const FunctionDecl& decl);
const char*       GetConst(const FunctionDecl& decl);

}  // namespace clang::insights

#endif /* INSIGHTS_HELPERS_H */
