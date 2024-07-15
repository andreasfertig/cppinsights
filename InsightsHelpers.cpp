/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "InsightsHelpers.h"
#include "ASTHelpers.h"
#include "ClangCompat.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "Insights.h"
#include "InsightsStaticStrings.h"
#include "OutputFormatHelper.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Lookup.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

ScopeHandler::ScopeHandler(const Decl* d)
: mStack{mGlobalStack}
, mHelper{mScope.length()}
{
    mStack.push(mHelper);

    if(const auto* recordDecl = dyn_cast_or_null<CXXRecordDecl>(d)) {
        mScope.append(GetName(*recordDecl));

        if(const auto* classTmplSpec = dyn_cast_or_null<ClassTemplateSpecializationDecl>(recordDecl)) {
            OutputFormatHelper ofm{};
            CodeGenerator      codeGenerator{ofm};
            codeGenerator.InsertTemplateArgs(*classTmplSpec);

            mScope.append(ofm);
        }

    } else if(const auto* namespaceDecl = dyn_cast_or_null<NamespaceDecl>(d)) {
        mScope.append(namespaceDecl->getName());
    }

    if(not mScope.empty()) {
        mScope.append("::");
    }
}
//-----------------------------------------------------------------------------

ScopeHandler::~ScopeHandler()
{
    const auto length = mStack.pop()->mLength;
    mScope.resize(length);
}
//-----------------------------------------------------------------------------

std::string ScopeHandler::RemoveCurrentScope(std::string name)
{
    if(mScope.length()) {
        auto findAndReplace = [&name](const std::string& scope) {
            if(const auto startPos = name.find(scope, 0); std::string::npos != startPos) {
                if(const auto pos = startPos + scope.length();
                   (pos > name.length()) or (name[pos] != '*')) {  // keep member points (See #374)
                    name.replace(startPos, scope.length(), ""sv);
                    return true;
                }
            }

            return false;
        };

        // The default is that we can replace the entire scope. Suppose we are currently in N::X and having a symbol
        // N::X::y then N::X:: is removed.
        if(not findAndReplace(mScope)) {

            // A special case where we need to remove the scope without the last item.
            std::string tmp{mScope};
            tmp.resize(mGlobalStack.back().mLength);

            findAndReplace(tmp);
        }
    }

    return name;
}
//-----------------------------------------------------------------------------

static std::string GetNamePlain(const NamedDecl& decl)
{
    if(const auto* fd = dyn_cast_or_null<FunctionDecl>(&decl); fd and GetInsightsOptions().UseShow2C) {
        if(fd->isOverloadedOperator()) {
            switch(fd->getOverloadedOperator()) {
#define OVERLOADED_OPERATOR(Name, Spelling, Token, Unary, Binary, MemberOnly)                                          \
    case OO_##Name: return "operator" #Name;

#include "clang/Basic/OperatorKinds.def"

#undef OVERLOADED_OPERATOR
                default: break;
            }
        }
    }

    return decl.getDeclName().getAsString();
}

std::string GetPlainName(const DeclRefExpr& DRE)
{
    return ScopeHandler::RemoveCurrentScope(GetNamePlain(*DRE.getDecl()));
}
//-----------------------------------------------------------------------------

STRONG_BOOL(InsightsSuppressScope);
//-----------------------------------------------------------------------------

STRONG_BOOL(InsightsCanonicalTypes);
//-----------------------------------------------------------------------------

static std::string GetUnqualifiedScopelessName(const Type* type, const InsightsSuppressScope supressScope);
//-----------------------------------------------------------------------------

struct CppInsightsPrintingPolicy : PrintingPolicy
{
    unsigned              CppInsightsUnqualified : 1;  // NOLINT
    InsightsSuppressScope CppInsightsSuppressScope;    // NOLINT

    CppInsightsPrintingPolicy(const Unqualified            unqualified,
                              const InsightsSuppressScope  supressScope,
                              const InsightsCanonicalTypes insightsCanonicalTypes = InsightsCanonicalTypes::No)
    : PrintingPolicy{LangOptions{}}
    {
        adjustForCPlusPlus();
        SuppressUnwrittenScope = true;
        Alignof                = true;
        ConstantsAsWritten     = true;
        AnonymousTagLocations  = false;  // does remove filename and line for from lambdas in parameters
        PrintCanonicalTypes    = InsightsCanonicalTypes::Yes == insightsCanonicalTypes;

        CppInsightsUnqualified   = (Unqualified::Yes == unqualified);
        CppInsightsSuppressScope = supressScope;
    }

    CppInsightsPrintingPolicy()
    : CppInsightsPrintingPolicy{Unqualified::No, InsightsSuppressScope::No}
    {
    }
};
//-----------------------------------------------------------------------------

void ReplaceAll(std::string& str, std::string_view from, std::string_view to)
{
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();  // Handles case where 'to' is a substring of 'from'
    }
}
//-----------------------------------------------------------------------------

namespace details {
static void
BuildNamespace(std::string& fullNamespace, const NestedNameSpecifier* stmt, const IgnoreNamespace ignoreNamespace)
{
    RETURN_IF(not stmt);

    if(const auto* prefix = stmt->getPrefix();
       prefix and not((NestedNameSpecifier::TypeSpecWithTemplate == stmt->getKind()) and
                      isa<DependentTemplateSpecializationType>(stmt->getAsType()))) {
        BuildNamespace(fullNamespace, prefix, ignoreNamespace);
    }

    switch(stmt->getKind()) {
        case NestedNameSpecifier::Identifier: fullNamespace.append(stmt->getAsIdentifier()->getName()); break;

        case NestedNameSpecifier::Namespace:
            RETURN_IF((IgnoreNamespace::Yes == ignoreNamespace) or (stmt->getAsNamespace()->isAnonymousNamespace()));

            fullNamespace.append(stmt->getAsNamespace()->getName());
            break;

        case NestedNameSpecifier::NamespaceAlias: fullNamespace.append(stmt->getAsNamespaceAlias()->getName()); break;

        case NestedNameSpecifier::TypeSpecWithTemplate:
            if(
#if IS_CLANG_NEWER_THAN(17)
                ElaboratedTypeKeyword::Typename
#else
                ElaboratedTypeKeyword::ETK_Typename
#endif
                == stmt->getAsType()->getAs<DependentTemplateSpecializationType>()->getKeyword()) {
                fullNamespace.append(kwTemplateSpace);
            }

            [[fallthrough]];

        case NestedNameSpecifier::TypeSpec:
            fullNamespace.append(GetUnqualifiedScopelessName(stmt->getAsType(), InsightsSuppressScope::Yes));
            // The template parameters are already contained in the type we inserted above.
            break;

        default: break;
    }

    fullNamespace.append("::"sv);
}
//-----------------------------------------------------------------------------
}  // namespace details

