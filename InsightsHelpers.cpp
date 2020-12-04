/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "InsightsHelpers.h"
#include "ClangCompat.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "InsightsStaticStrings.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

ScopeHandler::ScopeStackType ScopeHandler::mGlobalStack{};  // NOLINT
std::string                  ScopeHandler::mScope{};        // NOLINT
//-----------------------------------------------------------------------------

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

            mScope.append(ofm.GetString());
        }

    } else if(const auto* namespaceDecl = dyn_cast_or_null<NamespaceDecl>(d)) {
        mScope.append(namespaceDecl->getNameAsString());
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
    auto findAndReplace = [&name](const std::string& scope) {
        if(const auto startPos = name.find(scope, 0); std::string::npos != startPos) {
            name.replace(startPos, scope.length(), "");
            return true;
        }

        return false;
    };

    // The default is that we can replace the entire scope. Suppose we are currently in N::X and having a symbol N::X::y
    // then N::X:: is removed.
    if(not findAndReplace(mScope)) {

        // A special case where we need to remove the scope without the last item.
        std::string tmp{mScope};
        tmp.resize(mGlobalStack.back().mLength);

        findAndReplace(tmp);
    }

    return name;
}
//-----------------------------------------------------------------------------

std::string GetPlainName(const DeclRefExpr& DRE)
{
    return ScopeHandler::RemoveCurrentScope(DRE.getNameInfo().getAsString());
}
//-----------------------------------------------------------------------------

STRONG_BOOL(InsightsSuppressScope);
//-----------------------------------------------------------------------------

static std::string GetUnqualifiedScopelessName(const Type* type, const InsightsSuppressScope supressScope);
//-----------------------------------------------------------------------------

struct CppInsightsPrintingPolicy : PrintingPolicy
{
    unsigned              CppInsightsUnqualified : 1;  // NOLINT
    InsightsSuppressScope CppInsightsSuppressScope;    // NOLINT

    CppInsightsPrintingPolicy(const Unqualified unqualified, const InsightsSuppressScope supressScope)
    : PrintingPolicy{LangOptions{}}
    {
        adjustForCPlusPlus();
        SuppressUnwrittenScope = true;
        Alignof                = true;
        ConstantsAsWritten     = true;
        AnonymousTagLocations  = false;  // does remove filename and line for from lambdas in parameters

        CppInsightsUnqualified   = (Unqualified::Yes == unqualified);
        CppInsightsSuppressScope = supressScope;
    }

    CppInsightsPrintingPolicy()
    : CppInsightsPrintingPolicy{Unqualified::No, InsightsSuppressScope::No}
    {
    }
};
//-----------------------------------------------------------------------------

namespace details {
static void BuildNamespace(std::string& fullNamespace, const NestedNameSpecifier* stmt)
{
    if(!stmt) {
        return;
    }

    if(const auto* prefix = stmt->getPrefix()) {
        BuildNamespace(fullNamespace, prefix);
    }

    switch(stmt->getKind()) {
        case NestedNameSpecifier::Identifier: fullNamespace.append(stmt->getAsIdentifier()->getName()); break;

        case NestedNameSpecifier::Namespace:
            if(stmt->getAsNamespace()->isAnonymousNamespace()) {
                return;
            }

            fullNamespace.append(stmt->getAsNamespace()->getName());
            break;

        case NestedNameSpecifier::NamespaceAlias: fullNamespace.append(stmt->getAsNamespaceAlias()->getName()); break;

        case NestedNameSpecifier::TypeSpecWithTemplate: fullNamespace.append("template "); [[fallthrough]];

        case NestedNameSpecifier::TypeSpec:
            fullNamespace.append(GetUnqualifiedScopelessName(stmt->getAsType(), InsightsSuppressScope::Yes));
            // The template parameters are already contained in the type we inserted above.
            break;

        default: break;
    }

    fullNamespace.append("::");
}
//-----------------------------------------------------------------------------
}  // namespace details

