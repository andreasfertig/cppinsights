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
#include "StackList.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

static inline bool IsNewLine(const char c)
{
    return '\n' == c;
}
//-----------------------------------------------------------------------------

std::string BuildInternalVarName(const std::string& varName);
//-----------------------------------------------------------------------------

STRONG_BOOL(RequireSemi);
//-----------------------------------------------------------------------------

SourceLocation FindLocationAfterSemi(const SourceLocation                          loc,
                                     const ast_matchers::MatchFinder::MatchResult& Result,
                                     RequireSemi                                   requireSemi = RequireSemi::No);
SourceRange    GetSourceRangeAfterSemi(const SourceRange                             range,
                                       const ast_matchers::MatchFinder::MatchResult& Result,
                                       RequireSemi                                   requireSemi = RequireSemi::No);
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

std::string BuildRetTypeName(const Decl& decl);
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

static inline bool Contains(const std::string& source, const std::string& search)
{
    return std::string::npos != source.find(search, 0);
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
std::string GetPlainName(const DeclRefExpr& DRE);

std::string GetName(const DeclRefExpr& DRE);
std::string GetName(const VarDecl& VD);
//-----------------------------------------------------------------------------

std::string GetName(const NamedDecl& ND);
//-----------------------------------------------------------------------------

std::string GetNameAsFunctionPointer(const QualType& t);
//-----------------------------------------------------------------------------

std::string GetLambdaName(const CXXRecordDecl& lambda);

static inline std::string GetLambdaName(const LambdaExpr& lambda)
{
    return GetLambdaName(*lambda.getLambdaClass());
}
//-----------------------------------------------------------------------------

std::string GetName(const CXXRecordDecl& RD);
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

std::string GetUnqualifiedScopelessName(const Type* type);
//-----------------------------------------------------------------------------

std::string
GetTypeNameAsParameter(const QualType& t, const std::string& varName, const Unqualified unqualified = Unqualified::No);
//-----------------------------------------------------------------------------

std::string GetNestedName(const NestedNameSpecifier* nns);
std::string GetDeclContext(const DeclContext* ctx);
//-----------------------------------------------------------------------------

const std::string EvaluateAsFloat(const FloatingLiteral& expr);
const std::string GetNoExcept(const FunctionDecl& decl);
const char*       GetConst(const FunctionDecl& decl);
//-----------------------------------------------------------------------------

std::string GetElaboratedTypeKeyword(const ElaboratedTypeKeyword keyword);
//-----------------------------------------------------------------------------

template<typename T, typename TFunc>
void for_each(T start, T end, TFunc&& func)
{
    for(; start < end; ++start) {
        func(start);
    }
}
//-----------------------------------------------------------------------------

/// \brief Track the scope we are currently in to build a properly scoped variable.
///
/// The AST only knows about absolute scopes (namespace, struct, class), as once a declaration is parsed it is either in
/// a scope or not. Each request to give me the namespace automatically leads to the entire scope the item is in. This
/// makes it hard to have constructs like this: \code struct One
/// {
///    static const int o{};
///
///    struct Two
///    {
///        static const int d = o;
///    };
///
///    static const int a = Two::d;
///};
/// \endcode
///
/// Here the initializer of \c a is in the scope \c One::Two::d. At this point the qualification \c Two::d is enought.
///
/// \c ScopeHelper tracks whether we are currently in a class or namespace and simply remove the path we already in from
/// the scope.
struct ScopeHelper : public StackListEntry<ScopeHelper>
{
    ScopeHelper(const size_t len)
    : mLength{len}
    {
    }

    const size_t mLength;  //!< Length of the scope as it was _before_ this declaration was appended.
};
//-----------------------------------------------------------------------------

/// \brief The ScopeHandler tracks the current scope.
///
/// The \c ScopeHandler tracks the current scope, knows about all the parts and is able to remove the current scope part
/// from a name.
class ScopeHandler
{
public:
    ScopeHandler(const Decl* d);

    ~ScopeHandler();

    /// \brief Remove the current scope from a string.
    ///
    /// The default is that the entire scope is replaced. Suppose we are
    /// currently in N::X and having a symbol N::X::y then N::X:: is removed. However, there is a special case, where
    /// the last item is skipped.
    static std::string RemoveCurrentScope(std::string name);

private:
    using ScopeStackType = StackList<ScopeHelper>;

    ScopeStackType& mStack;   //!< Access to the global \c ScopeHelper stack.
    ScopeHelper     mHelper;  //!< The \c ScopeHelper this item refers to.

    static ScopeStackType mGlobalStack;  //!< Global stack to keep track of the scope elements.
    static std::string    mScope;        //!< The entire scope we are already in.
};
//-----------------------------------------------------------------------------

/// \brief Helper to create a \c ScopeHandler on the stack which adds the current \c Decl to it and removes it once the
/// scope is left.
#define SCOPE_HELPER(d)                                                                                                \
    ScopeHandler _scopeHandler { d }
//-----------------------------------------------------------------------------

/// \brief Specialization for \c ::llvm::raw_string_ostream with an internal \c std::string buffer.
///
class StringStream : public ::llvm::raw_string_ostream
{
private:
    std::string mData;

public:
    StringStream()
    : ::llvm::raw_string_ostream{mData}
    {
    }
};

}  // namespace clang::insights

#endif /* INSIGHTS_HELPERS_H */