std::string GetNestedName(const NestedNameSpecifier* nns, const IgnoreNamespace ignoreNamespace)
{
    std::string ret{};

    if(nns) {
        details::BuildNamespace(ret, nns, ignoreNamespace);
    }

    return ret;
}
//-----------------------------------------------------------------------------

static const std::string GetAsCPPStyleString(const QualType& t, const CppInsightsPrintingPolicy& printingPolicy)
{
    return t.getAsString(printingPolicy);
}
//-----------------------------------------------------------------------------

std::string BuildInternalVarName(const std::string_view& varName)
{
    return StrCat("__", varName);
}
//-----------------------------------------------------------------------------

static std::string
BuildInternalVarName(const std::string_view& varName, const SourceLocation& loc, const SourceManager& sm)
{
    const auto lineNo = sm.getSpellingLineNumber(loc);

    return StrCat(BuildInternalVarName(varName), lineNo);
}
//-----------------------------------------------------------------------------

void InsertBefore(std::string& source, const std::string_view& find, const std::string_view& replace)
{
    const std::string::size_type i = source.find(find, 0);

    if(std::string::npos != i) {
        source.insert(i, replace);
    }
}
//-----------------------------------------------------------------------------

static void InsertAfter(std::string& source, const std::string_view& find, const std::string_view& replace)
{
    const std::string::size_type i = source.find(find, 0);

    if(std::string::npos != i) {
        source.insert(i + find.length(), replace);
    }
}
//-----------------------------------------------------------------------------

std::string MakeLineColumnName(const SourceManager& sm, const SourceLocation& loc, const std::string_view& prefix)
{
    // In case of a macro expansion the expansion(line/column) number gives a unique value.
    const auto lineNo   = loc.isMacroID() ? sm.getExpansionLineNumber(loc) : sm.getSpellingLineNumber(loc);
    const auto columnNo = loc.isMacroID() ? sm.getExpansionColumnNumber(loc) : sm.getSpellingColumnNumber(loc);

    return StrCat(prefix, lineNo, "_"sv, columnNo);
}
//-----------------------------------------------------------------------------

static std::string MakeLineColumnName(const Decl& decl, const std::string_view prefix)
{
    return MakeLineColumnName(GetSM(decl), decl.getBeginLoc(), prefix);
}
//-----------------------------------------------------------------------------

std::string GetLambdaName(const CXXRecordDecl& lambda)
{
    static constexpr auto lambdaPrefix{"__lambda_"sv};
    return MakeLineColumnName(lambda, lambdaPrefix);
}
//-----------------------------------------------------------------------------

static std::string GetAnonymStructOrUnionName(const CXXRecordDecl& cxxRecordDecl)
{
    static constexpr auto prefix{"__anon_"sv};
    return MakeLineColumnName(cxxRecordDecl, prefix);
}
//-----------------------------------------------------------------------------

std::string BuildRetTypeName(const Decl& decl)
{
    static constexpr auto retTypePrefix{"retType_"sv};
    return MakeLineColumnName(decl, retTypePrefix);
}
//-----------------------------------------------------------------------------

const QualType GetDesugarType(const QualType& QT)
{
    if(QT.getTypePtrOrNull()) {
        if(const auto* autoType = QT->getAs<clang::AutoType>(); autoType and autoType->isSugared()) {
            const auto dt = autoType->getDeducedType();

            if(const auto* et = dt->getAs<ElaboratedType>()) {
                return et->getNamedType();
            } else {
                return dt;
            }

        } else if(auto declType = QT->getAs<clang::DecltypeType>()) {
            return declType->desugar();
        }
    }
    return QT;
}
//-----------------------------------------------------------------------------

const std::string EvaluateAsFloat(const FloatingLiteral& expr)
{
    SmallString<16> str{};
    expr.getValue().toString(str);

    if(std::string::npos == str.find('.')) {
        /* in case it is a number like 10.0 toString() seems to leave out the .0. However, as this distinguished
         * between an integer and a floating point literal we need that dot. */
        str.append(".0"sv);
    }

    return std::string{str.str()};
}
//-----------------------------------------------------------------------------

static const VarDecl* GetVarDeclFromDeclRefExpr(const DeclRefExpr& declRefExpr)
{
    const auto* valueDecl = declRefExpr.getDecl();

    return dyn_cast_or_null<VarDecl>(valueDecl);
}
//-----------------------------------------------------------------------------

// own implementation due to lambdas
std::string GetDeclContext(const DeclContext* ctx, WithTemplateParameters withTemplateParameters)
{
    OutputFormatHelper                 mOutputFormatHelper{};
    SmallVector<const DeclContext*, 8> contexts{};

    while(ctx) {
        if(isa<NamedDecl>(ctx)) {
            contexts.push_back(ctx);
        }
        ctx = ctx->getParent();
    }

    for(const auto* declContext : llvm::reverse(contexts)) {
        if(const auto* classTmplSpec = dyn_cast<ClassTemplateSpecializationDecl>(declContext)) {
            mOutputFormatHelper.Append(classTmplSpec->getName());

            CodeGenerator codeGenerator{mOutputFormatHelper};
            codeGenerator.InsertTemplateArgs(*classTmplSpec);

        } else if(const auto* nd = dyn_cast<NamespaceDecl>(declContext)) {
            if(nd->isAnonymousNamespace() or nd->isInline()) {
                continue;
            }

            mOutputFormatHelper.Append(nd->getName());

        } else if(const auto* rd = dyn_cast<RecordDecl>(declContext)) {
            if(not rd->getIdentifier()) {
                continue;
            }

            mOutputFormatHelper.Append(rd->getName());

            // A special case at least for out-of-line static member variables of a class template. They need to carry
            // the template parameters of the class template.
            if(WithTemplateParameters::Yes == withTemplateParameters /*declContext->isNamespace() or declContext->getLexicalParent()->isNamespace() or declContext->getLexicalParent()->isTranslationUnit()*/) {
                if(const auto* cxxRecordDecl = dyn_cast_or_null<CXXRecordDecl>(rd)) {
                    if(const auto* classTmpl = cxxRecordDecl->getDescribedClassTemplate()) {
                        CodeGenerator codeGenerator{mOutputFormatHelper};
                        codeGenerator.InsertTemplateParameters(*classTmpl->getTemplateParameters(),
                                                               CodeGenerator::TemplateParamsOnly::Yes);
                    }
                }
            }

        } else if(dyn_cast<FunctionDecl>(declContext)) {
            continue;

        } else if(const auto* ed = dyn_cast<EnumDecl>(declContext)) {
            if(not ed->isScoped()) {
                continue;
            }

            mOutputFormatHelper.Append(ed->getName());

        } else {
            mOutputFormatHelper.Append(cast<NamedDecl>(declContext)->getName());
        }

        mOutputFormatHelper.Append("::"sv);
    }

    return mOutputFormatHelper.GetString();
}
//-----------------------------------------------------------------------------

