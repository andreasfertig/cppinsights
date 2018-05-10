/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_CODE_GENERATOR_H
#define INSIGHTS_CODE_GENERATOR_H

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "DPrint.h"
#include "InsightsHelpers.h"
#include "InsightsStaticStrings.h"
#include "InsightsStrongTypes.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

/// \brief More or less the heart of C++ Insights.
///
/// This is the place where nearly all of the transformations happen. This class knows the needed types and how to
/// generated code from them.
class CodeGenerator
{
protected:
    OutputFormatHelper& mOutputFormatHelper;

public:
    explicit CodeGenerator(OutputFormatHelper& _outputFormatHelper)
    : mOutputFormatHelper{_outputFormatHelper}
    {
    }

    virtual ~CodeGenerator() = default;

#define IGNORED_STMT(type)                                                                                             \
    virtual void InsertArg(const type*) {}
#define SUPPORTED_DECL(type) virtual void InsertArg(const type* stmt);
#define SUPPORTED_STMT(type) virtual void InsertArg(const type* stmt);

#include "CodeGeneratorTypes.h"

    virtual void InsertArg(const Decl* stmt);
    virtual void InsertArg(const Stmt* stmt);

    void InsertTemplateArgs(const DeclRefExpr& stmt);

    void InsertTemplateArgs(const ClassTemplateSpecializationDecl& clsTemplateSpe);
    void InsertTemplateArgs(const FunctionDecl& FD)
    {
        if(const auto* tmplArgs = FD.getTemplateSpecializationArgs()) {
            InsertTemplateArgs(tmplArgs->asArray());
        }
    }

protected:
    void HandleCharacterLiteral(const CharacterLiteral& stmt);
    void HandleTemplateParameterPack(const ArrayRef<TemplateArgument>& args);
    void HandleCompoundStmt(const CompoundStmt* stmt);
    void HandleLocalStaticNonTrivialClass(const VarDecl* stmt);

    STRONG_BOOL(AsComment);
    void FormatCast(const std::string castName,
                    const QualType&   CastDestType,
                    const Expr*       SubExpr,
                    const CastKind&   castKind,
                    const AsComment   comment = AsComment::No);

    template<typename T, typename Lambda>
    void ForEachArg(const T& arguments, Lambda&& lambda)
    {
        OutputFormatHelper::ForEachArg(arguments, mOutputFormatHelper, lambda);
    }

    void InsertArgWithParensIfNeeded(const Stmt* stmt);
    void InsertSuffix(const QualType& type);
    void InsertTemplateArgs(const ArrayRef<TemplateArgument>& array);
    void InsertTemplateArg(const TemplateArgument& arg);

    static const char* GetKind(const UnaryExprOrTypeTraitExpr& uk);
    static const char* GetOpcodeName(const int kind);
    static const char* GetBuiltinTypeSuffix(const BuiltinType& type);
};
//-----------------------------------------------------------------------------

class StructuredBindingsCodeGenerator final : public CodeGenerator
{
    const std::string& mVarName;

public:
    StructuredBindingsCodeGenerator(OutputFormatHelper& _outputFormatHelper, const std::string& varName)
    : CodeGenerator{_outputFormatHelper}
    , mVarName{varName}
    {
    }

    void InsertArg(const DeclRefExpr* stmt) override;
};
//-----------------------------------------------------------------------------

class LambdaCodeGenerator final : public CodeGenerator
{
public:
    using CodeGenerator::CodeGenerator;

    void InsertArg(const CXXThisExpr* stmt) override;
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_CODE_GENERATOR_H */
