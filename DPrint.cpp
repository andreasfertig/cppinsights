/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "DPrint.h"
#include "InsightsStaticStrings.h"
#include "OutputFormatHelper.h"

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "llvm/Support/Path.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

static void ToDo(const char* name, OutputFormatHelper& outputFormatHelper, std::string_view file, const int line)
{
    const auto fileName = [&]() {
        if(llvm::sys::path::is_separator(file[0])) {
            return std::string_view{llvm::sys::path::filename(file)};
        }

        return file;
    }();

    outputFormatHelper.Append("/* INSIGHTS-TODO: "sv, fileName, ":"sv, line, " stmt: "sv, name, kwSpaceCCommentEnd);
}
//-----------------------------------------------------------------------------

void ToDo(const Stmt* stmt, OutputFormatHelper& outputFormatHelper, std::string_view file, const int line)
{
    const char* name = [&]() {
        if(stmt && stmt->getStmtClassName()) {
            Dump(stmt);

            return stmt->getStmtClassName();
        }

        Error("arg urg: class name is empty\n");

        return "";
    }();

    ToDo(name, outputFormatHelper, file, line);
}
//-----------------------------------------------------------------------------

void ToDo(const Decl* stmt, OutputFormatHelper& outputFormatHelper, std::string_view file, const int line)
{
    const char* name = [&]() {
        if(stmt && stmt->getDeclKindName()) {
            Dump(stmt);
            return stmt->getDeclKindName();
        }

        Error("decl urg: class name is empty\n");

        return "";
    }();

    ToDo(name, outputFormatHelper, file, line);
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