namespace details {

STRONG_BOOL(RemoveCurrentScope);  ///!< In some cases we need to keep the scope for a while, so don't remove the scope
                                  /// we are in right now.
//-----------------------------------------------------------------------------

static std::string GetQualifiedName(const NamedDecl&         decl,
                                    const RemoveCurrentScope removeCurrentScope = RemoveCurrentScope::Yes)
{
    std::string scope{GetDeclContext(decl.getDeclContext())};

    scope += decl.getName();

    if(RemoveCurrentScope::Yes == removeCurrentScope) {
        return ScopeHandler::RemoveCurrentScope(scope);
    }

    return scope;
}
//-----------------------------------------------------------------------------

static std::string GetScope(const DeclContext*       declCtx,
                            const RemoveCurrentScope removeCurrentScope = RemoveCurrentScope::Yes)
{
    std::string name{};

    if(not declCtx->isTranslationUnit() and not declCtx->isFunctionOrMethod()) {
        while(declCtx->isInlineNamespace()) {
            declCtx = declCtx->getParent();
        }

        if(not declCtx->isTranslationUnit() and (declCtx->isNamespace() or declCtx->getParent()->isTranslationUnit())) {
            if(const auto* namedDecl = dyn_cast_or_null<NamedDecl>(declCtx)) {
                name = GetQualifiedName(*namedDecl, removeCurrentScope);
                name.append("::"sv);
            }
        }
    }

    return name;
}
//-----------------------------------------------------------------------------

/// \brief SimpleTypePrinter a partially substitution of Clang's TypePrinter.
///
/// With Clang 9 there seems to be a change in `lib/AST/TypePrinter.cpp` in `printTemplateTypeParmBefore`. It now
/// inserts `auto` when it is a lambda auto parameter. Which is correct, but C++ Insights needs the
/// template parameter name to make up compiling code. Hence, this `if` "overrides" the implementation of
/// TypePrinter in that case.
///
/// It also drops some former code which handled `ClassTemplateSpecializationDecl` in a special way. Here the template
/// parameters can be lambdas. Those they need a proper name.
class SimpleTypePrinter
{
private:
    const QualType&                  mType;
    const CppInsightsPrintingPolicy& mPrintingPolicy;
    OutputFormatHelper               mData{};
    std::string                      mDataAfter{};
    bool                             mHasData{false};
    bool                             mSkipSpace{false};
    bool mScanningArrayDimension{};  //!< Only the outer most ConstantArrayType handles the array dimensions and size
    std::string mScope{};            //!< A scope coming from an ElaboratedType which is used for a
                                     //!< ClassTemplateSpecializationDecl if there is no other scope

    bool HandleType(const TemplateTypeParmType* type)
    {
        const TemplateTypeParmDecl* decl = type->getDecl();

        if((nullptr == type->getIdentifier()) or
           (decl and decl->isImplicit()) /* this fixes auto operator()(type_parameter_0_0 container) const */) {

            AppendTemplateTypeParamName(mData, decl, true, type);

            return true;
        }

        return false;
    }

    bool HandleType(const LValueReferenceType* type)
    {
        mDataAfter += " &"sv;

        return HandleType(type->getPointeeType().getTypePtrOrNull());
    }

    bool HandleType(const RValueReferenceType* type)
    {
        mDataAfter += " &&"sv;

        return HandleType(type->getPointeeType().getTypePtrOrNull());
    }

    bool HandleType(const PointerType* type)
    {
        mDataAfter += " *"sv;

        return HandleType(type->getPointeeType().getTypePtrOrNull());
    }

    bool HandleType(const InjectedClassNameType* type) { return HandleType(type->getInjectedTST()); }

    bool HandleType(const RecordType* type)
    {
        /// In case one of the template parameters is a lambda we need to insert the made up name.
        if(const auto* tt = dyn_cast_or_null<ClassTemplateSpecializationDecl>(type->getDecl())) {
            if(const auto* identifierName = mType.getBaseTypeIdentifier()) {
                const auto& scope = GetScope(type->getDecl()->getDeclContext());

                // If we don't have a scope with GetScope use a possible one from ElaboratedType
                if((InsightsSuppressScope::Yes == mPrintingPolicy.CppInsightsSuppressScope) or scope.empty()) {
                    mData.Append(mScope);
                } else {
                    mData.Append(scope);
                }

                mData.Append(identifierName->getName());
                CodeGenerator codeGenerator{mData};
                codeGenerator.InsertTemplateArgs(*tt);

                return true;
            }
        } else if(const auto* cxxRecordDecl = type->getAsCXXRecordDecl()) {
            // Special handling for dependent types. For example, ClassOperatorHandler7Test.cpp A<...* >::B.
            if(type->isDependentType()) {
                std::string context{GetDeclContext(type->getDecl()->getDeclContext())};

                if(not context.empty()) {
                    mData.Append(std::move(context));
                    mData.Append(cxxRecordDecl->getName());

                    return true;
                }
            }

            if(cxxRecordDecl->isLambda()) {
                mData.Append(GetLambdaName(*cxxRecordDecl));

                return true;
            }

            // Handle anonymous struct or union.
            if(IsAnonymousStructOrUnion(cxxRecordDecl)) {
                mData.Append(GetAnonymStructOrUnionName(*cxxRecordDecl));

                return true;
            }

            // we need a name here, as DecltypeType always says true
            mData.Append(GetName(*cxxRecordDecl));
        }

        return false;
    }

    bool HandleType(const AutoType* type) { return HandleType(type->getDeducedType().getTypePtrOrNull()); }

    bool HandleType(const SubstTemplateTypeParmType* type)
    {
        return HandleType(type->getReplacementType().getTypePtrOrNull());
    }

    bool HandleType(const ElaboratedType* type)
    {
        const IgnoreNamespace ignoreNamespace = (mPrintingPolicy.CppInsightsSuppressScope == InsightsSuppressScope::Yes)
                                                    ? IgnoreNamespace::No
                                                    : IgnoreNamespace::Yes;

        mScope = GetNestedName(type->getQualifier(), ignoreNamespace);

        const bool ret = HandleType(type->getNamedType().getTypePtrOrNull());

        mScope.clear();

        return ret;
    }

    bool HandleType(const DependentTemplateSpecializationType* type)
    {
        mData.Append(GetElaboratedTypeKeyword(type->getKeyword()),
                     GetNestedName(type->getQualifier()),
                     kwTemplateSpace,
                     type->getIdentifier()->getName());

        CodeGenerator codeGenerator{mData};
        codeGenerator.InsertTemplateArgs(*type);

        return true;
    }

    bool HandleType(const DeducedTemplateSpecializationType* type)
    {
        return HandleType(type->getDeducedType().getTypePtrOrNull());
    }

