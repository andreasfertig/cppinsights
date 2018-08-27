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
//-----------------------------------------------------------------------------

namespace clang::insights {

#if CLANG_VERSION_MAJOR <= 7
template<typename T>
auto inline GetBeginLoc(const T& decl) -> decltype(decl.getLocStart())
{
    return decl.getLocStart();
}

template<typename T>
auto inline GetBeginLoc(const T* decl) -> decltype(decl->getLocStart())
{
    return decl->getLocStart();
}

template<typename T>
auto inline GetEndLoc(const T& decl) -> decltype(decl.getLocEnd())
{
    return decl.getLocEnd();
}

template<typename T>
auto inline GetEndLoc(const T* decl) -> decltype(decl->getLocEnd())
{
    return decl->getLocEnd();
}

#else
template<typename T>
auto inline GetBeginLoc(const T& decl) -> decltype(decl.getBeginLoc())
{
    return decl.getBeginLoc();
}

template<typename T>
auto inline GetBeginLoc(const T* decl) -> decltype(decl->getBeginLoc())
{
    return decl->getBeginLoc();
}

template<typename T>
auto inline GetEndLoc(const T& decl) -> decltype(decl.getEndLoc())
{
    return decl.getEndLoc();
}

template<typename T>
auto inline GetEndLoc(const T* decl) -> decltype(decl->getEndLoc())
{
    return decl->getEndLoc();
}

#endif
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_CLANG_COMPAT_H */
