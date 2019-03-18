///////////////////////////////////////////////////////////////////////////////
//
// C++ Insights, copyright (C) by Andreas Fertig
// Distributed under an MIT license. See LICENSE for details
//
///////////////////////////////////////////////////////////////////////////////

#ifndef INSIGHTS_H
#define INSIGHTS_H
//-----------------------------------------------------------------------------

/// \brief Global C++ Insights command line options.
struct InsightsOptions
{
#define INSIGHTS_OPT(opt, name, deflt, description) bool name;
#include "InsightsOptions.def"
};
//-----------------------------------------------------------------------------

/// \brief Get the global C++ Insights options.
const InsightsOptions& GetInsightsOptions();
//-----------------------------------------------------------------------------

#endif /* INSIGHTS_H */