    bool HandleType(const TemplateSpecializationType* type)
    {
        if(type->getAsRecordDecl()) {
            // Only if it was some sort of "used" `RecordType` we continue here.
            if(HandleType(type->getAsRecordDecl()->getTypeForDecl())) {
                HandleType(type->getPointeeType().getTypePtrOrNull());
                return true;
            }
        }

        /// This is a specialty discovered with #188_2. In some cases there is a `TemplateTypeParmDecl` which has no
        /// identifier name. Then it will end up as `type-parameter-...`. At least in #188_2: _Head_base<_Idx,
        /// type_parameter_0_1, true> the repetition of the template specialization arguments is not required.
        /// `hasNoName` tries to detect this case and does then print the name of the template only.
        const bool hasNoName{[&] {
            for(const auto& arg : type->template_arguments()) {
                StringStream sstream{};
                sstream.Print(arg);

                if(Contains(sstream.str(), "type-parameter"sv)) {
                    return true;
                }
            }

            return false;
        }()};

        if(hasNoName) {
            StringStream sstream{};
            sstream.Print(*type);

            mData.Append(sstream.str());

            return true;
        }

        return false;
    }

    bool HandleType(const MemberPointerType* type)
    {
        HandleType(type->getPointeeType().getTypePtrOrNull());

        mData.Append('(');

        const bool ret = HandleType(type->getClass());

        mData.Append("::*)"sv);

        HandleTypeAfter(type->getPointeeType().getTypePtrOrNull());

        return ret;
    }

    bool HandleType(const FunctionProtoType* type) { return HandleType(type->getReturnType().getTypePtrOrNull()); }

    void HandleTypeAfter(const FunctionProtoType* type)
    {
        mData.Append('(');

        mSkipSpace = true;
        for(OnceFalse needsComma{}; const auto& t : type->getParamTypes()) {
            if(needsComma) {
                mData.Append(", "sv);
            }

            HandleType(t.getTypePtrOrNull());
        }

        mSkipSpace = false;

        mData.Append(')');

        if(not type->getMethodQuals().empty()) {
            mData.Append(" "sv, type->getMethodQuals().getAsString());
        }

        /// Currently, we are skipping `T->getRefQualifier()` and the exception specification, as well as the trailing
        /// return type.
    }

    bool HandleType(const BuiltinType* type)
    {
        mData.Append(type->getName(mPrintingPolicy));

        if(not mSkipSpace) {
            mData.Append(' ');
        }

        return false;
    }

    bool HandleType(const TypedefType* type)
    {
        if(const auto* decl = type->getDecl()) {
            /// Another filter place for type-parameter where it is contained in the FQN but leads to none compiling
            /// code. Remove it to keep the code valid.
            if(Contains(decl->getQualifiedNameAsString(), "type-parameter"sv)) {
                auto* identifierInfo = decl->getIdentifier();
                mData.Append(identifierInfo->getName());

                return true;
            }

            return HandleType(decl->getUnderlyingType().getTypePtrOrNull());
        }

        return HandleType(type->getPointeeType().getTypePtrOrNull());
    }

    bool HandleType(const ConstantArrayType* type)
    {
        // Only the outer most ConstantArrayType generates the aary dimensions, block others.
        bool scanningArrayDimension = false;
        if(not mScanningArrayDimension) {
            mScanningArrayDimension = true;
            scanningArrayDimension  = true;
        }

        const bool ret = HandleType(type->getElementType().getTypePtrOrNull());

        // Handle the array dimension after the type has been parsed.
        if(scanningArrayDimension) {
            do {
                mData.Append("["sv, GetSize(type), "]"sv);
            } while((type = dyn_cast_or_null<ConstantArrayType>(type->getElementType().getTypePtrOrNull())));

            mScanningArrayDimension = false;
        }

        return ret;
    }

    bool HandleType(const PackExpansionType* type)
    {
        const bool ret = HandleType(type->getPattern().getTypePtrOrNull());

        if(ret) {
            mData.Append(kwElipsis);
        }

        return ret;
    }

    bool HandleType(const DecltypeType* type)
    {
        // A DecltypeType in a template definition is unevaluated and refers ti itself. This check ensures, that in such
        // a situation no expansion is performed.
        if(not isa_and_nonnull<DecltypeType>(type->desugar().getTypePtrOrNull())) {
            const bool skipSpace{mSkipSpace};
            mSkipSpace = true;

            HandleType(type->desugar().getTypePtrOrNull());

            mSkipSpace = skipSpace;

            // if we hit a DecltypeType always use the expanded version to support things like a DecltypeType wrapped in
            // an LValueReferenceType
            return true;
        }

        if(not isa_and_nonnull<DeclRefExpr>(type->getUnderlyingExpr())) {
            P0315Visitor visitor{mData};

            return not visitor.TraverseStmt(type->getUnderlyingExpr());
        }

        return false;
    }

    bool HandleType(const Type* type)
    {
#define HANDLE_TYPE(t)                                                                                                 \
    if(isa<t>(type)) {                                                                                                 \
        return HandleType(dyn_cast_or_null<t>(type));                                                                  \
    }

        if(nullptr == type) {
            return false;
        }

        HANDLE_TYPE(FunctionProtoType);
        HANDLE_TYPE(PointerType);
        HANDLE_TYPE(LValueReferenceType);
        HANDLE_TYPE(RValueReferenceType);
        HANDLE_TYPE(TemplateTypeParmType);
        HANDLE_TYPE(RecordType);
        HANDLE_TYPE(AutoType);
        HANDLE_TYPE(SubstTemplateTypeParmType);
        HANDLE_TYPE(ElaboratedType);
        HANDLE_TYPE(TemplateSpecializationType);
        HANDLE_TYPE(DeducedTemplateSpecializationType);
        HANDLE_TYPE(MemberPointerType);
        HANDLE_TYPE(BuiltinType);
        HANDLE_TYPE(TypedefType);
        HANDLE_TYPE(ConstantArrayType);
        HANDLE_TYPE(InjectedClassNameType);
        HANDLE_TYPE(DependentTemplateSpecializationType);
        HANDLE_TYPE(PackExpansionType);
        HANDLE_TYPE(DecltypeType);

#undef HANDLE_TYPE
        return false;
    }

    void HandleTypeAfter(const Type* type)
    {
#define HANDLE_TYPE(t)                                                                                                 \
    if(isa<t>(type)) {                                                                                                 \
        HandleTypeAfter(dyn_cast_or_null<t>(type));                                                                    \
    }

        if(nullptr != type) {
            HANDLE_TYPE(FunctionProtoType);
        }
    }

    void AddCVQualifiers(const Qualifiers& quals)
    {
        if((false == mPrintingPolicy.CppInsightsUnqualified) and not quals.empty()) {
            mData.Append(quals.getAsString());

            if(not mData.empty() and not mSkipSpace) {
                mData.Append(' ');
            }
        }
    }

public:
    SimpleTypePrinter(const QualType& qt, const CppInsightsPrintingPolicy& printingPolicy)
    : mType{qt}
    , mPrintingPolicy{printingPolicy}
    {
    }

    std::string& GetString() { return mData.GetString(); }