std::string GetNestedName(const NestedNameSpecifier* nns)
{
    std::string ret{};

    if(nns) {
        details::BuildNamespace(ret, nns);
    }

    return ret;
}
//-----------------------------------------------------------------------------

static const std::string GetAsCPPStyleString(const QualType& t, const CppInsightsPrintingPolicy& printingPolicy)
{
    return t.getAsString(printingPolicy);
}
//-----------------------------------------------------------------------------

std::string BuildInternalVarName(const std::string& varName)
{
    return StrCat("__", varName);
}
//-----------------------------------------------------------------------------

static std::string BuildInternalVarName(const std::string& varName, const SourceLocation& loc, const SourceManager& sm)
{
    const auto lineNo = sm.getSpellingLineNumber(loc);

    return StrCat(BuildInternalVarName(varName), lineNo);
}
//-----------------------------------------------------------------------------

static const LangOptions& GetLangOpts(const ast_matchers::MatchFinder::MatchResult& result)
{
    return result.Context->getLangOpts();
}
//-----------------------------------------------------------------------------

SourceLocation FindLocationAfterSemi(const SourceLocation                          loc,
                                     const ast_matchers::MatchFinder::MatchResult& result,
                                     RequireSemi                                   requireSemi)
{
    auto findLocation = [&](const tok::TokenKind tKind) {
        return clang::Lexer::findLocationAfterToken(loc, tKind, GetSM(result), GetLangOpts(result), false);
    };

    if(const auto locEnd{findLocation(tok::semi)}; locEnd.isValid()) {
        return locEnd;

    } else if(RequireSemi::Yes == requireSemi) {
        // if we do not find a ; then it can possibly be a brace init like this:
        // auto x {23};
        // Try to find the right curly which seems to also contain the semi.
        if(const auto locEnd2{findLocation(tok::r_brace)}; locEnd2.isValid()) {
            return locEnd2;
        }
        // if we do not find a ; then it can possibly be a paren init like this:
        // int x(23);
        // Try to find the right paren which seems to also contain the semi.
        else if(const auto locEnd3{findLocation(tok::r_paren)}; locEnd3.isValid()) {
            return locEnd3;
        }
    }

    return loc;
}
//-----------------------------------------------------------------------------

SourceRange GetSourceRangeAfterSemi(const SourceRange                             range,
                                    const ast_matchers::MatchFinder::MatchResult& result,
                                    RequireSemi                                   requireSemi)
{
    const SourceLocation locEnd = FindLocationAfterSemi(range.getEnd(), result, requireSemi);

    // Special handling for macro locations.
    const auto startLoc = [&] {
        if(range.getBegin().isMacroID()) {
            return GetSM(result).getImmediateExpansionRange(range.getBegin()).getBegin();
        }

        return range.getBegin();
    }();

    // Special handling for macro locations.
    const auto locEnd2 = [&] {
        if(locEnd.isMacroID()) {
            return GetSM(result).getImmediateExpansionRange(locEnd).getEnd();
        }

        return locEnd;
    }();

    return {startLoc, locEnd2};
}
//-----------------------------------------------------------------------------

void InsertBefore(std::string& source, const std::string& find, const std::string& replace)
{
    const std::string::size_type i = source.find(find, 0);

    if(std::string::npos != i) {
        source.insert(i, replace);
    }
}
//-----------------------------------------------------------------------------

static void InsertAfter(std::string& source, const std::string& find, const std::string& replace)
{
    const std::string::size_type i = source.find(find, 0);

    if(std::string::npos != i) {
        source.insert(i + find.length(), replace);
    }
}
//-----------------------------------------------------------------------------

