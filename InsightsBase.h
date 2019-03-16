/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_BASE_H
#define INSIGHTS_BASE_H
//-----------------------------------------------------------------------------

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <unordered_map>

#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

namespace clang::insights {
class InsightsBase
{
protected:
    Rewriter& mRewrite;

    explicit InsightsBase(Rewriter& rewriter)
    : mRewrite{rewriter}
    , mMap{}
    {
    }

protected:
    void InsertIndentedText(SourceLocation loc, OutputFormatHelper& outputFormatHelper);

private:
    std::unordered_map<intptr_t, bool> mMap;
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_BASE_H */