    bool GetTypeString()
    {
        if(const SplitQualType splitted{mType.split()}; splitted.Quals.empty()) {
            const auto& canonicalType = mType.getCanonicalType();

            if(canonicalType->getPointeeType().getLocalFastQualifiers()) {
                AddCVQualifiers(canonicalType->getPointeeType().getLocalQualifiers());
            } else if(canonicalType.getLocalFastQualifiers()) {
                AddCVQualifiers(canonicalType.getLocalQualifiers());
            }
        } else {
            AddCVQualifiers(splitted.Quals);
        }

        const auto* typePtr = mType.getTypePtrOrNull();
        mHasData            = HandleType(typePtr);
        mData.Append(mDataAfter);

        // Take care of 'char* const'
        if(mType.getQualifiers().hasFastQualifiers()) {
            const QualType fastQualifierType{typePtr, mType.getQualifiers().getFastQualifiers()};

            mSkipSpace = true;
            AddCVQualifiers(fastQualifierType.getCanonicalType()->getPointeeType().getLocalQualifiers());
        }

        return mHasData;
    }
};
//-----------------------------------------------------------------------------

static std::string GetName(QualType                    t,
                           const Unqualified           unqualified  = Unqualified::No,
                           const InsightsSuppressScope supressScope = InsightsSuppressScope::No)
{
    const CppInsightsPrintingPolicy printingPolicy{unqualified,
                                                   supressScope,
                                                   (isa<AutoType>(t.getTypePtrOrNull())) ? InsightsCanonicalTypes::Yes
                                                                                         : InsightsCanonicalTypes::No};

    QualType tt = t;

    if(const auto* et = tt->getAs<ElaboratedType>()) {
        if((nullptr == et->getQualifier()) and (nullptr == et->getOwnedTagDecl())) {
            const auto quals = tt.getLocalFastQualifiers();
            tt               = et->getNamedType();
            tt.setLocalFastQualifiers(quals);
        }
    }

    if(SimpleTypePrinter st{t, printingPolicy}; st.GetTypeString()) {
        return ScopeHandler::RemoveCurrentScope(st.GetString());

    } else if(true == printingPolicy.CppInsightsUnqualified) {
        return ScopeHandler::RemoveCurrentScope(GetAsCPPStyleString(tt.getUnqualifiedType(), printingPolicy));
    }

    return ScopeHandler::RemoveCurrentScope(GetAsCPPStyleString(tt, printingPolicy));
}
}  // namespace details
//-----------------------------------------------------------------------------

static bool HasOverload(const FunctionDecl* fd)
{
    auto* ncfd = const_cast<FunctionDecl*>(fd);

    Sema&        sema = GetGlobalCI().getSema();
    LookupResult result{sema, ncfd->getDeclName(), {}, Sema::LookupOrdinaryName};

    if(sema.LookupName(result, sema.getScopeForContext(ncfd->getDeclContext()))) {
        return LookupResult::FoundOverloaded == result.getResultKind();
    }

    return false;
}
//-----------------------------------------------------------------------------

std::string GetSpecialMemberName(const ValueDecl* vd);
//-----------------------------------------------------------------------------

std::string GetCfrontOverloadedFunctionName(const FunctionDecl* fd)
{
    std::string name{};

    if(fd and GetInsightsOptions().UseShow2C and HasOverload(fd) and not fd->isMain()) {
        name = GetSpecialMemberName(fd);

        if(not fd->param_empty()) {
            name += "_";

            for(const auto& param : fd->parameters()) {
                QualType t         = param->getType();
                QualType plainType = t.getNonReferenceType();
                plainType.removeLocalConst();
                plainType.removeLocalVolatile();

                std::string ptr{};

                while(plainType->isPointerType()) {
                    ptr += "p";
                    plainType = plainType->getPointeeType();

                    auto quals  = plainType.getQualifiers();
                    auto lquals = plainType.getLocalQualifiers();

                    if(quals.hasConst() or lquals.hasConst()) {
                        ptr += "c";
                    }

                    plainType.removeLocalConst();
                }

                if(t.isCanonical()) {
                    t = t.getCanonicalType();
                }

                if(plainType->isBuiltinType() and plainType->hasUnsignedIntegerRepresentation()) {
                    std::string tmp{GetName(plainType)};

                    ReplaceAll(tmp, "unsigned ", "u");

                    name += tmp;
                } else {
                    name += GetName(plainType);
                }

                auto quals  = t.getQualifiers();
                auto lquals = t.getLocalQualifiers();

                if(t->isPointerType()) {
                    name += ptr;
                }

                if(quals.hasConst() or lquals.hasConst()) {
                    name += "c";
                }

                if(t->isLValueReferenceType()) {
                    name += "r";
                }

                if(t->isRValueReferenceType()) {
                    name += "R";
                }
            }
        }
    } else if(const auto* md = dyn_cast_or_null<CXXMethodDecl>(fd);
              md and GetInsightsOptions().UseShow2C and md->isVirtual()) {
        name = GetName(*md->getParent());
    }

    return name;
}
//-----------------------------------------------------------------------------

STRONG_BOOL(UseLexicalParent);
//-----------------------------------------------------------------------------

static bool NeedsNamespace(const Decl& decl, UseLexicalParent useLexicalParent)
{
    const auto* declCtx = decl.getDeclContext();
    if(UseLexicalParent::Yes == useLexicalParent) {
        declCtx = declCtx->getLexicalParent();
    }

    if(nullptr == declCtx) {
        return false;
    }

    const bool isFriend{(decl.getFriendObjectKind() != Decl::FOK_None)};
    const bool neitherTransparentNorFriend{not declCtx->isTransparentContext() and not isFriend};

    if(UseLexicalParent::Yes == useLexicalParent) {
        return (declCtx->isNamespace() and not declCtx->isInlineNamespace()) and neitherTransparentNorFriend;
    }

    return (declCtx->isNamespace() or declCtx->isInlineNamespace()) and neitherTransparentNorFriend;
}
//-----------------------------------------------------------------------------

static const SubstTemplateTypeParmType* GetSubstTemplateTypeParmType(const Type* t)
{
    if(const auto* substTemplateTypeParmType = dyn_cast_or_null<SubstTemplateTypeParmType>(t)) {
        return substTemplateTypeParmType;
    } else if(const auto& pointeeType = t->getPointeeType(); not pointeeType.isNull()) {
        return GetSubstTemplateTypeParmType(pointeeType.getTypePtrOrNull());
    }

    return nullptr;
}
//-----------------------------------------------------------------------------

static const DeclRefExpr* FindVarDeclRef(const Stmt* stmt)
{
    if(const auto* dref = dyn_cast_or_null<DeclRefExpr>(stmt)) {
        if(const auto* vd = dyn_cast_or_null<VarDecl>(dref->getDecl())) {
            return dref;
        }
    }

    if(stmt) {
        for(const auto* child : stmt->children()) {
            if(const auto* childRef = FindVarDeclRef(child)) {
                return childRef;
            }
        }
    }

    return nullptr;
}
//-----------------------------------------------------------------------------

