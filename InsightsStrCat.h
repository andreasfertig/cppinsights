/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_STRCAT_H
#define INSIGHTS_STRCAT_H

#include "clang/AST/AST.h"

#include <string>
#include <type_traits>
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

static inline bool BeginsWith(const std::string& src, const std::string& beginning)
{
    if(beginning.size() > src.size()) {
        return false;
    }

    return std::equal(beginning.begin(), beginning.end(), src.begin());
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

static inline std::string Normalize(const llvm::APSInt& arg)
{
    return ToString(arg);
}
//-----------------------------------------------------------------------------

static inline std::string Normalize(StringRef&& arg)
{
    return arg.str();
}
//-----------------------------------------------------------------------------

template<class T>
struct remove_cvref
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template<class T>
using remove_cvref_t = typename remove_cvref<T>::type;
//-----------------------------------------------------------------------------

template<class T>
static inline decltype(auto) Normalize(const T& arg)
{
    if constexpr(std::is_integral_v<T> && not std::is_same_v<remove_cvref_t<T>, bool>) {
        return std::to_string(arg);
    } else {
        return (arg);
    }
}
//-----------------------------------------------------------------------------

namespace details {
template<typename... Args>
void StrCat(std::string& ret, Args&&... args)
{
    (ret += ... += ::clang::insights::Normalize(std::forward<Args>(args)));
}
//-----------------------------------------------------------------------------
}  // namespace details

template<typename... Args>
std::string StrCat(Args&&... args)
{
    std::string ret{};
    details::StrCat(ret, std::forward<Args>(args)...);

    return ret;
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_STRCAT_H */
