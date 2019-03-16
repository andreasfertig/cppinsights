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

void InsightsBase::InsertIndentedText(SourceLocation loc, OutputFormatHelper& outputFormatHelper)
{
    mRewrite.InsertText(loc, outputFormatHelper.GetString(), true, true);
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