/*
 * \brief Get a usable name from a template parameter pack.
 *
 * A template parameter pack, args, as in:
 * \code
template<typename F, typename ...Types>
auto forward(F f, Types &&...args) {
  f(args...);
}

forward(f,1, 2,3);
 * \endcode
 *
 * gets expanded by clang as
 * \code
f(args, args, args);
 * \endcode
 *
 * which would obviously not compile. For clang AST dump it is the right thing. For C++ Insights where the resulting
 * code should be compilable it is not. What this function does is, figure out whether it is a pack expansion and if so,
 * make the parameters unique, such that \c args becomes \c __args1 to \c __args3.
 *
 * The expected type for \c T currently is \c ValueDecl or \c VarDecl.
 */
static std::string GetTemplateParameterPackArgumentName(std::string_view name, const Decl* decl)
{
    if(const auto* parmVarDecl = dyn_cast_or_null<ParmVarDecl>(decl)) {
        if(const auto& originalType = parmVarDecl->getOriginalType(); not originalType.isNull()) {
            if(const auto* substTemplateTypeParmType = GetSubstTemplateTypeParmType(originalType.getTypePtrOrNull());
               substTemplateTypeParmType and substTemplateTypeParmType->getReplacedParameter()->isParameterPack()) {
                return StrCat(BuildInternalVarName(name), parmVarDecl->getFunctionScopeIndex());

            } else if(const auto* fd = parmVarDecl->getParentFunctionOrMethod()) {
                // Get the primary template, if possible and check whether its parameters contain a parameter pack
                if(const auto* primTmpl = dyn_cast_or_null<FunctionDecl>(fd)->getPrimaryTemplate();
                   primTmpl and primTmpl->getTemplateParameters()->hasParameterPack()) {
                    // if so, then search for the matching parameter name.
                    for(const auto* pa : primTmpl->getTemplatedDecl()->parameters()) {
                        // if one is found we suffix it with its function scope index
                        if(pa->isParameterPack() and (parmVarDecl->getNameAsString() == pa->getNameAsString())) {
                            return StrCat(BuildInternalVarName(name), parmVarDecl->getFunctionScopeIndex());
                        }
                    }
                }
            }
        }
    } else if(const auto* varDecl = dyn_cast_or_null<VarDecl>(decl)) {
        // If it is an init-capture in C++2a p0780 brings "Allow pack expansion in lambda init-capture". We
        // need to figure out, whether the initializer for this \c VarDecl comes from a parameter pack. If
        // so, then we use this ParmVarDecl to get the index.
        if(varDecl->isInitCapture()) {
            if(const auto* drefExpr = FindVarDeclRef(varDecl->getInit())) {
                if(const auto* parmVarDecl = dyn_cast_or_null<ParmVarDecl>(drefExpr->getDecl())) {
                    return GetTemplateParameterPackArgumentName(name, parmVarDecl);
                }
            }
        }
    }

    return std::string{name};
}
//-----------------------------------------------------------------------------

std::string GetName(const NamedDecl& nd, const QualifiedName qualifiedName)
{
    std::string name{};

    if(NeedsNamespace(nd, UseLexicalParent::No) or (QualifiedName::Yes == qualifiedName)) {
        if(const auto* cxxMedthodDecl = dyn_cast_or_null<CXXMethodDecl>(&nd)) {
            if(cxxMedthodDecl->isLambdaStaticInvoker()) {
                name = GetName(*cxxMedthodDecl->getParent());
            }
        }

        name += details::GetScope(nd.getDeclContext(), details::RemoveCurrentScope::No);
    }

    name += GetNamePlain(nd);

    name += GetCfrontOverloadedFunctionName(dyn_cast_or_null<FunctionDecl>(&nd));

    return ScopeHandler::RemoveCurrentScope(GetTemplateParameterPackArgumentName(name, &nd));
}
//-----------------------------------------------------------------------------

std::string GetName(const CXXRecordDecl& RD)
{
    if(RD.isLambda()) {
        return GetLambdaName(RD);
    }

    // get the namespace as well
    if(NeedsNamespace(RD, UseLexicalParent::Yes)) {
        return details::GetQualifiedName(RD);
    }

    std::string ret{GetNestedName(RD.getQualifier())};

    if(auto name = RD.getName(); not name.empty()) {
        ret += name;

    } else {
        ret += GetAnonymStructOrUnionName(RD);
    }

    return ScopeHandler::RemoveCurrentScope(ret);
}
//-----------------------------------------------------------------------------

std::string GetTemporaryName(const Expr& tmp)
{
    return BuildInternalVarName(MakeLineColumnName(GetGlobalAST().getSourceManager(), tmp.getEndLoc(), "temporary"sv));
}
//-----------------------------------------------------------------------------

std::string GetName(const CXXTemporaryObjectExpr& tmp)
{
    return GetTemporaryName(tmp);
}
//-----------------------------------------------------------------------------

std::string GetName(const QualType& t, const Unqualified unqualified)
{
    return details::GetName(t, unqualified);
}
//-----------------------------------------------------------------------------

static std::string GetUnqualifiedScopelessName(const Type* type, const InsightsSuppressScope supressScope)
{
    return details::GetName(QualType(type, 0), Unqualified::Yes, supressScope);
}
//-----------------------------------------------------------------------------

std::string GetUnqualifiedScopelessName(const Type* type)
{
    return GetUnqualifiedScopelessName(type, InsightsSuppressScope::No);
}
//-----------------------------------------------------------------------------

QualType GetType(QualType t)
{
    if(GetInsightsOptions().UseShow2C and t->isReferenceType()) {
        return GetGlobalAST().getPointerType(t.getNonReferenceType());
    }

    return t;
}
//-----------------------------------------------------------------------------

template<typename QT, typename SUB_T, typename SUB_T2 = void>
static bool HasTypeWithSubType(const QualType& t)
{
    if(const auto* lref = dyn_cast_or_null<QT>(t.getTypePtrOrNull())) {
        const auto  subType      = GetDesugarType(lref->getPointeeType());
        const auto& ct           = subType.getCanonicalType();
        const auto* plainSubType = ct.getTypePtrOrNull();

        if(const auto* st = dyn_cast_or_null<SUB_T>(plainSubType)) {
            if constexpr(std::is_same_v<void, SUB_T2>) {
                return true;

            } else {
                const auto  subType      = GetDesugarType(st->getPointeeType());
                const auto& ct           = subType.getCanonicalType();
                const auto* plainSubType = ct.getTypePtrOrNull();

                return isa<SUB_T2>(plainSubType);
            }
        }
    }

    return false;
}
//-----------------------------------------------------------------------------

