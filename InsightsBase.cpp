/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "InsightsBase.h"
#include "DPrint.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

bool InsightsBase::SkipIfAlreadySeen(const Stmt* stmt)
{
    const intptr_t addr{reinterpret_cast<intptr_t>(stmt)};
    if(mMap.find(addr) != mMap.end()) {
        DPrint("duplicate\n");
        return true;
    }

    mMap[addr] = true;

    return false;
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
