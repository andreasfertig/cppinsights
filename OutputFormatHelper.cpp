/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

static const std::string MakeIndent(const int indent)
{
    std::string str{};

    for(int i = 1; i < indent; i++) {
        str += ' ';
    }

    return str;
}
//-----------------------------------------------------------------------------

void OutputFormatHelper::Indent(unsigned count)
{
    mOutput.append(MakeIndent(count + 1));
}
//-----------------------------------------------------------------------------

void OutputFormatHelper::AppendParameterList(const ArrayRef<ParmVarDecl*> parameters,
                                             const WithParameterName      withParameterName)
{
    ForEachArg(parameters, [&](const auto& p) {
        const auto& type = p->getType();

        if(WithParameterName::Yes == withParameterName) {
            const auto& name{GetName(*p)};

            Append(GetTypeNameAsParameter(type, name));
        } else {
            Append(GetName(type));
        }
    });
}
//-----------------------------------------------------------------------------

void OutputFormatHelper::CloseScope(const NoNewLineBefore newLineBefore)
{
    if(NoNewLineBefore::No == newLineBefore) {
        NewLine();
    }

    /* After a newline we are already indented by one level to much. Try to decrease it. */
    if(0 != mDefaultIndent) {
        for(unsigned i = 0; i < SCOPE_INDENT; ++i) {
            if(' ' != mOutput.back()) {
                break;
            }

            mOutput.pop_back();
        }
    }

    Append('}');

    DecreaseIndent();
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
