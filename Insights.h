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
}
//-----------------------------------------------------------------------------

/// \brief Global C++ Insights command line options.
struct InsightsOptions
{
#define INSIGHTS_OPT(opt, name, deflt, description) bool name;
#include "InsightsOptions.def"
};
//-----------------------------------------------------------------------------

/// \brief Get the global C++ Insights options.
extern const InsightsOptions& GetInsightsOptions();
//-----------------------------------------------------------------------------

/// \brief Get access to the ASTContext
extern const clang::ASTContext& GetGlobalAST();
//-----------------------------------------------------------------------------

#endif /* INSIGHTS_H */
