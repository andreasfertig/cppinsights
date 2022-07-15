/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_BASE_H
#define INSIGHTS_BASE_H
//-----------------------------------------------------------------------------

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
    {
    }

protected:
    void InsertIndentedText(SourceLocation loc, OutputFormatHelper& outputFormatHelper);
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_BASE_H */
