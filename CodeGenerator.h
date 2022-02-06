/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_CODE_GENERATOR_H
#define INSIGHTS_CODE_GENERATOR_H

#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/ADT/APInt.h"

#include "ClangCompat.h"
#include "InsightsStaticStrings.h"
#include "InsightsStrongTypes.h"
#include "InsightsUtility.h"
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
        InitCapture,
        CallExpr,
        OperatorCallExpr,
        MemberCallExpr,
        LambdaExpr,
        ReturnStmt,
        BinaryOperator,
        CXXMethodDecl,
    };

    class LambdaHelper : public StackListEntry<LambdaHelper>
    {
    public:
        LambdaHelper(const LambdaCallerType lambdaCallerType, OutputFormatHelper& outputFormatHelper)
        : mLambdaCallerType{lambdaCallerType}
        , mCurrentPos{outputFormatHelper.CurrentPos()}
        , mOutputFormatHelper{outputFormatHelper}
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
        OutputFormatHelper     mLambdaOutputFormatHelper{};
        std::string            mInits{};
    };
    //-----------------------------------------------------------------------------

    using LambdaStackType = StackList<class LambdaHelper>;

    STRONG_BOOL(LambdaInInitCapture);  ///! Singal whether we are processing a lambda created and assigned to an init
                                       /// capture of another lambda.

    constexpr CodeGenerator(OutputFormatHelper& _outputFormatHelper,
                            LambdaStackType&    lambdaStack,
                            LambdaInInitCapture lambdaInitCapture)
    : mOutputFormatHelper{_outputFormatHelper}
    , mLambdaStack{lambdaStack}
    , mLambdaInitCapture{lambdaInitCapture}
    {
    }

public:
    explicit constexpr CodeGenerator(OutputFormatHelper& _outputFormatHelper)
    : CodeGenerator{_outputFormatHelper, mLambdaStackThis}
    {
    }

    constexpr CodeGenerator(OutputFormatHelper& _outputFormatHelper, LambdaInInitCapture lambdaInitCapture)
    : CodeGenerator{_outputFormatHelper, mLambdaStackThis, lambdaInitCapture}
    {
    }

    constexpr CodeGenerator(OutputFormatHelper& _outputFormatHelper, LambdaStackType& lambdaStack)
    : CodeGenerator{_outputFormatHelper, lambdaStack, LambdaInInitCapture::No}
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

    template<typename T>
    void InsertTemplateArgs(const T& t)
    {
        if constexpr(std::is_same_v<T, FunctionDecl>) {
            if(const auto* tmplArgs = t.getTemplateSpecializationArgs()) {
                InsertTemplateArgs(*tmplArgs);
            }
        } else if constexpr(requires { t.template_arguments(); }) {
            if constexpr(std::is_same_v<DeclRefExpr, T>) {
                if(0 == t.getNumTemplateArgs()) {
                    return;
                }
            }

            InsertTemplateArgs(t.template_arguments());

        } else if constexpr(requires { t.asArray(); }) {
            InsertTemplateArgs(t.asArray());
        }
    }

    void InsertTemplateArgs(const ClassTemplateSpecializationDecl& clsTemplateSpe);

    /// \brief Insert the code for a FunctionDecl.
    ///
    /// This inserts the code of a FunctionDecl (and everything which is derived from one). It takes care of
    /// CXXMethodDecl's access modifier as well as things like constexpr, noexcept, static and more.
    ///
    /// @param decl The FunctionDecl to process.
    /// @param skipAccess Show or hide access modifiers (public, private, protected). The default is to show them.
    /// @param cxxInheritedCtorDecl If not null, the type and name of this decl is used for the parameters.
    void InsertAccessModifierAndNameWithReturnType(const FunctionDecl&       decl,
                                                   const CXXConstructorDecl* cxxInheritedCtorDecl = nullptr);

    /// Track whether we have at least one local static variable in this TU.
    /// If so we need to insert the <new> header for the placement-new.
    static bool NeedToInsertNewHeader() { return mHaveLocalStatic; }

    /// Track whether we have a noexcept transformation which needs the exception header.
    static bool NeedToInsertExceptionHeader() { return mHaveException; }

    /// Track whether we inserted a std::move due, to a static transformation, this means we need the utility header.
    static bool NeedToInsertUtilityHeader() { return mHaveMovedLambda; }

    template<typename T>
    void InsertTemplateArgs(const ArrayRef<T>& array)
    {
        mOutputFormatHelper.Append('<');

        ForEachArg(array, [&](const auto& arg) { InsertTemplateArg(arg); });

        /* put as space between to closing brackets: >> -> > > */
        if(mOutputFormatHelper.GetString().back() == '>') {
            mOutputFormatHelper.Append(' ');
        }

        mOutputFormatHelper.Append('>');
    }

    void InsertAttributes(const Decl::attr_range&);
    void InsertAttribute(const Attr&);