static std::string MakeLineColumnName(const Decl& decl, const std::string& prefix)
{
    const auto& sm       = GetSM(decl);
    const auto  locBegin = GetBeginLoc(decl);
    // In case of a macro expansion the expansion(line/column) number gives a unique value.
    const auto lineNo = locBegin.isMacroID() ? sm.getExpansionLineNumber(locBegin) : sm.getSpellingLineNumber(locBegin);
    const auto columnNo =
        locBegin.isMacroID() ? sm.getExpansionColumnNumber(locBegin) : sm.getSpellingColumnNumber(locBegin);

    return StrCat(prefix, lineNo, "_", columnNo);
}
//-----------------------------------------------------------------------------

std::string GetLambdaName(const CXXRecordDecl& lambda)
{
    static const std::string lambdaPrefix{"__lambda_"};
    return MakeLineColumnName(lambda, lambdaPrefix);
}
//-----------------------------------------------------------------------------

static std::string GetAnonymStructOrUnionName(const CXXRecordDecl& cxxRecordDecl)
{
    static const std::string prefix{"__anon_"};
    return MakeLineColumnName(cxxRecordDecl, prefix);
}
//-----------------------------------------------------------------------------

std::string BuildRetTypeName(const Decl& decl)
{
    static const std::string retTypePrefix{"retType_"};
    return MakeLineColumnName(decl, retTypePrefix);
}
//-----------------------------------------------------------------------------

