/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_STRCAT_H
#define INSIGHTS_STRCAT_H

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"

#include <string>
#include <utility>
//-----------------------------------------------------------------------------

namespace clang::insights {

static inline bool EndsWith(const std::string& src, const std::string& ending)
{
    if(ending.size() > src.size()) {
        return false;
    }

    return std::equal(ending.rbegin(), ending.rend(), src.rbegin());
}
//-----------------------------------------------------------------------------

static inline bool BeginsWith(const std::string& src, const std::string& begining)
{
    if(begining.size() > src.size()) {
        return false;
    }

    return std::equal(begining.begin(), begining.end(), src.begin());
}
//-----------------------------------------------------------------------------

static inline std::string ToString(const llvm::APSInt& val)
{
    return val.toString(10);
}
//-----------------------------------------------------------------------------

static inline uint64_t Normalize(const llvm::APInt& arg)
{
    return arg.getZExtValue();
}
//-----------------------------------------------------------------------------

static inline const std::string Normalize(const llvm::APSInt& arg)
{
    return ToString(arg);
}
//-----------------------------------------------------------------------------

static inline const std::string Normalize(StringRef&& arg)
{
    return arg.str();
}
//-----------------------------------------------------------------------------

static inline const std::string Normalize(const int& arg)
{
    return std::to_string(arg);
}
//-----------------------------------------------------------------------------

static inline const std::string Normalize(const unsigned int& arg)
{
    return std::to_string(arg);
}
//-----------------------------------------------------------------------------

static inline const std::string Normalize(const unsigned long& arg)
{
    return std::to_string(arg);
}
//-----------------------------------------------------------------------------

static inline const std::string Normalize(const unsigned long long& arg)
{
    return std::to_string(arg);
}
//-----------------------------------------------------------------------------

template<class T>
static inline const T& Normalize(const T& arg)
{
    return arg;
}
//-----------------------------------------------------------------------------

namespace details {
template<typename T, typename... Args>
void StrCat(std::string& ret, T&& t, Args&&... args)
{
    ret.append(Normalize(std::forward<T>(t)));

    if constexpr(0 < sizeof...(args)) {
        StrCat(ret, std::forward<Args>(args)...);
    }
}
//-----------------------------------------------------------------------------
}  // namespace details

template<typename T, typename... Args>
std::string StrCat(const T& first, Args&&... args)
{
    std::string ret{first};
    details::StrCat(ret, std::forward<Args>(args)...);

    return ret;
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_STRCAT_H */
