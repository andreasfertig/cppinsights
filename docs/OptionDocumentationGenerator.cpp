/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

/// \brief Create a markdown file which contains the information about the option.
static bool CreateFile(const std::string& optionName, bool optionDefault, const char* description)
{
    const std::string mdFileName{"opt-" + optionName + ".md"};
    ofstream          mdFile{mdFileName};

    if(not mdFile.is_open()) {
        return false;
    }

    cout << "Generating: " << mdFileName << "\n";
    std::string linkName{optionName};
    std::replace(linkName.begin(), linkName.end(), '-', '_');

    mdFile << "# " << optionName << " {#" << linkName << "}\n";
    mdFile << description << "\n\n";
    mdFile << "__Default:__ " << (optionDefault ? "On" : "Off") << "\n\n";
    mdFile << "__Examples:__\n\n";
    mdFile << "```.cpp\n";
    mdFile << optionName << "-source\n";
    mdFile << "```\n\n";
    mdFile << "transforms into this:\n\n";
    mdFile << "```.cpp\n";
    mdFile << optionName << "-transformed\n";
    mdFile << "```\n";
    mdFile.close();

    return true;
}

int main()
{
#define INSIGHTS_OPT(opt, name, deflt, description) CreateFile(opt, deflt, description);
#include "../InsightsOptions.def"

#undef INSIGHTS_OPT

    ofstream mdFile{"CommandLineOptions.md"};

    if(not mdFile.is_open()) {
        return -1;
    }

    mdFile << "# C++ Insights command line options {#command_line_options}\n\n";

    std::vector<std::string> options{};

#define INSIGHTS_OPT(opt, name, deflt, description) options.emplace_back(opt);
#include "../InsightsOptions.def"

    sort(options.begin(), options.end());

    for(const auto& opt : options) {
        std::string linkName{opt};
        std::replace(linkName.begin(), linkName.end(), '-', '_');

        //* [alt-syntax-for](@ref alt_syntax_for)
        mdFile << "* [" << opt << "](@ref " << linkName << ")\n";
    }
}