const QualType GetDesugarType(const QualType& QT)
{
    if(QT.getTypePtrOrNull()) {
        if(auto autoType = QT->getAs<clang::AutoType>()) {
            if(autoType->isSugared()) {
                return autoType->getDeducedType();
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
        str.append(".0");
    }

    return std::string(str.str());
}
//-----------------------------------------------------------------------------

static const VarDecl* GetVarDeclFromDeclRefExpr(const DeclRefExpr& declRefExpr)
{
    const auto* valueDecl = declRefExpr.getDecl();

    return dyn_cast_or_null<VarDecl>(valueDecl);
}
//-----------------------------------------------------------------------------

std::string GetNameAsWritten(const QualType& t)
{
    SplitQualType splitted = t.split();

    return ScopeHandler::RemoveCurrentScope(QualType::getAsString(splitted, CppInsightsPrintingPolicy{}));
}
//-----------------------------------------------------------------------------

// own implementation due to lambdas
std::string GetDeclContext(const DeclContext* ctx)
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
            if(nd->isAnonymousNamespace() || nd->isInline()) {
                continue;
            }

            mOutputFormatHelper.Append(nd->getNameAsString());

        } else if(const auto* rd = dyn_cast<RecordDecl>(declContext)) {
            if(!rd->getIdentifier()) {
                continue;
            }

            mOutputFormatHelper.Append(rd->getNameAsString());

        } else if(dyn_cast<FunctionDecl>(declContext)) {
            continue;

        } else if(const auto* ed = dyn_cast<EnumDecl>(declContext)) {
            if(!ed->isScoped()) {
                continue;
            }

            mOutputFormatHelper.Append(ed->getNameAsString());

        } else {
            mOutputFormatHelper.Append(cast<NamedDecl>(declContext)->getNameAsString());
        }

        mOutputFormatHelper.Append("::");
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

    scope += decl.getName().str();

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

    if(!declCtx->isTranslationUnit() && !declCtx->isFunctionOrMethod()) {

        while(declCtx->isInlineNamespace()) {
            declCtx = declCtx->getParent();
        }

        if(declCtx->isNamespace() || declCtx->getParent()->isTranslationUnit()) {
            if(const auto* namedDecl = dyn_cast_or_null<NamedDecl>(declCtx)) {
                name = GetQualifiedName(*namedDecl, removeCurrentScope);
                name.append("::");
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
    std::string                      mScope{};  //!< A scope coming from an ElaboratedType which is used for a
                                                //!< ClassTemplateSpecializationDecl if there is no other scope

    bool HandleType(const TemplateTypeParmType* type)
    {
        TemplateTypeParmDecl* decl = type->getDecl();

        if((nullptr == type->getIdentifier()) ||
           (decl && decl->isImplicit()) /* this fixes auto operator()(type_parameter_0_0 container) const */) {

            AppendTemplateTypeParamName(mData, decl, true, type);

            return true;
        }

        return false;
    }

    bool HandleType(const LValueReferenceType* type)
    {
        mDataAfter += " &";

        return HandleType(type->getPointeeType().getTypePtrOrNull());
    }

    bool HandleType(const RValueReferenceType* type)
    {
        mDataAfter += " &&";

        return HandleType(type->getPointeeType().getTypePtrOrNull());
    }

    bool HandleType(const PointerType* type)
    {
        mDataAfter += " *";

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
                if((InsightsSuppressScope::Yes == mPrintingPolicy.CppInsightsSuppressScope) || scope.empty()) {
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
        mScope = GetNestedName(type->getQualifier());

        const bool ret = HandleType(type->getNamedType().getTypePtrOrNull());

        mScope.clear();

        return ret;
    }

    bool HandleType(const DependentTemplateSpecializationType* type)
    {
        mData.Append(GetElaboratedTypeKeyword(type->getKeyword()),
                     GetNestedName(type->getQualifier()),
                     "template ",
                     type->getIdentifier()->getName().str());

        CodeGenerator codeGenerator{mData};
        codeGenerator.InsertTemplateArgs(*type);

        return true;
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

                if(Contains(sstream.str(), "type-parameter")) {
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

        mData.Append("::*)");

        HandleTypeAfter(type->getPointeeType().getTypePtrOrNull());

        return ret;
    }

    bool HandleType(const FunctionProtoType* type) { return HandleType(type->getReturnType().getTypePtrOrNull()); }

    void HandleTypeAfter(const FunctionProtoType* type)
    {
        mData.Append('(');
        OnceFalse needsComma{};

        mSkipSpace = true;
        for(const auto& t : type->getParamTypes()) {
            if(needsComma) {
                mData.Append(", ");
            }

            HandleType(t.getTypePtrOrNull());
        }

        mSkipSpace = false;

        mData.Append(')');

#if IS_CLANG_NEWER_THAN(8)
        if(not type->getMethodQuals().empty()) {
            mData.Append(" ", type->getMethodQuals().getAsString());
        }
#else
        if(not type->getTypeQuals().empty()) {
            mData.Append(" ", type->getTypeQuals().getAsString());
        }
#endif

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
            if(Contains(decl->getQualifiedNameAsString(), "type-parameter")) {
                auto* identifierInfo = decl->getIdentifier();
                mData.Append(identifierInfo->getName().str());

                return true;
            }

            return HandleType(decl->getUnderlyingType().getTypePtrOrNull());
        }

        return HandleType(type->getPointeeType().getTypePtrOrNull());
    }

    bool HandleType(const ConstantArrayType* type)
    {
        const bool ret = HandleType(type->getElementType().getTypePtrOrNull());

        mData.Append("[", type->getSize().getZExtValue(), "]");

        return ret;
    }

    bool HandleType(const PackExpansionType* type)
    {
        const bool ret = HandleType(type->getPattern().getTypePtrOrNull());

        if(ret) {
            mData.Append("...");
        }

        return ret;
    }

    bool HandleType(const DecltypeType* type)
    {
        // A DecltypeType in a template definition is unevaluated and refers ti itself. This check ensures, that in such
        // a situation no expansion is performed.
        if(const auto* subType = type->desugar().getTypePtrOrNull(); not isa<DecltypeType>(subType)) {
            const bool skipSpace{mSkipSpace};
            mSkipSpace = true;

            HandleType(type->desugar().getTypePtrOrNull());

            mSkipSpace = skipSpace;

            // if we hit a DecltypeType always use the expanded version to support things like a DecltypeType wrapped in
            // an LValueReferenceType
            return true;
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
        if((false == mPrintingPolicy.CppInsightsUnqualified) && not quals.empty()) {
            mData.Append(quals.getAsString());

            if(not mData.empty() && not mSkipSpace) {
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

static std::string GetName(const QualType&             t,
                           const Unqualified           unqualified  = Unqualified::No,
                           const InsightsSuppressScope supressScope = InsightsSuppressScope::No)
{
    const CppInsightsPrintingPolicy printingPolicy{unqualified, supressScope};

    if(SimpleTypePrinter st{t, printingPolicy}; st.GetTypeString()) {
        return ScopeHandler::RemoveCurrentScope(st.GetString());

    } else if(true == printingPolicy.CppInsightsUnqualified) {
        return ScopeHandler::RemoveCurrentScope(GetAsCPPStyleString(t.getUnqualifiedType(), printingPolicy));
    }

    return ScopeHandler::RemoveCurrentScope(GetAsCPPStyleString(t, printingPolicy));
}
}  // namespace details
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
    const bool neitherTransparentNorFriend{not declCtx->isTransparentContext() && not isFriend};

    if(UseLexicalParent::Yes == useLexicalParent) {
        return (declCtx->isNamespace() && not declCtx->isInlineNamespace()) && neitherTransparentNorFriend;
    }

    return (declCtx->isNamespace() || declCtx->isInlineNamespace()) && neitherTransparentNorFriend;
}
//-----------------------------------------------------------------------------

std::string GetName(const NamedDecl& nd)
{
    std::string name{};

    if(NeedsNamespace(nd, UseLexicalParent::No)) {
        name = details::GetScope(nd.getDeclContext(), details::RemoveCurrentScope::No);
    }

    name += nd.getNameAsString();

    return ScopeHandler::RemoveCurrentScope(name);
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

    if(auto&& name = RD.getNameAsString(); not name.empty()) {
        ret += RD.getNameAsString();

    } else {
        ret += GetAnonymStructOrUnionName(RD);
    }

    return ScopeHandler::RemoveCurrentScope(ret);
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

template<typename QT, typename SUB_T>
static bool HasTypeWithSubType(const QualType& t)
{
    if(const auto* lref = dyn_cast_or_null<QT>(t.getTypePtrOrNull())) {
        const auto  subType      = GetDesugarType(lref->getPointeeType());
        const auto& ct           = subType.getCanonicalType();
        const auto* plainSubType = ct.getTypePtrOrNull();

        return isa<SUB_T>(plainSubType);
    }

    return false;
}
//-----------------------------------------------------------------------------

std::string GetTypeNameAsParameter(const QualType& t, const std::string& varName, const Unqualified unqualified)
{
    const bool isFunctionPointer = HasTypeWithSubType<ReferenceType, FunctionProtoType>(t);
    const bool isArrayRef        = HasTypeWithSubType<ReferenceType, ArrayType>(t);
    // Special case for Issue81, auto returns an array-ref and to catch auto deducing an array (Issue106)
    const bool isAutoType             = (nullptr != dyn_cast_or_null<AutoType>(t.getTypePtrOrNull()));
    const auto pointerToArrayBaseType = isAutoType ? t->getContainedAutoType()->getDeducedType() : t;
    const bool isPointerToArray       = HasTypeWithSubType<PointerType, ArrayType>(pointerToArrayBaseType);

    std::string typeName = details::GetName(t, unqualified);

    // Sometimes we get char const[2]. If we directly insert the typename we end up with char const__var[2] which is not
    // a valid type name. Hence check for this condition and, if necessary, insert a space before __var.
    auto getSpaceOrEmpty = [&](const std::string& needle) -> std::string {
        if(not Contains(typeName, needle)) {
            return " ";
        }

        return "";
    };

    if(t->isArrayType() && !t->isLValueReferenceType()) {
        std::string space = getSpaceOrEmpty(" [");
        InsertBefore(typeName, "[", StrCat(space, varName));

    } else if(isArrayRef) {
        const bool        isRValueRef{HasTypeWithSubType<RValueReferenceType, ArrayType>(t)};
        const std::string contains{isRValueRef ? "(&&" : "(&"};

        if(Contains(typeName, contains)) {
            InsertAfter(typeName, contains, varName);
        } else {
            const std::string insertBefore{isRValueRef ? "&&[" : "&["};

            InsertBefore(typeName, insertBefore, "(");

            // check whether we are dealing with a function or an array
            if(Contains(typeName, contains)) {
                InsertAfter(typeName, contains, StrCat(varName, ")"));
            } else {
                InsertAfter(typeName, typeName, StrCat(" ", varName));
            }
        }

    } else if(isFunctionPointer) {
        const bool        isRValueRef{HasTypeWithSubType<RValueReferenceType, FunctionProtoType>(t)};
        const std::string contains{isRValueRef ? "(&&" : "(&"};

        if(Contains(typeName, contains)) {
            InsertAfter(typeName, contains, varName);
        } else {
            typeName += StrCat(" ", varName);
        }

    } else if(isPointerToArray) {
        if(Contains(typeName, "(*")) {
            InsertAfter(typeName, "(*", varName);
        } else if(Contains(typeName, "*")) {
            InsertBefore(typeName, "*", "(");
            InsertAfter(typeName, "*", StrCat(varName, ")"));
        }
    } else if(t->isFunctionPointerType()) {
        if(Contains(typeName, "(*")) {
            InsertAfter(typeName, "(*", varName);
        } else {
            typeName += StrCat(" ", varName);
        }

    } else if(!t->isArrayType() && !varName.empty()) {
        typeName += StrCat(" ", varName);
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
        if(const auto* typeConstraint = decl->getTypeConstraint(); typeConstraint && not isParameter) {
            StringStream sstream{};
            sstream.Print(*typeConstraint);

            ofm.Append(sstream.str(), " ");
        }
    }

    const auto depth = [&] { return decl ? decl->getDepth() : type->getDepth(); }();
    const auto index = [&] { return decl ? decl->getIndex() : type->getIndex(); }();

    ofm.Append("type_parameter_", depth, "_", index);
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

bool IsTrivialStaticClassVarDecl(const VarDecl& varDecl)
{
    if(varDecl.isStaticLocal()) {
        if(const auto* cxxRecordDecl = varDecl.getType()->getAsCXXRecordDecl()) {
            if(cxxRecordDecl->hasNonTrivialDestructor() || cxxRecordDecl->hasNonTrivialDefaultConstructor()) {
                return true;
            }
        }
    }

    return false;
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
template<typename T>
static std::string GetTemplateParameterPackArgumentName(std::string& name, const T* decl)
{
    if(const auto* parmVarDecl = dyn_cast_or_null<ParmVarDecl>(decl)) {
        if(const auto& originalType = parmVarDecl->getOriginalType(); not originalType.isNull()) {
            if(const auto* substTemplateTypeParmType = GetSubstTemplateTypeParmType(originalType.getTypePtrOrNull())) {
                if(substTemplateTypeParmType->getReplacedParameter()->isParameterPack()) {
                    name = StrCat(BuildInternalVarName(name), parmVarDecl->getFunctionScopeIndex());
                }
            }
        }
    }

    return name;
}
//-----------------------------------------------------------------------------

std::string GetName(const DeclRefExpr& declRefExpr)
{
    std::string name{};
    const auto* declRefDecl = declRefExpr.getDecl();
    const auto* declCtx     = declRefDecl->getDeclContext();
    const bool  needsNamespace{NeedsNamespace(*declRefDecl, UseLexicalParent::No)};

    // get the namespace as well
    if(needsNamespace) {
        name = details::GetScope(declCtx);
    } else if(declRefExpr.hasQualifier()) {
        name = details::GetQualifiedName(*declRefDecl);
    }

    if(needsNamespace || not declRefExpr.hasQualifier()) {
        std::string plainName{GetPlainName(declRefExpr)};

        // try to handle the special case of a function local static with class type and non trivial destructor. In
        // this case, as we teared that variable apart, we need to adjust the variable named and add a reinterpret
        // cast
        if(IsTrivialStaticClassVarDecl(declRefExpr)) {
            if(const VarDecl* vd = GetVarDeclFromDeclRefExpr(declRefExpr)) {
                if(const auto* cxxRecordDecl = vd->getType()->getAsCXXRecordDecl()) {
                    plainName = StrCat(
                        "*reinterpret_cast<", GetName(vd->getType()), "*>(", BuildInternalVarName(plainName), ")");
                }
            }
        }

        name.append(plainName);
    }

    return ScopeHandler::RemoveCurrentScope(GetTemplateParameterPackArgumentName(name, declRefDecl));
}
//-----------------------------------------------------------------------------

/*
 * Go deep in a Stmt if necessary and look to all childs for a DeclRefExpr.
 */
static const DeclRefExpr* FindDeclRef(const Stmt* stmt)
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

                const std::string operatorName{"operator"};
                if(Contains(name, operatorName)) {
                    return operatorName;
                }

                return name;
            }

            // We approached an unnamed decl. This happens for example like this: auto& [x, y] = Point{};
            return std::string{};
        }()};

        return {BuildInternalVarName(baseVarName, GetBeginLoc(decompositionDeclStmt), GetSM(*decompositionDeclStmt))};
    }

    std::string name{VD.getNameAsString()};

    return ScopeHandler::RemoveCurrentScope(GetTemplateParameterPackArgumentName(name, &VD));
}
//-----------------------------------------------------------------------------

static bool EvaluateAsBoolenCondition(const Expr& expr, const Decl& decl)
{
    bool r{false};

    expr.EvaluateAsBooleanCondition(r, decl.getASTContext());

    return r;
}
//-----------------------------------------------------------------------------

const std::string GetNoExcept(const FunctionDecl& decl)
{
    const auto* func = decl.getType()->castAs<FunctionProtoType>();

    if(func && func->hasNoexceptExceptionSpec()) {
        std::string ret{kwSpaceNoexcept};

        if(const auto* expr = func->getNoexceptExpr()) {
            const auto value = [&] {
                if(const auto* boolExpr = dyn_cast_or_null<CXXBoolLiteralExpr>(expr)) {
                    return boolExpr->getValue();

                } else if(const auto* bExpr = dyn_cast_or_null<BinaryOperator>(expr)) {
                    return EvaluateAsBoolenCondition(*bExpr, decl);

                } else if(const auto* cExpr = dyn_cast_or_null<ConstantExpr>(expr)) {
                    return EvaluateAsBoolenCondition(*cExpr, decl);
                }

                Error(expr, "INSIGHTS: Unexpected noexcept expr\n");

                return false;
            }();

            ret += "(";

            if(value) {
                ret += "true";
            } else {
                ret += "false";
            }

            ret += ")";
        }

        return ret;
    }

    return {};
}
//-----------------------------------------------------------------------------

const char* GetConst(const FunctionDecl& decl)
{
    if(const auto* methodDecl = dyn_cast_or_null<CXXMethodDecl>(&decl)) {
        if(methodDecl->isConst()) {
            return kwSpaceConst;
        }
    }

    return "";
}
//-----------------------------------------------------------------------------

std::string GetElaboratedTypeKeyword(const ElaboratedTypeKeyword keyword)
{
    if(ETK_None != keyword) {
        return TypeWithKeyword::getKeywordName(keyword).str() + " ";
    }

    return {};
}
//-----------------------------------------------------------------------------

void StringStream::Print(const TemplateArgument& arg)
{
    arg.print(CppInsightsPrintingPolicy{}, *this);
}
//-----------------------------------------------------------------------------

void StringStream::Print(const TemplateSpecializationType& arg)
{
    arg.getTemplateName().print(*this, CppInsightsPrintingPolicy{}, true);
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

}  // namespace clang::insights
