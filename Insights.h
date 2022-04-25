///////////////////////////////////////////////////////////////////////////////
//
// C++ Insights, copyright (C) by Andreas Fertig
// Distributed under an MIT license. See LICENSE for details
//
///////////////////////////////////////////////////////////////////////////////

#ifndef INSIGHTS_H
#define INSIGHTS_H
//-----------------------------------------------------------------------------

namespace clang {
class ASTContext;
class CompilerInstance;
}  // namespace clang
//-----------------------------------------------------------------------------

/// \brief Global C++ Insights command line options.
struct InsightsOptions
{
#define INSIGHTS_OPT(opt, name, deflt, description, category) bool name;
#include "InsightsOptions.def"
};
//-----------------------------------------------------------------------------

/// \brief Get the global C++ Insights options.
extern const InsightsOptions& GetInsightsOptions();
//-----------------------------------------------------------------------------

/// \brief Get access to the ASTContext
extern const clang::ASTContext& GetGlobalAST();
//-----------------------------------------------------------------------------

/// \brief Get access to the CompilerInstance
extern const clang::CompilerInstance& GetGlobalCI();
//-----------------------------------------------------------------------------

namespace clang::insights {
enum class GlobalInserts
{               // Headers go first
    HeaderNew,  //!< Track whether we have at least one local static variable in this TU. If so we need to insert the
                //!< <new> header for the placement-new.
    HeaderException,  //!< Track whether there was a noexcept transformation requireing the exception header.
    HeaderUtility,    //!< Track whether there was a std::move inserted.
    HeaderStddef,     //!< Track whether we need to insert <stddef.h> in Cfront mode
    HeaderAssert,     //!< Track whether we need to insert <assert.h> in Cfront mode

    // Now all the forward declared functions
    FuncCxaStart,
    FuncCxaAtExit,
    FuncMalloc,
    FuncFree,
    FuncMemset,
    FuncMemcpy,
    FuncCxaVecNew,
    FuncCxaVecCtor,
    FuncCxaVecDel,
    FuncCxaVecDtor,

    // The traditional enum element count
    MAX
};
//-----------------------------------------------------------------------------

void EnableGlobalInsert(GlobalInserts);
//-----------------------------------------------------------------------------
}  // namespace clang::insights

#endif /* INSIGHTS_H */
