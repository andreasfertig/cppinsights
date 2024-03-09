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

static void ToDo(std::string_view name, OutputFormatHelper& outputFormatHelper, std::source_location loc)
{
    const auto fileName = [&]() -> std::string_view {
        if(llvm::sys::path::is_separator(loc.file_name()[0])) {
            return llvm::sys::path::filename(loc.file_name());
        }

        return loc.file_name();
    }();

    outputFormatHelper.Append(
        "/* INSIGHTS-TODO: "sv, fileName, ":"sv, loc.line(), " stmt: "sv, name, kwSpaceCCommentEnd);
}
//-----------------------------------------------------------------------------

void ToDo(const Stmt* stmt, OutputFormatHelper& outputFormatHelper, std::source_location loc)
{
    const std::string_view name = [&]() {
        if(stmt and stmt->getStmtClassName()) {
            Dump(stmt);

            return stmt->getStmtClassName();
        }

        Error("arg urg: class name is empty\n");

        return "";
    }();

    ToDo(name, outputFormatHelper, loc);
}
//-----------------------------------------------------------------------------

void ToDo(const Decl* stmt, OutputFormatHelper& outputFormatHelper, std::source_location loc)
{
    const std::string_view name = [&]() {
        if(stmt and stmt->getDeclKindName()) {
            Dump(stmt);
            return stmt->getDeclKindName();
        }

        Error("decl urg: class name is empty\n");

        return "";
    }();

    ToDo(name, outputFormatHelper, loc);
}
//-----------------------------------------------------------------------------

void ToDo(const class TemplateArgument& stmt, class OutputFormatHelper& outputFormatHelper, std::source_location loc)
{
    const std::string_view name{StrCat("tmplArgKind: ", stmt.getKind())};

    ToDo(name, outputFormatHelper, loc);
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
