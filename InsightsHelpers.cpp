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
    const auto               locBegin = GetBeginLoc(lambda);
    const auto               lineNo   = sm.getSpellingLineNumber(locBegin);
    const auto               columnNo = sm.getSpellingColumnNumber(locBegin);

    return StrCat(lambdaPrefix, std::to_string(lineNo), "_", std::to_string(columnNo));
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
    SplitQualType T_split = t.split();

    return QualType::getAsString(T_split, InsightsPrintingPolicy);
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

                    return t.getLocalQualifiers().getAsString();
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

template<typename QT, typename T>
static bool TestPlainSubType(const QualType& t, T&& lambda)
{
    if(const auto* lref = dyn_cast_or_null<QT>(t.getTypePtrOrNull())) {
        const auto  subType      = GetDesugarType(lref->getPointeeType());
        const auto& ct           = subType.getCanonicalType();
        const auto* plainSubType = ct.getTypePtrOrNull();

        return lambda(plainSubType);
    }

    return false;
}
//-----------------------------------------------------------------------------

std::string GetTypeNameAsParameter(const QualType& t, const std::string& varName, const Unqualified unqualified)
{
    const bool isFunctionPointer = TestPlainSubType<LValueReferenceType>(t, [&](auto* plainSubType) {
        if(isa<FunctionProtoType>(plainSubType)) {
            return true;
        }
        return false;
    });

    const bool isArrayRef = TestPlainSubType<LValueReferenceType>(t, [&](auto* plainSubType) {
        if(const auto* pt = dyn_cast_or_null<ParenType>(plainSubType)) {
            if(pt->getInnerType()->isArrayType()) {
                return true;
            }
        } else if(isa<ConstantArrayType>(plainSubType)) {
            return true;
        }
        return false;
    });

    const bool isPointerToArray = TestPlainSubType<PointerType>(t, [&](auto* plainSubType) {
        if(isa<ConstantArrayType>(plainSubType)) {
            return true;
        }

        return false;
    });

    std::string typeName = details::GetName(t, unqualified);

    // Sometimes we get char const[2]. If we directly insert the typename we end up with char const__var[2] which is not
    // a valid type name. Hence check for this condition and, if necessary, insert a space before __var.
    auto getSpaceOrEmpty = [&](const std::string& needle) -> std::string {
        if(std::string::npos == typeName.find(needle, 0)) {
            return " ";
        }

        return "";
    };

    if(t->isArrayType() && t->isLValueReferenceType()) {
        std::string space = getSpaceOrEmpty(" [");
        InsertBefore(typeName, "[", StrCat(space, "(&", varName, ")"));

    } else if(t->isArrayType() && !t->isLValueReferenceType()) {
        std::string space = getSpaceOrEmpty(" [");
        InsertBefore(typeName, "[", StrCat(space, varName));

    } else if(isArrayRef) {
        if(std::string::npos != typeName.find("(&", 0)) {
            InsertAfter(typeName, "(&", varName);
        } else {
            InsertBefore(typeName, "&[", "(");
            InsertAfter(typeName, "(&", StrCat(varName, ")"));
        }
    } else if(isFunctionPointer) {
        if(std::string::npos != typeName.find("(&", 0)) {
            InsertAfter(typeName, "(&", varName);
        } else {
            typeName += StrCat(" ", varName);
        }
    } else if(isPointerToArray) {
        if(std::string::npos != typeName.find("(*", 0)) {
            InsertAfter(typeName, "(*", varName);
        } else if(std::string::npos != typeName.find("*", 0)) {
            InsertBefore(typeName, "*", "(");
            InsertAfter(typeName, "*", StrCat(varName, ")"));
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
    if(const VarDecl* VD = GetVarDeclFromDeclRefExpr(declRefExpr)) {
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
            if(const VarDecl* VD = GetVarDeclFromDeclRefExpr(declRefExpr)) {
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

std::string GetNameAsFunctionPointer(const QualType& t)
{
    std::string typeName{GetName(t)};

    if(!t->isFunctionPointerType()) {
        InsertBefore(typeName, "(", "(*)");
    }

    return typeName;
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
                    bool r{false};

                    if(bExpr->EvaluateAsBooleanCondition(r, decl.getASTContext())) {
                        return r;
                    }
                } else if(const auto* bExpr = dyn_cast_or_null<ConditionalOperator>(expr)) {
                    bool r{false};

                    if(bExpr->EvaluateAsBooleanCondition(r, decl.getASTContext())) {
                        return r;
                    }
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
