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

static const struct CppInsightsPrintingPolicy : PrintingPolicy
{
    CppInsightsPrintingPolicy()
    : PrintingPolicy{LangOptions{}}
    {
        adjustForCPlusPlus();
        SuppressUnwrittenScope = true;
        Alignof                = true;
        ConstantsAsWritten     = true;
        AnonymousTagLocations  = false;  // does remove filename and line for from lambdas in parameters
    }
} InsightsPrintingPolicy{};  // NOLINT
//-----------------------------------------------------------------------------

static const struct CppInsightsNoScopePrintingPolicy : CppInsightsPrintingPolicy
{
    CppInsightsNoScopePrintingPolicy()
    : CppInsightsPrintingPolicy{}
    {
        SuppressScope = true;
    }
} InsightsNoScopePrintingPolicy{};  // NOLINT
//-----------------------------------------------------------------------------

static std::string ReplaceAll(std::string str, const std::string& from, const std::string& to)
{
    if(Contains(str, "type-parameter-")) {
        for(size_t startPos = 0; (startPos = str.find(from, startPos)) != std::string::npos; startPos += to.length()) {
            str.replace(startPos, from.length(), to);
        }
    }

    return str;
}
//-----------------------------------------------------------------------------

std::string ReplaceDash(std::string&& str)
{
    return ReplaceAll(str, "-", "_");
}
//-----------------------------------------------------------------------------

STRONG_BOOL(SupressScope);

static const std::string GetAsCPPStyleString(const QualType& t, const SupressScope supressScope = SupressScope::No)
{
    if(SupressScope::Yes == supressScope) {
        return ReplaceDash(t.getAsString(InsightsNoScopePrintingPolicy));
    }

    return ReplaceDash(t.getAsString(InsightsPrintingPolicy));
}
//-----------------------------------------------------------------------------

std::string BuildInternalVarName(const std::string& varName)
{
    return StrCat("__", varName);
}
//-----------------------------------------------------------------------------

std::string BuildInternalVarName(const std::string& varName, const SourceLocation& loc, const SourceManager& sm)
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
    const auto  lineNo   = sm.getSpellingLineNumber(locBegin);
    const auto  columnNo = sm.getSpellingColumnNumber(locBegin);

    return StrCat(prefix, lineNo, "_", columnNo);
}
//-----------------------------------------------------------------------------

std::string GetLambdaName(const CXXRecordDecl& lambda)
{
    static const std::string lambdaPrefix{"__lambda_"};
    return MakeLineColumnName(lambda, lambdaPrefix);
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

    return str.str();
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

    return QualType::getAsString(splitted, InsightsPrintingPolicy);
}
//-----------------------------------------------------------------------------

