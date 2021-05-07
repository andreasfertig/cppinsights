/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_CLANG_COMPAT_H
#define INSIGHTS_CLANG_COMPAT_H
//-----------------------------------------------------------------------------

#include <clang/Basic/Version.h>
#include "version.h"
//-----------------------------------------------------------------------------

#define IS_CLANG_NEWER_THAN(major) CLANG_VERSION_MAJOR > (major)
//-----------------------------------------------------------------------------

namespace clang::insights {

template<unsigned int MAJOR>
struct IsClangNewerThan
{
    static_assert(INSIGHTS_MIN_LLVM_MAJOR_VERSION < MAJOR, "Remove this function, all clang versions support it now");
    constexpr static inline bool value{CLANG_VERSION_MAJOR > MAJOR};
};
//-----------------------------------------------------------------------------

// inline constexpr bool IsClangNewerThan8 = IsClangNewerThan<8>::value;
//-----------------------------------------------------------------------------

inline auto* GetTemporary(const MaterializeTemporaryExpr* stmt)
{
    return stmt->getSubExpr();
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_CLANG_COMPAT_H */