protected:
    virtual bool InsertVarDecl() { return true; }
    virtual bool InsertComma() { return false; }
    virtual bool InsertSemi() { return true; }
    virtual bool InsertNamespace() const { return false; }

    /// \brief Show casts to xvalues independent from the show all casts option.
    ///
    /// This helps showing xvalue casts in structured bindings.
    virtual bool ShowXValueCasts() const { return false; }

    void HandleTemplateParameterPack(const ArrayRef<TemplateArgument>& args);
    void HandleCompoundStmt(const CompoundStmt* stmt);
    /// \brief Show what is behind a local static variable.
    ///
    /// [stmt.dcl] p4: Initialization of a block-scope variable with static storage duration is thread-safe since C++11.
    /// Regardless of that, as long as it is a non-trivally construct and destructable class the compiler adds code to
    /// track the initialization state. Reference:
    /// - www.opensource.apple.com/source/libcppabi/libcppabi-14/src/cxa_guard.cxx
    void HandleLocalStaticNonTrivialClass(const VarDecl* stmt);

    void FormatCast(const std::string_view castName,
                    const QualType&        CastDestType,
                    const Expr*            SubExpr,
                    const CastKind&        castKind);

    void ForEachArg(const auto& arguments, auto&& lambda) { mOutputFormatHelper.ForEachArg(arguments, lambda); }

    void InsertArgWithParensIfNeeded(const Stmt* stmt);
    void InsertSuffix(const QualType& type);
    void InsertTemplateArg(const TemplateArgument& arg);
    void InsertTemplateArg(const TemplateArgumentLoc& arg) { InsertTemplateArg(arg.getArgument()); }
    bool InsertLambdaStaticInvoker(const CXXMethodDecl* cxxMethodDecl);
    void InsertTemplateParameters(const TemplateParameterList& list);

    STRONG_BOOL(InsertInline);

    void InsertConceptConstraint(const llvm::SmallVectorImpl<const Expr*>& constraints,
                                 const InsertInline                        insertInline);
    void InsertConceptConstraint(const FunctionDecl* tmplDecl);
    void InsertConceptConstraint(const VarDecl* varDecl);
    void InsertConceptConstraint(const TemplateParameterList& tmplDecl);

    void InsertQualifierAndNameWithTemplateArgs(const DeclarationName& declName, const auto* stmt)
    {
        InsertQualifierAndName(declName, stmt->getQualifier(), stmt->hasTemplateKeyword());

        if(stmt->getNumTemplateArgs()) {
            InsertTemplateArgs(*stmt);
        } else if(stmt->hasExplicitTemplateArgs()) {
            // we have empty templates arguments, but angle brackets provided by the user
            mOutputFormatHelper.Append("<>"sv);
        }
    }

    void InsertQualifierAndName(const DeclarationName&     declName,
                                const NestedNameSpecifier* qualifier,
                                const bool                 hasTemplateKeyword);

    /// For a special case, when a LambdaExpr occurs in a Constructor from an
    /// in class initializer, there is a need for a more narrow scope for the \c LAMBDA_SCOPE_HELPER.
    void InsertCXXMethodHeader(const CXXMethodDecl* stmt, OutputFormatHelper& initOutputFormatHelper);

    void InsertTemplateGuardBegin(const FunctionDecl* stmt);
    void InsertTemplateGuardEnd(const FunctionDecl* stmt);

    /// \brief Insert \c template<> to introduce a template specialization.
    void InsertTemplateSpecializationHeader() { mOutputFormatHelper.AppendNewLine("template<>"sv); }

    void InsertNamespace(const NestedNameSpecifier* namespaceSpecifier);
    void ParseDeclContext(const DeclContext* Ctx);

    STRONG_BOOL(SkipBody);
    void InsertCXXMethodDecl(const CXXMethodDecl* stmt, SkipBody skipBody);

    /// \brief Generalized function to insert either a \c CXXConstructExpr or \c CXXUnresolvedConstructExpr
    template<typename T>
    void InsertConstructorExpr(const T* stmt);

    /// \brief Check whether or not this statement will add curlys or parentheses and add them only if required.
    void InsertCurlysIfRequired(const Stmt* stmt);

    void InsertIfOrSwitchInitVariables(same_as_any_of<const IfStmt, const SwitchStmt> auto* stmt);

    STRONG_BOOL(AddNewLineAfter);

    void WrapInCompoundIfNeeded(const Stmt* stmt, const AddNewLineAfter addNewLineAfter);

    STRONG_BOOL(AddSpaceAtTheEnd);

    enum class BraceKind
    {
        Parens,
        Curlys
    };

    void WrapInParens(void_func_ref lambda, const AddSpaceAtTheEnd addSpaceAtTheEnd = AddSpaceAtTheEnd::No);

    void WrapInParensIfNeeded(bool                   needsParens,
                              void_func_ref          lambda,
                              const AddSpaceAtTheEnd addSpaceAtTheEnd = AddSpaceAtTheEnd::No);

    void WrapInCurliesIfNeeded(bool                   needsParens,
                               void_func_ref          lambda,
                               const AddSpaceAtTheEnd addSpaceAtTheEnd = AddSpaceAtTheEnd::No);

    void WrapInCurlys(void_func_ref lambda, const AddSpaceAtTheEnd addSpaceAtTheEnd = AddSpaceAtTheEnd::No);

    void WrapInParensOrCurlys(const BraceKind        curlys,
                              void_func_ref          lambda,
                              const AddSpaceAtTheEnd addSpaceAtTheEnd = AddSpaceAtTheEnd::No);

    void UpdateCurrentPos() { mCurrentPos = mOutputFormatHelper.CurrentPos(); }

    static std::string_view GetBuiltinTypeSuffix(const BuiltinType::Kind& kind);

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

    void               HandleLambdaExpr(const LambdaExpr* stmt, LambdaHelper& lambdaHelper);
    static std::string FillConstantArray(const ConstantArrayType* ct, const std::string& value, const uint64_t startAt);
    static std::string GetValueOfValueInit(const QualType& t);

    LambdaStackType  mLambdaStackThis{};
    LambdaStackType& mLambdaStack;

    STRONG_BOOL(SkipVarDecl);
    STRONG_BOOL(UseCommaInsteadOfSemi);
    STRONG_BOOL(NoEmptyInitList);
    STRONG_BOOL(ShowConstantExprValue);
    LambdaInInitCapture mLambdaInitCapture{LambdaInInitCapture::No};

    ShowConstantExprValue mShowConstantExprValue{ShowConstantExprValue::No};
    SkipVarDecl           mSkipVarDecl{SkipVarDecl::No};
    UseCommaInsteadOfSemi mUseCommaInsteadOfSemi{UseCommaInsteadOfSemi::No};
    NoEmptyInitList       mNoEmptyInitList{
        NoEmptyInitList::No};  //!< At least in case if a requires-clause containing T{} we don't want to get T{{}}.
    const LambdaExpr*  mLambdaExpr{};
    static inline bool mHaveLocalStatic;  //!< Track whether there was a thread-safe \c static in the code.
    static inline bool mHaveMovedLambda;  //!< Track whether there was a std::move inserted.
    static inline bool
        mHaveException;  //!< Track whether there was a noexcept transformation requireing the exception header.
    static constexpr auto MAX_FILL_VALUES_FOR_ARRAYS{
        uint64_t{100}};  //!< This is the upper limit of elements which will be shown for an array when filled by \c
                         //!< FillConstantArray.
    llvm::Optional<size_t> mCurrentPos{};        //!< The position in mOutputFormatHelper where a potential
                                                 //!< std::initializer_list expansion must be inserted.
    llvm::Optional<size_t> mCurrentReturnPos{};  //!< The position in mOutputFormatHelper from a return where a
                                                 //!< potential std::initializer_list expansion must be inserted.
    llvm::Optional<size_t> mCurrentFieldPos{};   //!< The position in mOutputFormatHelper in a class where where a
                                                 //!< potential std::initializer_list expansion must be inserted.
    OutputFormatHelper* mOutputFormatHelperOutside{
        nullptr};                        //!< Helper output buffer for std::initializer_list expansion.
    bool mRequiresImplicitReturnZero{};  //!< Track whether this is a function with an imlpicit return 0.
};
//-----------------------------------------------------------------------------