namespace details {

static std::string GetQualifiedName(const NamedDecl& decl)
{
    std::string              name;
    llvm::raw_string_ostream stream(name);
    decl.printQualifiedName(stream, InsightsPrintingPolicy);

    return stream.str();
}
//-----------------------------------------------------------------------------

static std::string GetScope(const DeclContext* declCtx)
{
    std::string name{};

    if(!declCtx->isTranslationUnit() && !declCtx->isFunctionOrMethod()) {

        while(declCtx->isInlineNamespace()) {
            declCtx = declCtx->getParent();
        }

        if(declCtx->isNamespace() || declCtx->getParent()->isTranslationUnit()) {
            if(const auto* namespaceDecl = dyn_cast_or_null<NamespaceDecl>(declCtx)) {
                name = GetQualifiedName(*namespaceDecl);
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
    const QualType&    mType;
    const Unqualified  mUnqualified;
    OutputFormatHelper mData{};
    std::string        mDataAfter{};
    bool               mHasData{false};
    bool               mSkipSpace{false};

    bool HandleType(const TemplateTypeParmType* type)
    {
        if(nullptr == type->getIdentifier()) {
            mData.Append("type_parameter_", type->getDepth(), "_", type->getIndex());

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

    bool HandleType(const RecordType* type)
    {
        /// In case one of the template parameters is a lambda we need to insert the made up name.
        if(const auto* tt = dyn_cast_or_null<ClassTemplateSpecializationDecl>(type->getDecl())) {
            if(const auto* x = mType.getBaseTypeIdentifier()) {
                mData.Append(GetScope(type->getDecl()->getDeclContext()), x->getName());
                CodeGenerator codeGenerator{mData};
                codeGenerator.InsertTemplateArgs(*tt);

                return true;
            }
        } else if(const auto* cxxRecordDecl = type->getAsCXXRecordDecl()) {
            if(cxxRecordDecl->isLambda()) {
                mData.Append(GetLambdaName(*cxxRecordDecl));

                return true;
            }
        }

        return false;
    }

    bool HandleType(const AutoType* type)
    {
        if(not type->getDeducedType().isNull()) {
            return HandleType(type->getDeducedType().getTypePtrOrNull());
        }

        return false;
    }

    bool HandleType(const SubstTemplateTypeParmType* type)
    {
        // At least when coming from a TypedefType there can be a `const` attached to the actual type. To get it to
        // another round of `AddCVQulifiers` here.
        AddCVQualifiers(QualType(type, 0).getQualifiers());

        return HandleType(type->getReplacementType().getTypePtrOrNull());
    }

    bool HandleType(const ElaboratedType* type) { return HandleType(type->getNamedType().getTypePtrOrNull()); }
    bool HandleType(const TemplateSpecializationType* type)
    {
        if(type->getAsRecordDecl()) {
            // Only if it was some sort of "used" `RecordType` we continue here.
            if(HandleType(type->getAsRecordDecl()->getTypeForDecl())) {
                HandleType(type->getPointeeType().getTypePtrOrNull());
                return true;
            }
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
        for(unsigned i = 0, e = type->getNumParams(); i != e; ++i) {

            HandleType(type->getParamType(i).getTypePtrOrNull());

            if(needsComma) {
                mData.Append(", ");
            }
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
        mData.Append(type->getName(InsightsPrintingPolicy));

        if(not mSkipSpace) {
            mData.Append(' ');
        }

        return false;
    }

    bool HandleType(const TypedefType* type)
    {
        if(type->getDecl()) {
            return HandleType(type->getDecl()->getUnderlyingType().getTypePtrOrNull());
        }

        return HandleType(type->getPointeeType().getTypePtrOrNull());
    }

    bool HandleType(const ConstantArrayType* type)
    {
        const bool ret = HandleType(type->getElementType().getTypePtrOrNull());

        mData.Append("[", type->getSize().getZExtValue(), "]");

        return ret;
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

#undef HANDLE_TYPE
        return false;
    }

    void HandleTypeAfter(const Type* type)
    {
#define HANDLE_TYPE(t)                                                                                                 \
    if(isa<t>(type)) {                                                                                                 \
        HandleTypeAfter(dyn_cast_or_null<t>(type));                                                                    \
    }

        if(nullptr == type) {
            return;
        }

        HANDLE_TYPE(FunctionProtoType);
    }

    void AddCVQualifiers(const Qualifiers& quals)
    {
        if((Unqualified::No == mUnqualified) && not quals.empty()) {
            mData.Append(quals.getAsString());

            if(not mData.empty() && not mSkipSpace) {
                mData.Append(' ');
            }
        }
    }

public:
    SimpleTypePrinter(const QualType& qt, const Unqualified unqualified)
    : mType{qt}
    , mUnqualified{unqualified}
    {
    }

    std::string& GetString() { return mData.GetString(); }

    bool GetTypeString()
    {
        if(const SplitQualType splitted{mType.split()}; splitted.Quals.empty()) {
            AddCVQualifiers(mType->getPointeeType().getLocalQualifiers());
        } else {
            AddCVQualifiers(splitted.Quals);
        }

        mHasData = HandleType(mType.getTypePtrOrNull());
        mData.Append(mDataAfter);

        return mHasData;
    }
};
//-----------------------------------------------------------------------------

static std::string
GetNameInternal(const QualType& t, const Unqualified unqualified, const SupressScope supressScope = SupressScope::No)
{
    if(SimpleTypePrinter st{t, unqualified}; st.GetTypeString()) {
        return st.GetString();

    } else if(Unqualified::Yes == unqualified) {
        return GetAsCPPStyleString(t.getUnqualifiedType(), supressScope);
    }

    return GetAsCPPStyleString(t, supressScope);
}
//-----------------------------------------------------------------------------

static bool IsDecltypeType(const QualType& t)
{
    if(t.getTypePtrOrNull()) {
        if(isa<clang::DecltypeType>(t)) {
            return true;
        }
    }

    return false;
}
//-----------------------------------------------------------------------------

static std::string GetName(const QualType&    t,
                           const Unqualified  unqualified  = Unqualified::No,
                           const SupressScope supressScope = SupressScope::No)
{
    const auto  desugaredType = GetDesugarType(t);
    const auto* autoType      = desugaredType->getContainedAutoType();
    const bool  isAutoType{autoType && autoType->isSugared()};

    // Handle decltype(var)
    if(not isAutoType && IsDecltypeType(t)) {
        return GetNameInternal(desugaredType, unqualified, supressScope);
    }

    return GetNameInternal(t, unqualified, supressScope);
}
}  // namespace details
//-----------------------------------------------------------------------------

std::string GetName(const CXXRecordDecl& RD)
{
    if(RD.isLambda()) {
        return GetLambdaName(RD);
    }

    const auto* declCtx = RD.getDeclContext()->getLexicalParent();
    const bool  isFriend{(RD.getFriendObjectKind() != Decl::FOK_None)};
    const bool  hasNamespace{(declCtx && (declCtx->isNamespace() && not declCtx->isInlineNamespace()) &&
                             !declCtx->isTransparentContext() && !isFriend)};

    // get the namespace as well
    if(hasNamespace) {
        return ReplaceDash(details::GetQualifiedName(RD));
    }

    return ReplaceDash(RD.getNameAsString());
}
//-----------------------------------------------------------------------------

std::string GetName(const QualType& t, const Unqualified unqualified)
{
    return details::GetName(t, unqualified, SupressScope::No);
}
//-----------------------------------------------------------------------------

std::string GetUnqualifiedScopelessName(const Type* type)
{
    return details::GetName(QualType(type, 0), Unqualified::Yes, SupressScope::Yes);
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
    const bool isAutoType             = dyn_cast_or_null<AutoType>(t.getTypePtrOrNull());
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
            InsertAfter(typeName, contains, StrCat(varName, ")"));
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
        } else {
            typeName += StrCat(" ", varName);
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
    const bool  isFriend{(declRefDecl->getFriendObjectKind() != Decl::FOK_None)};
    const bool  hasNamespace{(declCtx && (declCtx->isNamespace() || declCtx->isInlineNamespace()) &&
                             !declCtx->isTransparentContext() && !isFriend)};

    // get the namespace as well
    if(hasNamespace) {
        name = details::GetScope(declCtx);
    } else if(declRefExpr.hasQualifier()) {
        name = details::GetQualifiedName(*declRefDecl);
    }

    if(hasNamespace || !declRefExpr.hasQualifier()) {
        std::string plainName{GetPlainName(declRefExpr)};

        // try to handle the special case of a function local static with class type and non trivial destructor. In
        // this case, as we teared that variable apart, we need to adjust the variable named and add a reinterpret
        // cast
        if(IsTrivialStaticClassVarDecl(declRefExpr)) {
            if(const VarDecl* vd = GetVarDeclFromDeclRefExpr(declRefExpr)) {
                if(const auto* cxxRecordDecl = vd->getType()->getAsCXXRecordDecl()) {
                    plainName = StrCat(
                        "*reinterpret_cast<", GetName(*cxxRecordDecl), "*>(", BuildInternalVarName(plainName), ")");
                }
            }
        }

        name.append(plainName);
    }

    return GetTemplateParameterPackArgumentName(name, declRefDecl);
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

    return GetTemplateParameterPackArgumentName(name, &VD);
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

}  // namespace clang::insights
