/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_STRCAT_H
#define INSIGHTS_STRCAT_H

#include "clang/AST/AST.h"
#include "llvm/ADT/StringExtras.h"

#include <string>
#include <type_traits>
#include <utility>

#include "ClangCompat.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

namespace details {
/// \brief Convert a boolean value to a string representation of "true" or "false"
inline std::string ConvertToBoolString(bool b)
{
    return b ? std::string{"true"} : std::string{"false"};
}

}  // namespace details

inline std::string ToString(const llvm::APSInt& val)
{
    if(1 == val.getBitWidth()) {
        return details::ConvertToBoolString(0 != val.getExtValue());
    }

    return llvm::toString(val, 10);
}
//-----------------------------------------------------------------------------

inline uint64_t Normalize(const llvm::APInt& arg)
{
    return arg.getZExtValue();
}
//-----------------------------------------------------------------------------

inline std::string Normalize(const llvm::APSInt& arg)
{
    return ToString(arg);
}
//-----------------------------------------------------------------------------

inline std::string Normalize(const APValue& arg)
{
    switch(arg.getKind()) {
        case APValue::Int: return Normalize(arg.getInt());
        case APValue::Float: {
            std::string                str{};
            ::llvm::raw_string_ostream stream{str};

            arg.getFloat().print(stream);
            str.pop_back();

            if(std::string::npos == str.find('.')) {
                /* in case it is a number like 10.0 toString() seems to leave out the .0. However, as this distinguished
                 * between an integer and a floating point literal we need that dot. */
                str.append(".0");
            }

            return str;
        }

        default: break;
    }

    return std::string{"unsupported APValue"};
}
//-----------------------------------------------------------------------------

inline std::string_view Normalize(const StringRef& arg)
{
    return arg;
}
//-----------------------------------------------------------------------------

static inline std::string Normalize(const CharUnits& arg)
{
    return std::to_string(arg.getQuantity());
}
//-----------------------------------------------------------------------------

template<class T>
inline decltype(auto) Normalize(const T& arg)
{
    // Handle bool's first, we like their string representation.
    if constexpr(std::is_same_v<std::remove_cvref_t<T>, bool>) {
        return details::ConvertToBoolString(arg);

    } else if constexpr(std::is_integral_v<T>) {
        return std::to_string(arg);

    } else {
        return (arg);
    }
}
//-----------------------------------------------------------------------------

namespace details {
void StrCat(std::string& ret, const auto&... args)
{
    (ret += ... += ::clang::insights::Normalize(args));
}
//-----------------------------------------------------------------------------
}  // namespace details

inline std::string StrCat(const auto&... args)
{
    std::string ret{};
    details::StrCat(ret, args...);

    return ret;
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_STRCAT_H */