class LambdaCodeGenerator final : public CodeGenerator
{
public:
    using CodeGenerator::CodeGenerator;

    using CodeGenerator::InsertArg;
    void InsertArg(const CXXThisExpr* stmt) override;

    bool mCapturedThisAsCopy{};
};
//-----------------------------------------------------------------------------

/*
 * \brief Special case to generate the inits of e.g. a \c ForStmt.
 *
 * This class is a specialization to handle cases where we can have multiple init statements to the same variable and
 * hence need only one time the \c VarDecl. An example a for-loops:
\code
for(int x=2, y=3, z=4; i < x; ++i) {}
\endcode
 */
class MultiStmtDeclCodeGenerator final : public CodeGenerator
{
public:
    using CodeGenerator::CodeGenerator;

    // Insert the semi after the last declaration. This implies that this class always requires its own scope.
    ~MultiStmtDeclCodeGenerator() { mOutputFormatHelper.Append("; "sv); }

    using CodeGenerator::InsertArg;

protected:
    OnceTrue  mInsertVarDecl{};  //! Insert the \c VarDecl only once.
    OnceFalse mInsertComma{};    //! Insert the comma after we have generated the first \c VarDecl and we are about to
                                 //! insert another one.

    bool InsertVarDecl() override { return mInsertVarDecl; }
    bool InsertComma() override { return mInsertComma; }
    bool InsertSemi() override { return false; }
};
//-----------------------------------------------------------------------------

}  // namespace clang::insights

#endif /* INSIGHTS_CODE_GENERATOR_H */
