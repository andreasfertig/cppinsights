/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "OutputFormatHelper.h"
#include "CodeGenerator.h"
#include "InsightsHelpers.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

static const std::string MakeIndent(const unsigned indent)
{
    std::string str{};

    for(unsigned i = 1; i < indent; i++) {
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

void OutputFormatHelper::AppendParameterList(const ArrayRef<ParmVarDecl*> parameters, const NameOnly nameOnly)
{
    ForEachArg(parameters, [&](const auto& p) {
        const auto& name{GetName(*p)};

        // Get the attributes and insert them, if there are any
        CodeGenerator codeGenerator{*this};
        codeGenerator.InsertAttributes(p->attrs());

        if(NameOnly::No == nameOnly) {
            const auto& type{p->getType()};

            Append(GetTypeNameAsParameter(type, name));
        } else {
            Append(name);
        }
    });
}
//-----------------------------------------------------------------------------

void OutputFormatHelper::CloseScope(const NoNewLineBefore newLineBefore)
{
    if(NoNewLineBefore::No == newLineBefore) {
        NewLine();
    }

    RemoveIndent();

    Append('}');

    DecreaseIndent();
}
//-----------------------------------------------------------------------------

void OutputFormatHelper::RemoveIndent()
{
    /* After a newline we are already indented by one level to much. Try to decrease it. */
    if(0 != mDefaultIndent) {
        for(unsigned i = 0; i < SCOPE_INDENT; ++i) {
            if(' ' != mOutput.back()) {
                break;
            }

            mOutput.pop_back();
        }
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
