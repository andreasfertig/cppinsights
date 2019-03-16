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
} InsightsPrintingPolicy{};
//-----------------------------------------------------------------------------

static const std::string GetAsCPPStyleString(const QualType& t)
{
    return t.getAsString(InsightsPrintingPolicy);
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

SourceLocation FindLocationAfterSemi(const SourceLocation loc, const ast_matchers::MatchFinder::MatchResult& result)
{
    const SourceLocation locEnd =
        clang::Lexer::findLocationAfterToken(loc, tok::semi, GetSM(result), GetLangOpts(result), false);

    if(locEnd.isValid()) {
        return locEnd;
    }

    return loc;
}
//-----------------------------------------------------------------------------

SourceRange GetSourceRangeAfterSemi(const SourceRange range, const ast_matchers::MatchFinder::MatchResult& result)
{
    const SourceLocation locEnd = FindLocationAfterSemi(range.getEnd(), result);

    return {range.getBegin(), locEnd};
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

std::string GetLambdaName(const CXXRecordDecl& lambda)
{
    static const std::string lambdaPrefix{"__lambda_"};
    const auto&              sm       = GetSM(lambda);
    const auto               locBegin = GetBeginLoc(lambda);
    const auto               lineNo   = sm.getSpellingLineNumber(locBegin);
    const auto               columnNo = sm.getSpellingColumnNumber(locBegin);

    return StrCat(lambdaPrefix, lineNo, "_", columnNo);
}
//-----------------------------------------------------------------------------

std::string BuildRetTypeName(const Decl& decl)
{
    static const std::string retTypePrefix{"retType_"};
    const auto&              sm       = GetSM(decl);
    const auto               locBegin = GetBeginLoc(decl);
    const auto               lineNo   = sm.getSpellingLineNumber(locBegin);
    const auto               columnNo = sm.getSpellingColumnNumber(locBegin);

    return StrCat(retTypePrefix, lineNo, "_", columnNo);
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

static std::string GetNameInternal(const QualType& t, const Unqualified unqualified)
{
    if(const auto* memberPointerType = t->getAs<MemberPointerType>()) {
        if(const auto* recordType2 = dyn_cast_or_null<RecordType>(memberPointerType->getClass())) {
            if(const auto* decl = recordType2->getDecl()) {
                if(const auto* cxxRecordDecl = dyn_cast_or_null<CXXRecordDecl>(decl)) {
                    if(cxxRecordDecl->isLambda()) {
                        std::string result{GetAsCPPStyleString(t)};

                        static const std::string clangLambdaName{"(lambda)::*"};
                        if(auto pos = result.find(clangLambdaName); std::string::npos != pos) {
                            std::string lambdaName = GetLambdaName(*cxxRecordDecl);
                            lambdaName += "::*";

                            result.replace(pos, clangLambdaName.length(), lambdaName);
                            return result;
                        }
                    }
                }
            }
        }
    }

    if(t.getTypePtrOrNull()) {
        std::string       refOrPointer{};
        const RecordType* recordType = [&]() -> const RecordType* {
            const auto& ct  = t.getCanonicalType();
            const auto* ctp = ct.getTypePtrOrNull();

            if(const auto* sta = dyn_cast_or_null<RValueReferenceType>(ctp)) {
                refOrPointer = "&&";
                return dyn_cast_or_null<RecordType>(sta->getPointeeTypeAsWritten().getTypePtrOrNull());

            } else if(const auto* ref = dyn_cast_or_null<ReferenceType>(ctp)) {
                refOrPointer = "&";
                return dyn_cast_or_null<RecordType>(ref->getPointeeTypeAsWritten().getTypePtrOrNull());

            } else if(const auto* ptr = dyn_cast_or_null<PointerType>(ctp)) {
                refOrPointer = "*";
                return dyn_cast_or_null<RecordType>(ptr->getPointeeType().getTypePtrOrNull());
            }

            return dyn_cast_or_null<RecordType>(ctp);
        }();

        if(recordType) {
            if(const auto* decl = recordType->getDecl()) {
                const std::string cvqStr{[&]() {
                    if(const auto* refType = dyn_cast_or_null<ReferenceType>(t.getTypePtrOrNull())) {
                        return refType->getPointeeTypeAsWritten().getLocalQualifiers().getAsString();
                    }

                    return t.getCanonicalType().getLocalQualifiers().getAsString();
                }()};

                if(const auto* tt = dyn_cast_or_null<ClassTemplateSpecializationDecl>(decl)) {
                    if(const auto* x = t.getBaseTypeIdentifier()) {
                        OutputFormatHelper ofm{};

                        if((Unqualified::No == unqualified) && !cvqStr.empty()) {
                            ofm.Append(cvqStr, " ");
                        }

                        ofm.Append(GetScope(decl->getDeclContext()), x->getName());
                        CodeGenerator codeGenerator{ofm};
                        codeGenerator.InsertTemplateArgs(*tt);

                        if(!refOrPointer.empty()) {
                            ofm.Append(" ", refOrPointer);
                        }

                        return ofm.GetString();
                    }
                } else if(const auto* cxxRecordDecl = dyn_cast_or_null<CXXRecordDecl>(decl)) {
                    if(cxxRecordDecl->isLambda()) {
                        std::string result{cvqStr};
                        if(!cvqStr.empty()) {
                            result += ' ';
                        }

                        result += GetLambdaName(*cxxRecordDecl);
                        if(!refOrPointer.empty()) {
                            result += ' ';
                            result += refOrPointer;
                        }

                        return result;
                    }
                }
            }
        }
    }

    if(Unqualified::Yes == unqualified) {
        return GetAsCPPStyleString(t.getUnqualifiedType());
    }

    return GetAsCPPStyleString(t);
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

static std::string GetName(const QualType& t, const Unqualified unqualified = Unqualified::No)
{
    const auto  t2 = GetDesugarType(t);
    const auto* at = t2->getContainedAutoType();

    if(at && at->isSugared()) {
        const auto dt = at->getDeducedType();

        // treat LValueReference special at this point. This means we are coming from auto&& and it decayed to an
        // l-value reference.
        if(dt->isLValueReferenceType()) {
            return GetNameInternal(dt, unqualified);
        }

    } else if(IsDecltypeType(t)) {  // Handle decltype(var)
        return GetNameInternal(t2, unqualified);
    }

    return GetNameInternal(t, unqualified);
}
}  // namespace details
//-----------------------------------------------------------------------------

std::string GetName(const QualType& t, const Unqualified unqualified)
{
    return details::GetName(t, unqualified);
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
        InsertAfter(typeName, "(*", varName);
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

std::string GetName(const VarDecl& VD)
{
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

                } else if(const auto* noExceptExpr = dyn_cast_or_null<CXXNoexceptExpr>(expr)) {
                    return noExceptExpr->getValue();

                } else if(const auto* bExpr = dyn_cast_or_null<BinaryOperator>(expr)) {
                    return EvaluateAsBoolenCondition(*bExpr, decl);

                } else if(const auto* coExpr = dyn_cast_or_null<ConditionalOperator>(expr)) {
                    return EvaluateAsBoolenCondition(*coExpr, decl);

#if IS_CLANG_NEWER_THAN(7)
                } else if(const auto* cExpr = dyn_cast_or_null<ConstantExpr>(expr)) {
                    return EvaluateAsBoolenCondition(*cExpr, decl);
#endif
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
