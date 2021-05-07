/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_UTILITY_H
#define INSIGHTS_UTILITY_H

#include <utility>
//-----------------------------------------------------------------------------

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)
//-----------------------------------------------------------------------------

#endif /* INSIGHTS_UTILITY_H */
