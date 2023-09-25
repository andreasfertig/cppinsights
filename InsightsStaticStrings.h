/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_STATIC_STRINGS_H
#define INSIGHTS_STATIC_STRINGS_H

#include <string_view>
using namespace std::literals;
//-----------------------------------------------------------------------------

#define KW_CONST "const"
#define KW_AUTO "auto"
#define KW_CONSTEXPR "constexpr"
#define KW_CONSTEVAL "consteval"
#define KW_CONSTINIT "constinit"
#define KW_VOLATILE "volatile"
#define KW_STATIC "static"
#define KW_EXTERN "extern"
#define KW_NOEXCEPT "noexcept"
#define KW_CLASS "class"
#define KW_STRUCT "struct"
#define KW_UNION "union"
#define KW_VIRTUAL "virtual"
#define KW_INLINE "inline"
#define KW_REQUIRES "requires"
#define KW_REINTERPRET_CAST "reinterpret_cast"
#define KW_STATIC_CAST "static_cast"
#define KW_PUBLIC "public"
#define KW_PROTECTED "protected"
#define KW_PRIVATE "private"
#define KW_FINAL "final"
#define KW_SIZEOF "sizeof"
#define KW_ALIGNOF "alignof"
#define KW_OPERATOR "operator"
#define KW_USING "using"
#define KW_FRIEND "friend"
#define KW_EXPLICIT "explicit"
#define KW_TEMPLATE "template"
#define KW_NULLPTR "nullptr"
#define KW_NULL "NULL"
#define KW_ENUM "enum"
#define KW_NAMESPACE "namespace"
#define KW_CONCEPT "concept"
#define KW_DELETE "delete"
#define KW_WHILE "while"
#define KW_BREAK "break"
#define KW_DO "do"
#define KW_CASE "case"
#define KW_GOTO "goto"
#define KW_CONTINUE "continue"
#define KW_SWITCH "switch"
#define KW_TYPEID "typeid"
#define KW_MUTABLE "mutable"
#define KW_RETURN "return"
#define KW_STATIC_ASSERT "static_assert"
#define KW_CO_RETURN "co_return"
#define KW_CO_YIELD "co_yield"
#define KW_CO_AWAIT "co_await"
#define KW_TYPENAME "typename"
#define KW_EQUALS_DEFAULT "= default;"
#define KW_EQUALS_DELETE "= delete;"
#define KW_INTERNAL_THIS "__this"
#define KW_ELIPSIS "..."
#define KW_FALSE "false"
#define KW_TRY "try"

#define HLP_ASSIGN " = "
#define HLP_ARROW " -> "
//-----------------------------------------------------------------------------

#define BUILD_KW(kw) " " kw " "

inline constexpr std::string_view KW_OPERATOR_ALL{BUILD_KW(KW_OPERATOR)};

consteval std::string_view KwWithSpaceBefore(std::string_view kw)
{
    kw.remove_suffix(1);

    return kw;
}

consteval std::string_view KwWithSpaceAfter(std::string_view kw)
{
    kw.remove_prefix(1);

    return kw;
}

consteval std::string_view KwWithNoSpace(std::string_view kw)
{
    kw.remove_suffix(1);
    kw.remove_prefix(1);

    return kw;
}

inline constexpr std::string_view kwNoexcept{KW_NOEXCEPT};
inline constexpr std::string_view kwRequires{KW_REQUIRES};
inline constexpr std::string_view kwPublic{KW_PUBLIC};
inline constexpr std::string_view kwProtected{KW_PROTECTED};
inline constexpr std::string_view kwPrivate{KW_PRIVATE};
inline constexpr std::string_view kwReinterpretCast{KW_REINTERPRET_CAST};
inline constexpr std::string_view kwStaticCast{KW_STATIC_CAST};
inline constexpr std::string_view kwSizeof{KW_SIZEOF};
inline constexpr std::string_view kwAlignof{KW_ALIGNOF};
inline constexpr std::string_view kwUnkown{"unkown"sv};
inline constexpr std::string_view kwOperator{KwWithNoSpace(KW_OPERATOR_ALL)};
// inline constexpr std::string_view kwOperator{KW_OPERATOR};
inline constexpr std::string_view kwTemplate{KW_TEMPLATE};
inline constexpr std::string_view kwCommentStart{"/*"sv};
inline constexpr std::string_view kwNullptr{KW_NULLPTR};
inline constexpr std::string_view kwNull{KW_NULL};
inline constexpr std::string_view kwNamespace{KW_NAMESPACE};
inline constexpr std::string_view kwDelete{KW_DELETE};
inline constexpr std::string_view kwWhile{KW_WHILE};
inline constexpr std::string_view kwBreak{KW_BREAK};
inline constexpr std::string_view kwContinue{KW_CONTINUE};
inline constexpr std::string_view kwSwitch{KW_SWITCH};
inline constexpr std::string_view kwTypeId{KW_TYPEID};
inline constexpr std::string_view kwReturn{KW_RETURN};
inline constexpr std::string_view kwFalse{KW_FALSE};
inline constexpr std::string_view kwElipsis{KW_ELIPSIS};
inline constexpr std::string_view kwStaticAssert{KW_STATIC_ASSERT};
inline constexpr std::string_view kwInternalThis{KW_INTERNAL_THIS};
inline constexpr std::string_view kwThis{KwWithSpaceAfter(KwWithSpaceAfter(kwInternalThis))};

inline constexpr std::string_view hlpAssing{HLP_ASSIGN};
inline constexpr std::string_view hlpArrow{HLP_ARROW};
inline constexpr std::string_view hlpResumeFn{"resume_fn"sv};
inline constexpr std::string_view hlpDestroyFn{"destroy_fn"sv};
//-----------------------------------------------------------------------------

