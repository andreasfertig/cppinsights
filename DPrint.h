/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_DPRINT_H
#define INSIGHTS_DPRINT_H

#include "InsightsStrCat.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

namespace details {

static inline const char* Normalize(const std::string& arg)
{
    static constexpr const char emptyString[]{""};

    if(arg.empty()) {
        return emptyString;
    }

    return arg.c_str();
}
//-----------------------------------------------------------------------------

static inline uint64_t Normalize(const llvm::APInt& arg)
{
    return arg.getZExtValue();
}
//-----------------------------------------------------------------------------

static inline const char* Normalize(const llvm::APSInt& arg)
{
    return Normalize(ToString(arg));
}
//-----------------------------------------------------------------------------

static inline const char* Normalize(const StringRef& arg)
{
    return Normalize(arg.str());
}
//-----------------------------------------------------------------------------

template<class T>
static inline const T& Normalize(const T& arg)
{
    return arg;
}
//-----------------------------------------------------------------------------

template<typename... Args>
inline void FPrintf(const char* fmt, const Args&... args)
{
    if constexpr(0 < (sizeof...(args))) {
        fprintf(stderr, fmt, Normalize(args)...);
    } else {
        fprintf(stderr, "%s", fmt);
    }
}
//-----------------------------------------------------------------------------

}  // namespace details

/// \brief Debug print which is disabled in release-mode.
///
/// It takes a variable number of parameters which are normalized if they are a \ref std::string or a \ref StringRef.
template<typename... Args>
inline void DPrint([[maybe_unused]] const char* fmt, [[maybe_unused]] const Args&... args)
{
#ifdef INSIGHTS_DEBUG
    details::FPrintf(fmt, args...);
#endif /* INSIGHTS_DEBUG */
}
//-----------------------------------------------------------------------------

/// \brief Log an error.
template<typename... Args>
inline void Error(const char* fmt, const Args&... args)
{
    details::FPrintf(fmt, args...);
}
//-----------------------------------------------------------------------------

template<typename T>
inline void Dump([[maybe_unused]] const T* stmt)
{
#ifdef INSIGHTS_DEBUG
    if(stmt) {
        stmt->dump();
    }
#endif /* INSIGHTS_DEBUG */
}
//-----------------------------------------------------------------------------

/// \brief Log an error.
///
/// In debug-mode this dumps the \ref Decl which caused the error and the error message.
template<typename... Args>
inline void Error(const Decl* stmt, const char* fmt, const Args&... args)
{
    if(stmt) {
        Dump(stmt);
    }

    Error(fmt, args...);
}
//-----------------------------------------------------------------------------

/// \brief Log an error.
///
/// In debug-mode this dumps the \ref Stmt which caused the error and the error message.
template<typename... Args>
inline void Error(const Stmt* stmt, const char* fmt, const Args&... args)
{
    if(stmt) {
        Dump(stmt);
    }

    Error(fmt, args...);
}
//-----------------------------------------------------------------------------

/// \brief Helper function to generate TODO comments for an unsupported \ref Stmt.
void ToDo(const class Stmt* stmt, class OutputFormatHelper& outputFormatHelper, std::string_view file, const int line);
/// \brief Helper function to generate TODO comments for an unsupported \ref Decl.
void ToDo(const class Decl* stmt, class OutputFormatHelper& outputFormatHelper, std::string_view file, const int line);
//-----------------------------------------------------------------------------

/// \brief Convenience marco to get file-name and line-number for better analysis.
#define TODO(stmt, outputFormatHelper) ToDo(stmt, outputFormatHelper, __FILE__, __LINE__)

}  // namespace clang::insights

#endif /* INSIGHTS_DPRINT_H */
