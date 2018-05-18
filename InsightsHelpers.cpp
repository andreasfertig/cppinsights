/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "InsightsHelpers.h"
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

    return StrCat(BuildInternalVarName(varName), std::to_string(lineNo));
}
//-----------------------------------------------------------------------------

static const LangOptions& GetLangOpts(const ast_matchers::MatchFinder::MatchResult& result)
{
    return result.Context->getLangOpts();
}
//-----------------------------------------------------------------------------

SourceLocation FindLocationAfterToken(const SourceLocation                          loc,
                                      const tok::TokenKind                          tokenKind,
                                      const ast_matchers::MatchFinder::MatchResult& result)
{
    const SourceLocation locEnd =
        clang::Lexer::findLocationAfterToken(loc, tokenKind, GetSM(result), GetLangOpts(result), false);

    if(locEnd.isValid()) {
        return locEnd;
    }

    return loc;
}
//-----------------------------------------------------------------------------

SourceRange GetSourceRangeAfterToken(const SourceRange                             range,
                                     const tok::TokenKind                          tokenKind,
                                     const ast_matchers::MatchFinder::MatchResult& result)
{
    SourceLocation locEnd = FindLocationAfterToken(range.getEnd(), tokenKind, result);

    if(locEnd.isInvalid()) {
        locEnd = range.getEnd();
    }

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
    const auto               lineNo   = sm.getSpellingLineNumber(lambda.getLocStart());
    const auto               columnNo = sm.getSpellingColumnNumber(lambda.getLocStart());

    return StrCat(lambdaPrefix, std::to_string(lineNo), "_", std::to_string(columnNo));
}
//-----------------------------------------------------------------------------

const QualType GetDesugarType(const QualType& QT)
{
    if(QT.getTypePtrOrNull()) {
        if(auto declType = QT->getAs<clang::DecltypeType>()) {
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

const VarDecl* GetVarDeclFromDeclRefExpr(const DeclRefExpr* declRefExpr)
{
    if(nullptr != declRefExpr) {
        const auto* valueDecl = declRefExpr->getDecl();

        return dyn_cast_or_null<VarDecl>(valueDecl);
    }

    return nullptr;
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
    if(t.getTypePtrOrNull()) {
        const std::string cvqStr{t.getQualifiers().getAsString()};
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

std::string GetTypeNameAsParameter(const QualType& t, const std::string& varName, const Unqualified unqualified)
{
    const bool isArrayRef = [&]() {
        if(const auto* lref = dyn_cast_or_null<LValueReferenceType>(t.getTypePtrOrNull())) {
            if(const auto* pt = dyn_cast_or_null<ParenType>(lref->getPointeeType().getTypePtrOrNull())) {
                if(pt->getInnerType()->isArrayType()) {
                    return true;
                }
            } else if(isa<ConstantArrayType>(lref->getPointeeType().getTypePtrOrNull())) {
                return true;
            }
        }
        return false;
    }();

    std::string typeName = details::GetName(t, unqualified);

    if(t->isArrayType() && t->isLValueReferenceType()) {
        InsertBefore(typeName, "[", StrCat("(&", varName, ")"));
    } else if(t->isArrayType() && !t->isLValueReferenceType()) {
        InsertBefore(typeName, "[", varName);
    } else if(isArrayRef) {
        InsertAfter(typeName, "(&", varName);
    } else if(!t->isArrayType() && !varName.empty()) {
        typeName += StrCat(" ", varName);
    }

    return typeName;
}
//-----------------------------------------------------------------------------

static bool IsTrivialStaticClassVarDecl(const DeclRefExpr& declRefExpr)
{
    if(const VarDecl* VD = GetVarDeclFromDeclRefExpr(&declRefExpr)) {
        return IsTrivialStaticClassVarDecl(*VD);
    }

    return false;
}
//-----------------------------------------------------------------------------

bool IsTrivialStaticClassVarDecl(const VarDecl& varDecl)
{
    if(varDecl.isStaticLocal()) {
        if(const auto* cxxRecordDecl = varDecl.getType()->getAsCXXRecordDecl()) {
            if(cxxRecordDecl->hasNonTrivialDestructor()) {
                return true;
            }
        }
    }

    return false;
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
            if(const VarDecl* VD = GetVarDeclFromDeclRefExpr(&declRefExpr)) {
                if(const auto* cxxRecordDecl = VD->getType()->getAsCXXRecordDecl()) {
                    plainName = StrCat(
                        "*reinterpret_cast<", GetName(*cxxRecordDecl), "*>(", BuildInternalVarName(plainName), ")");
                }
            }
        }

        name.append(plainName);
    }

    return name;
}
//-----------------------------------------------------------------------------

const char* GetNoExcept(const FunctionDecl& decl)
{
    const auto* func = decl.getType()->castAs<FunctionProtoType>();

    if(func && func->hasNoexceptExceptionSpec()) {
        return kwSpaceNoexcept;
    }

    return "";
}
//-----------------------------------------------------------------------------

const char* GetConst(const CXXMethodDecl& decl)
{
    if(decl.isConst()) {
        return kwSpaceConst;
    }

    return "";
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
