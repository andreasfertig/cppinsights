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

inline constexpr bool IsClangNewerThan7 = IsClangNewerThan<7>::value;
//-----------------------------------------------------------------------------

template<typename T>
auto inline GetBeginLoc(const T& decl)
{
    if constexpr(IsClangNewerThan7) {
        return decl.getBeginLoc();
    } else {
        return decl.getLocStart();
    }
}
//-----------------------------------------------------------------------------

template<typename T>
auto inline GetBeginLoc(const T* decl)
{
    if constexpr(IsClangNewerThan7) {
        return decl->getBeginLoc();
    } else {
        return decl->getLocStart();
    }
}
//-----------------------------------------------------------------------------

template<typename T>
auto inline GetEndLoc(const T& decl)
{
    if constexpr(IsClangNewerThan7) {
        return decl.getEndLoc();
    } else {
        return decl.getLocEnd();
    }
}
//-----------------------------------------------------------------------------

template<typename T>
auto inline GetEndLoc(const T* decl)
{
    if constexpr(IsClangNewerThan7) {
        return decl->getEndLoc();
    } else {
        return decl->getLocEnd();
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_CLANG_COMPAT_H */