template<typename QT, typename SUB_T>
static bool HasTypePath(const QualType& t)
{
    if(const auto* lref = dyn_cast_or_null<QT>(t.getTypePtrOrNull())) {
        const auto subType = GetDesugarType(lref->getPointeeType());

        return isa<SUB_T>(subType);
    }

    return false;
}
//-----------------------------------------------------------------------------

std::string GetTypeNameAsParameter(const QualType& t, std::string_view varName, const Unqualified unqualified)
{
    const bool isFunctionPointer =
        HasTypeWithSubType<ReferenceType, FunctionProtoType>(t.getCanonicalType()) or
        HasTypeWithSubType<ReferenceType, PointerType, FunctionProtoType>(t.getCanonicalType());
    const bool isArrayRef = HasTypeWithSubType<ReferenceType, ArrayType>(t);
    // Special case for Issue81, auto returns an array-ref and to catch auto deducing an array (Issue106)
    const bool isAutoType             = (nullptr != dyn_cast_or_null<AutoType>(t.getTypePtrOrNull()));
    const auto pointerToArrayBaseType = isAutoType ? t->getContainedAutoType()->getDeducedType() : t;
    const bool isPointerToArray       = HasTypeWithSubType<PointerType, ArrayType>(pointerToArrayBaseType);
    // Only treat this as an array if it is a top-level arry. Typdef's et all can hide the arrayness.
    const bool isRawArrayType =
        t->isArrayType() and not(isa<TypedefType>(t) or isa<ElaboratedType>(t) or isa<UsingType>(t));

    std::string typeName = details::GetName(t, unqualified);

    // Sometimes we get char const[2]. If we directly insert the typename we end up with char const__var[2] which is not
    // a valid type name. Hence check for this condition and, if necessary, insert a space before __var.
    auto getSpaceOrEmpty = [&](const std::string_view& needle) -> std::string_view {
        if(not Contains(typeName, needle)) {
            return " ";
        }

        return {};
    };

    if(isRawArrayType and not t->isLValueReferenceType()) {
        const auto space = getSpaceOrEmpty(" ["sv);
        InsertBefore(typeName, "["sv, StrCat(space, varName));

    } else if(isArrayRef) {
        const bool             isRValueRef{HasTypeWithSubType<RValueReferenceType, ArrayType>(t)};
        const std::string_view contains{isRValueRef ? "(&&" : "(&"};

        if(Contains(typeName, contains)) {
            InsertAfter(typeName, contains, varName);
        } else {
            const std::string_view insertBefore{isRValueRef ? "&&[" : "&["};

            InsertBefore(typeName, insertBefore, "("sv);

            // check whether we are dealing with a function or an array
            if(Contains(typeName, contains)) {
                InsertAfter(typeName, contains, StrCat(varName, ")"sv));
            } else {
                InsertAfter(typeName, typeName, StrCat(" "sv, varName));
            }
        }

    } else if(isFunctionPointer) {
        const bool isRValueRef{HasTypeWithSubType<RValueReferenceType, FunctionProtoType>(t)};
        const auto contains{[&]() {
            if(isRValueRef) {
                return "(&&"sv;
            }

            else if(HasTypeWithSubType<LValueReferenceType, PointerType, FunctionProtoType>(t)) {
                return "(*&"sv;
            } else {
                return "(&"sv;
            }
        }()};

        if(Contains(typeName, contains)) {
            InsertAfter(typeName, contains, varName);
        } else {
            typeName += StrCat(" "sv, varName);
        }

    } else if(isa<MemberPointerType>(t)) {
        InsertAfter(typeName, "::*"sv, varName);

    } else if(isPointerToArray) {
        if(Contains(typeName, "(*"sv)) {
            InsertAfter(typeName, "(*"sv, varName);
        } else if(Contains(typeName, "*"sv)) {
            InsertBefore(typeName, "*"sv, "("sv);
            InsertAfter(typeName, "*"sv, StrCat(varName, ")"sv));
        }
    } else if(t->isFunctionPointerType()) {
        if(Contains(typeName, "(*"sv)) {
            InsertAfter(typeName, "(*"sv, varName);
        } else {
            typeName += StrCat(" "sv, varName);
        }
    } else if(HasTypePath<PointerType, ParenType>(t)) {
        InsertAfter(typeName, "(*"sv, varName);

    } else if(not isRawArrayType and not varName.empty()) {
        typeName += StrCat(" "sv, varName);
    }

    return typeName;
}
//-----------------------------------------------------------------------------

void AppendTemplateTypeParamName(OutputFormatHelper&         ofm,
                                 const TemplateTypeParmDecl* decl,
                                 const bool                  isParameter,
                                 const TemplateTypeParmType* type)
{
    if(decl) {
        if(const auto* typeConstraint = decl->getTypeConstraint(); typeConstraint and not isParameter) {
            StringStream sstream{};
            sstream.Print(*typeConstraint);

            ofm.Append(sstream.str(), " "sv);
        }
    }

    const auto depth = decl ? decl->getDepth() : type->getDepth();
    const auto index = decl ? decl->getIndex() : type->getIndex();

    ofm.Append("type_parameter_"sv, depth, "_"sv, index);
}
//-----------------------------------------------------------------------------

static bool IsTrivialStaticClassVarDecl(const DeclRefExpr& declRefExpr)
{
    if(const VarDecl* vd = GetVarDeclFromDeclRefExpr(declRefExpr)) {
        return IsTrivialStaticClassVarDecl(*vd);
    }

    return false;
}
//-----------------------------------------------------------------------------

APValue* GetEvaluatedValue(const VarDecl& varDecl)
{
    if((nullptr != varDecl.ensureEvaluatedStmt()) and varDecl.ensureEvaluatedStmt()->Value.isValid() and
       not varDecl.getInit()->isValueDependent()) {
        return varDecl.evaluateValue();
    }

    return nullptr;
}
//-----------------------------------------------------------------------------

bool IsEvaluatable(const VarDecl& varDecl)
{
    return (nullptr != GetEvaluatedValue(varDecl));
}
//-----------------------------------------------------------------------------

bool IsTrivialStaticClassVarDecl(const VarDecl& varDecl)
{
    // Should the VarDecl be evaluatable at compile-time, there is no additional guard added by the compiler.
    if(varDecl.isStaticLocal() and not IsEvaluatable(varDecl)) {
        if(const auto* cxxRecordDecl = varDecl.getType()->getAsCXXRecordDecl()) {
            if(cxxRecordDecl->hasNonTrivialDestructor() or cxxRecordDecl->hasNonTrivialDefaultConstructor()) {
                return true;
            }
        }
    }

    return false;
}
//-----------------------------------------------------------------------------

