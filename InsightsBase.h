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

#include "InsightsStrongTypes.h"
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

    STRONG_BOOL(SkipConstexpr);

public:
    static void GenerateFunctionPrototype(OutputFormatHelper& outputFormatHelper, const FunctionDecl& FD);

protected:
    bool SkipIfAlreadySeen(const Stmt* stmt);

private:
    std::unordered_map<intptr_t, bool> mMap;
};
//-----------------------------------------------------------------------------

#define SKIP_IF_ALREADY_SEEN(expr)                                                                                     \
    if(SkipIfAlreadySeen(expr)) {                                                                                      \
        return;                                                                                                        \
    }
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_BASE_H */
