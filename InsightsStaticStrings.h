/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_STATIC_STRINGS_H
#define INSIGHTS_STATIC_STRINGS_H

#define KW_CONST "const"
#define KW_AUTO "auto"
#define KW_CONSTEXPR "constexpr"
#define KW_VOLATILE "volatile"
#define KW_STATIC "static"
#define KW_EXTERN "extern"
#define KW_NOEXCEPT "noexcept"
#define KW_CLASS "class"
#define KW_VIRTUAL "virtual"
#define KW_INLINE "inline"
//-----------------------------------------------------------------------------

// static constexpr const char kwConst[]     = KW_CONST;
static constexpr const char kwAuto[]      = KW_AUTO;
static constexpr const char kwConstExpr[] = KW_CONSTEXPR;
// static constexpr const char kwVolatile[]  = KW_VOLATILE;
static constexpr const char kwStatic[] = KW_STATIC;
static constexpr const char kwExtern[] = KW_EXTERN;
// static constexpr const char kwNoexcept[]  = KW_NOEXCEPT;
// static constexpr const char kwClass[]     = KW_CLASS;
// static constexpr const char kwVirtual[] = KW_VIRTUAL;
// static constexpr const char kwInline[] = KW_INLINE;
//-----------------------------------------------------------------------------

#define BUILD_WITH_SPACE_AFTER(kw) kw " "
#define BUILD_WITH_SPACE_BEFORE(kw) " " kw

static constexpr const char kwConstSpace[] = BUILD_WITH_SPACE_AFTER(KW_CONST);
// static constexpr const char kwAutoSpace[]      = BUILD_WITH_SPACE_AFTER(KW_AUTO);
static constexpr const char kwConstExprSpace[] = BUILD_WITH_SPACE_AFTER(KW_CONSTEXPR);
static constexpr const char kwStaticSpace[]    = BUILD_WITH_SPACE_AFTER(KW_STATIC);
static constexpr const char kwExternSpace[]    = BUILD_WITH_SPACE_AFTER(KW_EXTERN);
// static constexpr const char kwNoexceptSpace[]  = BUILD_WITH_SPACE_AFTER(KW_NOEXCEPT);
static constexpr const char kwClassSpace[]   = BUILD_WITH_SPACE_AFTER(KW_CLASS);
static constexpr const char kwVirtualSpace[] = BUILD_WITH_SPACE_AFTER(KW_VIRTUAL);
static constexpr const char kwInlineSpace[]  = BUILD_WITH_SPACE_AFTER(KW_INLINE);
//-----------------------------------------------------------------------------

static constexpr const char kwSpaceNoexcept[]  = BUILD_WITH_SPACE_BEFORE(KW_NOEXCEPT);
static constexpr const char kwSpaceConst[]     = BUILD_WITH_SPACE_BEFORE(KW_CONST);
static constexpr const char kwSpaceConstExpr[] = BUILD_WITH_SPACE_BEFORE(KW_CONSTEXPR);
static constexpr const char kwSpaceVolatile[]  = BUILD_WITH_SPACE_BEFORE(KW_VOLATILE);
//-----------------------------------------------------------------------------

#endif /* INSIGHTS_STATIC_STRINGS_H */