std::string GetName(const DeclRefExpr& declRefExpr)
{
    const auto* declRefDecl = declRefExpr.getDecl();
    std::string name{};
    const auto* declCtx = declRefDecl->getDeclContext();
    const bool  needsNamespace{NeedsNamespace(*declRefDecl, UseLexicalParent::No)};

    // get the namespace as well
    if(needsNamespace) {
        name = details::GetScope(declCtx);
    } else if(declRefExpr.hasQualifier()) {
        name = details::GetQualifiedName(*declRefDecl);
    }

    if(needsNamespace or not declRefExpr.hasQualifier()) {
        std::string plainName{GetPlainName(declRefExpr)};

        // try to handle the special case of a function local static with class type and non trivial destructor. In
        // this case, as we teared that variable apart, we need to adjust the variable named and add a reinterpret
        // cast
        if(IsTrivialStaticClassVarDecl(declRefExpr)) {
            if(const VarDecl* vd = GetVarDeclFromDeclRefExpr(declRefExpr)) {
                if(const auto* cxxRecordDecl = vd->getType()->getAsCXXRecordDecl()) {
                    plainName = StrCat("*"sv,
                                       kwReinterpretCast,
                                       "<"sv,
                                       GetName(vd->getType()),
                                       "*>("sv,
                                       BuildInternalVarName(plainName),
                                       ")"sv);
                }
            }
        }

        name.append(plainName);
    }

    name += GetCfrontOverloadedFunctionName(dyn_cast_or_null<FunctionDecl>(declRefDecl));

    return ScopeHandler::RemoveCurrentScope(GetTemplateParameterPackArgumentName(name, declRefDecl));
}
//-----------------------------------------------------------------------------

/*
 * Go deep in a Stmt if necessary and look to all childs for a DeclRefExpr.
 */
const DeclRefExpr* FindDeclRef(const Stmt* stmt)
{
    if(const auto* dref = dyn_cast_or_null<DeclRefExpr>(stmt)) {
        return dref;
    } else if(const auto* arrayInitExpr = dyn_cast_or_null<ArrayInitLoopExpr>(stmt)) {
        const auto* srcExpr = arrayInitExpr->getCommonExpr()->getSourceExpr();

        if(const auto* arrayDeclRefExpr = dyn_cast_or_null<DeclRefExpr>(srcExpr)) {
            return arrayDeclRefExpr;
        }
    } else if(const auto func = dyn_cast_or_null<CXXFunctionalCastExpr>(stmt)) {
        //        TODO(stmt, "");
    }

    if(stmt) {
        for(const auto* child : stmt->children()) {
            if(const auto* childRef = FindDeclRef(child)) {
                return childRef;
            }
        }
    }

    return nullptr;
}
//-----------------------------------------------------------------------------

std::string GetName(const VarDecl& VD)
{
    // Handle a special case of DecompositionDecl. A DecompositionDecl does not have a name. Hence we make one up from
    // the original name of the variable that is decomposed plus line number where the decomposition was written.
    if(const auto* decompositionDeclStmt = dyn_cast_or_null<DecompositionDecl>(&VD)) {
        const auto baseVarName{[&]() {
            if(const auto* declName = FindDeclRef(decompositionDeclStmt->getInit())) {
                std::string name = GetPlainName(*declName);

                if(Contains(name, kwOperator)) {
                    return std::string{kwOperator};
                }

                return name;
            }

            // We approached an unnamed decl. This happens for example like this: auto& [x, y] = Point{};
            return std::string{};
        }()};

        return BuildInternalVarName(baseVarName, decompositionDeclStmt->getBeginLoc(), GetSM(*decompositionDeclStmt));
    }

    std::string name{VD.getNameAsString()};

    return ScopeHandler::RemoveCurrentScope(GetTemplateParameterPackArgumentName(name, &VD));
}
//-----------------------------------------------------------------------------

static std::optional<bool> EvaluateAsBoolenCondition(const Expr& expr, const Decl& decl)
{
    bool r{false};

    if(expr.EvaluateAsBooleanCondition(r, decl.getASTContext())) {
        return {r};
    }

    return std::nullopt;
}
//-----------------------------------------------------------------------------

const std::string GetNoExcept(const FunctionDecl& decl)
{
    const auto* func = decl.getType()->castAs<FunctionProtoType>();

    if(func and func->hasNoexceptExceptionSpec() and not isUnresolvedExceptionSpec(func->getExceptionSpecType())) {
        std::string ret{kwSpaceNoexcept};

        if(const auto* expr = func->getNoexceptExpr()) {
            ret += "("sv;

            if(const auto value = EvaluateAsBoolenCondition(*expr, decl); value) {
                ret += details::ConvertToBoolString(*value);
            } else {
                OutputFormatHelper ofm{};
                CodeGenerator      cg{ofm};
                cg.InsertArg(expr);

                ret += ofm.GetString();
            }

            ret += ")"sv;
        }

        return ret;

    } else if(func and isUnresolvedExceptionSpec(func->getExceptionSpecType())) {
        // For special members the exception specification is unevaluated as long as the special member is unused.
        return StrCat(" "sv, kwCommentStart, kwSpaceNoexcept, kwSpaceCCommentEnd);
    }

    return {};
}
//-----------------------------------------------------------------------------

const std::string_view GetConst(const FunctionDecl& decl)
{
    if(const auto* methodDecl = dyn_cast_or_null<CXXMethodDecl>(&decl)) {
        if(methodDecl->isConst()) {
            return kwSpaceConst;
        }
    }

    return {};
}
//-----------------------------------------------------------------------------

std::string GetElaboratedTypeKeyword(const ElaboratedTypeKeyword keyword)
{
    std::string ret{TypeWithKeyword::getKeywordName(keyword)};

    if(not ret.empty()) {
        ret += ' ';
    }

    return ret;
}
//-----------------------------------------------------------------------------

uint64_t GetSize(const ConstantArrayType* arrayType)
{
    return arrayType->getSize().getZExtValue();
}
//-----------------------------------------------------------------------------

void StringStream::Print(const TemplateArgument& arg)
{
    arg.print(CppInsightsPrintingPolicy{}, *this, false);
}
//-----------------------------------------------------------------------------

void StringStream::Print(const TemplateSpecializationType& arg)
{
    arg.getTemplateName().print(*this, CppInsightsPrintingPolicy{}, TemplateName::Qualified::AsWritten);
}
//-----------------------------------------------------------------------------

void StringStream::Print(const TypeConstraint& arg)
{
    arg.print(*this, CppInsightsPrintingPolicy{});
}
//-----------------------------------------------------------------------------

void StringStream::Print(const StringLiteral& arg)
{
    arg.outputString(*this);
}
//-----------------------------------------------------------------------------

void StringStream::Print(const CharacterLiteral& arg)
{
    CharacterLiteral::print(arg.getValue(), arg.getKind(), *this);
}
//-----------------------------------------------------------------------------

template<class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

bool P0315Visitor::VisitLambdaExpr(const LambdaExpr* expr)
{
    mLambdaExpr = expr;

    std::visit(overloaded{
                   [&](OutputFormatHelper& ofm) { ofm.Append(GetLambdaName(*expr)); },
                   [&](CodeGenerator& cg) { cg.InsertArg(expr); },
               },
               mConsumer);

    return false;
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