#define BUILD_WITH_SPACE_AFTER(kw) kw " "
#define BUILD_WITH_SPACE_BEFORE(kw) " " kw

inline constexpr std::string_view kwConstExprSpace{BUILD_WITH_SPACE_AFTER(KW_CONSTEXPR)};
inline constexpr std::string_view kwConstEvalSpace{BUILD_WITH_SPACE_AFTER(KW_CONSTEVAL)};
inline constexpr std::string_view kwStaticSpace{BUILD_WITH_SPACE_AFTER(KW_STATIC)};
inline constexpr std::string_view kwClassSpace{BUILD_WITH_SPACE_AFTER(KW_CLASS)};
inline constexpr std::string_view kwStructSpace{BUILD_WITH_SPACE_AFTER(KW_STRUCT)};
inline constexpr std::string_view kwEnumSpace{BUILD_WITH_SPACE_AFTER(KW_ENUM)};
inline constexpr std::string_view kwUnionSpace{BUILD_WITH_SPACE_AFTER(KW_UNION)};
inline constexpr std::string_view kwVirtualSpace{BUILD_WITH_SPACE_AFTER(KW_VIRTUAL)};
inline constexpr std::string_view kwInlineSpace{BUILD_WITH_SPACE_AFTER(KW_INLINE)};
inline constexpr std::string_view kwRequiresSpace{BUILD_WITH_SPACE_AFTER(KW_REQUIRES)};
inline constexpr std::string_view kwOperatorSpace{KwWithSpaceAfter(KW_OPERATOR_ALL)};
inline constexpr std::string_view kwCppCommentStartSpace{BUILD_WITH_SPACE_AFTER("//")};
inline constexpr std::string_view kwUsingSpace{BUILD_WITH_SPACE_AFTER(KW_USING)};
inline constexpr std::string_view kwFriendSpace{BUILD_WITH_SPACE_AFTER(KW_FRIEND)};
inline constexpr std::string_view kwExplicitSpace{BUILD_WITH_SPACE_AFTER(KW_EXPLICIT)};
inline constexpr std::string_view kwTemplateSpace{BUILD_WITH_SPACE_AFTER(KW_TEMPLATE)};
inline constexpr std::string_view kwNamespaceSpace{BUILD_WITH_SPACE_AFTER(KW_NAMESPACE)};
inline constexpr std::string_view kwConceptSpace{BUILD_WITH_SPACE_AFTER(KW_CONCEPT)};
inline constexpr std::string_view kwDoSpace{BUILD_WITH_SPACE_AFTER(KW_DO)};
inline constexpr std::string_view kwCaseSpace{BUILD_WITH_SPACE_AFTER(KW_CASE)};
inline constexpr std::string_view kwGotoSpace{BUILD_WITH_SPACE_AFTER(KW_GOTO)};
inline constexpr std::string_view kwMutableSpace{BUILD_WITH_SPACE_AFTER(KW_MUTABLE)};
inline constexpr std::string_view kwCoReturnSpace{BUILD_WITH_SPACE_AFTER(KW_CO_RETURN)};
inline constexpr std::string_view kwCoYieldSpace{BUILD_WITH_SPACE_AFTER(KW_CO_YIELD)};
inline constexpr std::string_view kwCoAwaitSpace{BUILD_WITH_SPACE_AFTER(KW_CO_AWAIT)};
inline constexpr std::string_view kwTypeNameSpace{BUILD_WITH_SPACE_AFTER(KW_TYPENAME)};
inline constexpr std::string_view kwCCommentStartSpace{BUILD_WITH_SPACE_AFTER("/*")};
inline constexpr std::string_view kwCCommentEndSpace{BUILD_WITH_SPACE_AFTER("*/")};
inline constexpr std::string_view kwElipsisSpace{BUILD_WITH_SPACE_AFTER(KW_ELIPSIS)};
inline constexpr std::string_view kwTrySpace{BUILD_WITH_SPACE_AFTER(KW_TRY)};
//-----------------------------------------------------------------------------

inline constexpr std::string_view kwSpaceNoexcept{BUILD_WITH_SPACE_BEFORE(KW_NOEXCEPT)};
inline constexpr std::string_view kwSpaceConst{BUILD_WITH_SPACE_BEFORE(KW_CONST)};
inline constexpr std::string_view kwSpaceConstExpr{BUILD_WITH_SPACE_BEFORE(KW_CONSTEXPR)};
inline constexpr std::string_view kwSpaceVolatile{BUILD_WITH_SPACE_BEFORE(KW_VOLATILE)};
inline constexpr std::string_view kwSpaceFinal{BUILD_WITH_SPACE_BEFORE(KW_FINAL)};
inline constexpr std::string_view kwSpaceEqualsDefault{BUILD_WITH_SPACE_BEFORE(KW_EQUALS_DEFAULT)};
inline constexpr std::string_view kwSpaceEqualsDelete{BUILD_WITH_SPACE_BEFORE(KW_EQUALS_DELETE)};
inline constexpr std::string_view kwSpaceCCommentEnd{BUILD_WITH_SPACE_BEFORE("*/")};
//-----------------------------------------------------------------------------

inline constexpr std::string_view kwSpaceCCommentEndSpace{BUILD_WITH_SPACE_BEFORE("*/ ")};
inline constexpr std::string_view kwSpaceConstEvalSpace{BUILD_WITH_SPACE_BEFORE(BUILD_WITH_SPACE_AFTER(KW_CONSTEVAL))};

inline constexpr std::string_view memberVariablePointerPrefix{"MemberVarPtr_"};
inline constexpr std::string_view functionPointerPrefix{"FuncPtr_"};
//-----------------------------------------------------------------------------

#endif /* INSIGHTS_STATIC_STRINGS_H */
