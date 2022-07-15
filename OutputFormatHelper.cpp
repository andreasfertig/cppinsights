/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include <algorithm>

#include "CodeGenerator.h"
#include "InsightsHelpers.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

void OutputFormatHelper::Indent(unsigned count)
{
    mOutput.insert(mOutput.size(), count, ' ');
}
//-----------------------------------------------------------------------------

void OutputFormatHelper::AppendParameterList(const ArrayRef<ParmVarDecl*> parameters,
                                             const NameOnly               nameOnly,
                                             const GenMissingParamName    genMissingParamName)
{
    int count{};

    ForEachArg(parameters, [&](const auto& p) {
        auto name{GetName(*p)};

        // A special case for CXXInheritedCtor. A user can omit the parameters name, but wihtout a name the call to the
        // base constructor may look like calling the default constructor. In such a case we create a name.
        if((GenMissingParamName::Yes == genMissingParamName) && (0 == name.length())) {
            name = BuildInternalVarName(StrCat("param", count));
            ++count;
        }

        // Get the attributes and insert them, if there are any
        CodeGenerator codeGenerator{*this};
        codeGenerator.InsertAttributes(p->attrs());

        if(NameOnly::No == nameOnly) {
            const auto& type{p->getType()};

            Append(GetTypeNameAsParameter(type, name));
        } else {
            Append(name);

            if(isa<PackExpansionType>(p->getType())) {
                Append(kwElipsis);
            }
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
        // go the string backwards and find the first non-whitespace character
        const auto res = std::find_if(
            std::rbegin(mOutput), std::rbegin(mOutput) + SCOPE_INDENT, [](const char& c) { return ' ' != c; });

        // check if the string did end with at least one whitespace
        if(const auto& end = std::rbegin(mOutput); res != end) {
            // remove the whitespaces at the end of the string
            mOutput.resize(mOutput.size() - std::distance(end, res));
        }
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
