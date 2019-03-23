/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_BASE_H
#define INSIGHTS_BASE_H
//-----------------------------------------------------------------------------

#include <stdint.h>       // for intptr_t
#include <unordered_map>  // for unordered_map
namespace clang {
class Rewriter;
}
namespace clang {
class SourceLocation;
}
namespace clang {
namespace insights {
class OutputFormatHelper;
}
}  // namespace clang
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
