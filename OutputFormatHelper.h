/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef OUTPUT_FORMAT_HELPER_H
#define OUTPUT_FORMAT_HELPER_H
//-----------------------------------------------------------------------------

#include <string_view>
#include <utility>
using namespace std::literals;

#include "InsightsOnce.h"
#include "InsightsStrCat.h"
#include "InsightsStrongTypes.h"
//-----------------------------------------------------------------------------

namespace clang::insights {
//-----------------------------------------------------------------------------

/// \brief The C++ Insights formatter.
///
/// Most of the code is handed to \ref OutputFormatHelper for easy code formatting.
class OutputFormatHelper
{
public:
    OutputFormatHelper() = default;

    explicit OutputFormatHelper(const unsigned indent)
    : mDefaultIndent{indent}
    {
    }

    operator std::string_view() const& { return {mOutput}; }
    operator StringRef() const& { return {mOutput}; }

    auto size() const { return mOutput.size(); }

    /// \brief Returns the current position in the output buffer.
    size_t CurrentPos() const { return mOutput.length(); }

    /// \brief Insert a string before the position \c atPos
    void InsertAt(const size_t atPos, std::string_view data) { mOutput.insert(atPos, data); }

    STRONG_BOOL(SkipIndenting);

    auto GetIndent() const { return mDefaultIndent; }

    /// \brief Set the indent level of this class to \c indent.
    void SetIndent(const unsigned indent, const SkipIndenting skipIndenting = SkipIndenting::No)
    {
        mDefaultIndent = indent;

        if(SkipIndenting::No == skipIndenting) {
            Indent(mDefaultIndent);
        }
    }

    /// \brief Set the indent level of this class to that of \c rhs.
    void SetIndent(const OutputFormatHelper& rhs, const SkipIndenting skipIndenting = SkipIndenting::No)
    {
        if(&rhs != this) {
            mDefaultIndent = rhs.mDefaultIndent;

            if(SkipIndenting::No == skipIndenting) {
                Indent(mDefaultIndent);
            }
        }
    }

    /// \brief Check whether the buffer is empty.
    ///
    /// This also treats a string of just whitespaces as empty.
    bool empty() const { return mOutput.empty() or (std::string::npos == mOutput.find_first_not_of(' ', 0)); }

    /// \brief Returns a reference to the underlying string buffer.
    std::string& GetString() { return mOutput; }

    /// \brief Append a single character
    ///
    /// Append a single character to the buffer
    void Append(const char c) { mOutput += c; }

    void Append(const std::string_view& arg) { mOutput += arg; }

    /// \brief Append a variable number of data
    ///
    /// The \c StrCat function which is used ensures, that a \c StringRef or a char are converted appropriately.
    void Append(const auto&... args) { details::StrCat(mOutput, args...); }

    /// \brief Same as \ref Append but adds a newline after the last argument.
    ///
    /// Append a single character to the buffer
    void AppendNewLine(const char c)
    {
        mOutput += c;
        NewLine();
    }

    void AppendNewLine(const std::string_view& arg)
    {
        mOutput += arg;
        NewLine();
    }

    /// \brief Same as \ref Append but adds a newline after the last argument.
    void AppendNewLine(const auto&... args)
    {
        if constexpr(0 < sizeof...(args)) {
            details::StrCat(mOutput, args...);
        }

        NewLine();
    }

    void AppendComment(const std::string_view& arg)
    {
        Append("/* "sv);
        Append(arg);
        Append(" */"sv);
    }

    void AppendCommentNewLine(const std::string_view& arg)
    {
        AppendComment(arg);
        NewLine();
    }

    void AppendCommentNewLine(const auto&... args)
    {
        if constexpr(0 < sizeof...(args)) {
            AppendComment(StrCat(args...));
        }

        NewLine();
    }

    STRONG_BOOL(NameOnly);
    STRONG_BOOL(GenMissingParamName);

    /// \brief Append a \c ParamVarDecl array.
    ///
    /// The parameter name is always added as well.
    void AppendParameterList(const ArrayRef<ParmVarDecl*> parameters,
                             const NameOnly               nameOnly            = NameOnly::No,
                             const GenMissingParamName    genMissingParamName = GenMissingParamName::No);

    /// \brief Increase the current indention by \c SCOPE_INDENT
    void IncreaseIndent() { mDefaultIndent += SCOPE_INDENT; }
    /// \brief Decrease the current indention by \c SCOPE_INDENT
    void DecreaseIndent()
    {
        if(mDefaultIndent >= SCOPE_INDENT) {
            mDefaultIndent -= SCOPE_INDENT;
        }
    }

    /// \brief Open a scope by inserting a '{' followed by an indented newline.
    void OpenScope()
    {
        Append('{');
        IncreaseIndent();
        NewLine();
    }

    STRONG_BOOL(NoNewLineBefore);
    /// \brief Close a scope by inserting a '}'
    ///
    /// With the parameter \c newLineBefore a newline after the brace can be inserted.
    void CloseScope(const NoNewLineBefore newLineBefore = NoNewLineBefore::No);

    /// \brief Similiar to \ref CloseScope only this time a ';' is inserted after the brace.
    void CloseScopeWithSemi(const NoNewLineBefore newLineBefore = NoNewLineBefore::No)
    {
        CloseScope(newLineBefore);
        Append(';');
    }

    /// \brief Append a comma if needed.
    void AppendComma(OnceFalse& needsComma)
    {
        if(needsComma) {
            Append(", "sv);
        }
    }

    /// \brief Append a semicolon and a newline.
    void AppendSemiNewLine() { AppendNewLine(';'); }

    /// \brief Append a semicolon and a newline.
    template<typename... Args>
    void AppendSemiNewLine(const Args&... args)
    {
        if constexpr(0 < sizeof...(args)) {
            details::StrCat(mOutput, args...);
        }

        AppendNewLine(';');
    }

    void AppendSemiNewLine(const std::string_view& arg)
    {
        mOutput += arg;
        AppendNewLine(';');
    }

    /// \brief Append a argument list to the buffer.
    ///
    /// This function takes care of the delimiting ',' between the parameters. The lambda \c lambda is called to each
    /// argument after the comma was inserted.
    /// Usage:
    /// \code
    /// ForEachArg(parameters, [&](const auto& p) {
    /// 		// do something with p
    /// });
    /// \endcode
    inline void ForEachArg(const auto& arguments, /*XXX: invocable*/ auto&& lambda)
    {
        OnceFalse needsComma{};
        for(const auto& arg : arguments) {
            if constexpr(std::is_same_v<const TemplateArgument&, decltype(arg)>) {
                if((TemplateArgument::Pack == arg.getKind()) and (0 == arg.pack_size())) {
                    break;
                }
            }

            AppendComma(needsComma);

            lambda(arg);
        }
    }

    void InsertIfDefTemplateGuard() { AppendNewLine("#ifdef INSIGHTS_USE_TEMPLATE"sv); }
    void InsertEndIfTemplateGuard() { AppendNewLine("#endif"sv); }

private:
    static constexpr unsigned SCOPE_INDENT{2};
    unsigned                  mDefaultIndent{};
    std::string               mOutput{};

    void Indent(unsigned count);
    void NewLine()
    {
        mOutput += '\n';
        Indent(mDefaultIndent);
    }

    void RemoveIndent();
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights
#endif /* OUTPUT_FORMAT_HELPER_H */
