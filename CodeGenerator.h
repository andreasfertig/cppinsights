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

#include "ClangCompat.h"
#include "InsightsHelpers.h"
#include "InsightsStaticStrings.h"
#include "InsightsStrongTypes.h"
#include "OutputFormatHelper.h"
#include "StackList.h"
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

    enum class LambdaCallerType
    {
        VarDecl,
        CallExpr,
        OperatorCallExpr,
        MemberCallExpr,
        LambdaExpr,
        ReturnStmt,
        BinaryOperator,
    };

    class LambdaHelper : public StackListEntry<LambdaHelper>
    {
    public:
        LambdaHelper(const LambdaCallerType lambdaCallerType, OutputFormatHelper& outputFormatHelper)
        : mLambdaCallerType{lambdaCallerType}
        , mCurrentPos{outputFormatHelper.CurrentPos()}
        , mOutputFormatHelper{outputFormatHelper}
        , mLambdaOutputFormatHelper{}
        , mInits{}
        {
            mLambdaOutputFormatHelper.SetIndent(mOutputFormatHelper);
        }

        void finish()
        {
            if(!mLambdaOutputFormatHelper.empty()) {
                mOutputFormatHelper.InsertAt(mCurrentPos, mLambdaOutputFormatHelper.GetString());
            }
        }

        OutputFormatHelper& buffer() { return mLambdaOutputFormatHelper; }

        std::string& inits() { return mInits; }

        void insertInits(OutputFormatHelper& outputFormatHelper)
        {
            if(!mInits.empty()) {
                outputFormatHelper.Append(mInits);
                mInits.clear();
            }
        }

        LambdaCallerType callerType() const { return mLambdaCallerType; }

    private:
        const LambdaCallerType mLambdaCallerType;
        const size_t           mCurrentPos;
        OutputFormatHelper&    mOutputFormatHelper;
        OutputFormatHelper     mLambdaOutputFormatHelper;
        std::string            mInits;
    };
    //-----------------------------------------------------------------------------

    using LambdaStackType = StackList<class LambdaHelper>;

public:
    explicit CodeGenerator(OutputFormatHelper& _outputFormatHelper)
    : mOutputFormatHelper{_outputFormatHelper}
    , mLambdaStackThis{}
    , mLambdaStack{mLambdaStackThis}
    , mSkipVarDecl{SkipVarDecl::No}
    , mUseCommaInsteadOfSemi{UseCommaInsteadOfSemi::No}
    , mLambdaExpr{nullptr}
    {
    }

    explicit CodeGenerator(OutputFormatHelper& _outputFormatHelper, LambdaStackType& lambdaStack)
    : mOutputFormatHelper{_outputFormatHelper}
    , mLambdaStackThis{}
    , mLambdaStack{lambdaStack}
    , mSkipVarDecl{SkipVarDecl::No}
    , mUseCommaInsteadOfSemi{UseCommaInsteadOfSemi::No}
    , mLambdaExpr{nullptr}
    {
    }

    virtual ~CodeGenerator() = default;

#define IGNORED_DECL(type)                                                                                             \
    virtual void InsertArg(const type*) {}
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

    STRONG_BOOL(SkipAccess);

    /// \brief Insert the code for a FunctionDecl.
    ///
    /// This inserts the code of a FunctionDecl (and everything which is derived from one). It takes care of
    /// CXXMethodDecl's access modifier as well as things like constexpr, noexcept, static and more.
    ///
    /// @param decl The FunctionDecl to process.
    /// @param skipAccess Show or hide access modifiers (public, private, protected). The default is to show them.
    /// @param cxxInheritedCtorDecl If not null, the type and name of this decl is used for the parameters.
    void InsertAccessModifierAndNameWithReturnType(const FunctionDecl&       decl,
                                                   const SkipAccess          skipAccess           = SkipAccess::No,
                                                   const CXXConstructorDecl* cxxInheritedCtorDecl = nullptr);

protected:
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

    void print(const NestedNameSpecifier* namespaceSpecifier);
    void ParseDeclContext(const DeclContext* Ctx);

    /// \brief Check whether or not this statement will add curlys or parentheses and add them only if required.
    void InsertCurlysIfRequired(const Stmt* stmt);

    STRONG_BOOL(AddSpaceAtTheEnd);

    enum class BraceKind
    {
        Parens,
        Curlys
    };

    template<typename T>
    void WrapInParensOrCurlys(const BraceKind        curlys,
                              T&&                    lambda,
                              const AddSpaceAtTheEnd addSpaceAtTheEnd = AddSpaceAtTheEnd::No);

    static const char* GetKind(const UnaryExprOrTypeTraitExpr& uk);
    static const char* GetBuiltinTypeSuffix(const BuiltinType::Kind& kind);

    class LambdaScopeHandler
    {
    public:
        LambdaScopeHandler(LambdaStackType&       stack,
                           OutputFormatHelper&    outputFormatHelper,
                           const LambdaCallerType lambdaCallerType);

        ~LambdaScopeHandler();

    private:
        LambdaStackType& mStack;
        LambdaHelper     mHelper;

        OutputFormatHelper& GetBuffer(OutputFormatHelper& outputFormatHelper) const;
    };

    void HandleLambdaExpr(const LambdaExpr* stmt, LambdaHelper& lambdaHelper);

    LambdaStackType  mLambdaStackThis;
    LambdaStackType& mLambdaStack;

    STRONG_BOOL(SkipVarDecl);
    STRONG_BOOL(UseCommaInsteadOfSemi);

    SkipVarDecl           mSkipVarDecl;
    UseCommaInsteadOfSemi mUseCommaInsteadOfSemi;

    const LambdaExpr* mLambdaExpr;
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

    using CodeGenerator::InsertArg;
    void InsertArg(const DeclRefExpr* stmt) override;
};
//-----------------------------------------------------------------------------

class LambdaCodeGenerator final : public CodeGenerator
{
public:
    using CodeGenerator::CodeGenerator;

    using CodeGenerator::InsertArg;
    void InsertArg(const CXXThisExpr* stmt) override;

    bool mCapturedThisAsCopy;
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_CODE_GENERATOR_H */
