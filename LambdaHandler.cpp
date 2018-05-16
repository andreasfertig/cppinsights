/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "LambdaHandler.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "InsightsStaticStrings.h"
#include "InsightsStrCat.h"
#include "OutputFormatHelper.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::insights {

/// \brief Get a correct type for an array.
///
/// This is a special case for lambdas. The QualType of the VarDecl we are looking at could be a plain type. But if we
/// capture via reference, obviously we need to a a reference. This is why the more general version does not work here.
/// Probably needs improvement.
static std::string GetCaptureTypeNameAsParameter(const QualType& t, const std::string& varName)
{
    std::string typeName = GetName(t);

    if(t->isArrayType()) {
        InsertBefore(typeName, "[", StrCat("(&", varName, ")"));
    }

    return typeName;
}
//-----------------------------------------------------------------------------

LambdaHandler::LambdaHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    const auto lambdaMatcher = anyOf(hasAncestor(classTemplateSpecializationDecl()),
                                     isExpansionInSystemHeader(),
                                     isMacroOrInvalidLocation(),
                                     hasParent(lambdaExpr()));

    //        Matcher.addMatcher(varDecl(has(lambdaExpr(hasDescendant(cxxRecordDecl().bind("recodDecl"))).bind("lambdaExpr")))
    matcher.addMatcher(lambdaExpr(unless(lambdaMatcher), hasAncestor(varDecl().bind("varDecl"))).bind("lambdaExpr"),
                       this);

    matcher.addMatcher(lambdaExpr(unless(lambdaMatcher), hasAncestor(callExpr().bind("callExpr"))).bind("lambdaExpr"),
                       this);

    // directly invoke lambda
    matcher.addMatcher(lambdaExpr(unless(anyOf(lambdaMatcher, hasAncestor(varDecl())))).bind("lambdaExpr"), this);
}
//-----------------------------------------------------------------------------

void LambdaHandler::run(const MatchFinder::MatchResult& result)
{
    DPrint("---------------------------------\n");
    const auto* vd       = result.Nodes.getNodeAs<VarDecl>("varDecl");
    const auto* callExpr = result.Nodes.getNodeAs<CallExpr>("callExpr");
    const auto* lambda   = result.Nodes.getNodeAs<LambdaExpr>("lambdaExpr");

    DPrint("lambda: %d  %d\n", (vd != nullptr), (lambda != nullptr));
    Dump(lambda);

    SKIP_IF_ALREADY_SEEN(lambda);

    const std::string lambdaTypeName{GetLambdaName(*lambda->getLambdaClass())};
    SourceLocation    locStart = [&]() {
        if(vd) {
            return vd->getLocStart();
        } else if(callExpr) {
            return callExpr->getLocStart();
        }

        return lambda->getLocStart();
    }();
    const auto columnNr = GetSM(result).getSpellingColumnNumber(locStart);

    OutputFormatHelper outputFormatHelper{columnNr};
    outputFormatHelper.AppendNewLine();
    outputFormatHelper.AppendNewLine(kwClassSpace, lambdaTypeName);
    outputFormatHelper.OpenScope();

    const auto& callOp      = *lambda->getCallOperator();
    const auto& lambdaClass = *lambda->getLambdaClass();

    if(lambda->isGenericLambda()) {
        bool       haveConversionOperator{false};
        const auto conversions = llvm::make_range(lambdaClass.conversion_begin(), lambdaClass.conversion_end());
        for(auto&& conversion : conversions) {
            for(const auto* s : conversion->getAsFunction()->getDescribedFunctionTemplate()->specializations()) {
                if(const auto* cxxmd = dyn_cast_or_null<CXXMethodDecl>(s)) {
                    haveConversionOperator = true;
                    InsertMethod(s, outputFormatHelper, *cxxmd, !vd);
                }
            }

            DPrint("-----\n");
        }

        for(const auto* o : lambdaClass.getLambdaCallOperator()->getDescribedFunctionTemplate()->specializations()) {
            InsertMethod(o, outputFormatHelper, *lambdaClass.getLambdaCallOperator(), !vd);
        }

        if(haveConversionOperator && lambdaClass.getLambdaStaticInvoker()) {
            for(const auto* iv :
                lambdaClass.getLambdaStaticInvoker()->getDescribedFunctionTemplate()->specializations()) {
                DPrint("invoker:\n");

                InsertMethod(iv, outputFormatHelper, *lambdaClass.getLambdaCallOperator(), !vd);
            }
        }

    } else {
        bool       haveConversionOperator{false};
        const auto conversions = llvm::make_range(lambdaClass.conversion_begin(), lambdaClass.conversion_end());
        for(auto&& conversion : conversions) {
            const auto* func = conversion->getAsFunction();

            if(const auto* cxxmd = dyn_cast_or_null<CXXMethodDecl>(func)) {
                /* looks like a conversion operator is (often) there but sometimes undeduced. e.g. still has return
                 * type auto and no body. We do not want these functions. */
                if(cxxmd->hasBody()) {
                    haveConversionOperator = true;
                    InsertMethod(func, outputFormatHelper, *cxxmd, !vd);
                }
            }

            DPrint("-----\n");
        }

        InsertMethod(&callOp, outputFormatHelper, callOp, !vd);

        if(haveConversionOperator && lambdaClass.getLambdaStaticInvoker()) {
            InsertMethod(
                lambdaClass.getLambdaStaticInvoker(), outputFormatHelper, *lambdaClass.getLambdaCallOperator(), !vd);
        }
    }

    /*
     *   class xx
     *   {
     *      x _var1{var1}
     *      ...
     *
     *      RET operator()() MUTABLE
     *      {
     *        BODY
     *      }
     *
     *   };
     *
     */

    std::string ctor{StrCat("public: ", lambdaTypeName, "(")};
    std::string ctorInits{": "};
    std::string inits("{");

    if(0 != lambda->capture_size()) {
        outputFormatHelper.AppendNewLine();
        outputFormatHelper.Append("private:");
    }

    DPrint("captures\n");
    bool        first{true};
    bool        ctorRequired{false};
    const auto* captureInits = lambda->capture_init_begin();
    for(const auto& c : lambda->captures()) {
        const auto* captureInit = *captureInits;
        ++captureInits;
        ctorRequired = true;

        if(!c.capturesVariable() && !c.capturesThis()) {
            // This also catches VLA captures
            if(!c.capturesVLAType()) {
                Error(captureInit, "no capture var\n");
            }
            continue;
        }

        if(first) {
            first = false;
            outputFormatHelper.AppendNewLine();
        } else {
            ctor.append(", ");
            inits.append(", ");
            ctorInits.append("\n, ");
        }

        const auto* capturedVar = c.getCapturedVar();
        const auto& varType     = [&]() {
            if(c.capturesThis()) {
                return captureInit->getType();
            }

            return capturedVar->getType();
        }();

        const std::string varNamePlain = [&]() {
            if(c.capturesThis()) {
                return std::string{"this"};
            }

            return GetName(*capturedVar);
        }();

        DPrint("plain name: %s\n", varNamePlain);

        const std::string varName = [&]() {
            if(c.capturesThis()) {
                return StrCat("__", varNamePlain);
            }

            return varNamePlain;
        }();

        const std::string varTypeName     = GetCaptureTypeNameAsParameter(varType, varNamePlain);
        const std::string ctorVarTypeName = GetCaptureTypeNameAsParameter(varType, StrCat("_", varNamePlain));

        ctor.append(ctorVarTypeName);

        outputFormatHelper.Append(varTypeName);

        const auto captureKind = c.getCaptureKind();
        switch(captureKind) {
            case LCK_This: break;
            case LCK_StarThis: break;
            case LCK_ByCopy: break;
            case LCK_VLAType: break;  //  unreachable
            case LCK_ByRef:
                /* varTypeName already carries the & in case we capture a reference by reference, we need to skip it in
                 * case of an array */
                if(!varType->isReferenceType() && !varType->isArrayType()) {
                    ctor.append("&");
                    outputFormatHelper.Append("&");
                }
                break;
        }

        // If we initialize by copy we can assign a variable: [a=b[1]], get this assigned variable (b[1]) and not a in
        // this case.
        if(!c.capturesThis() && capturedVar->hasInit() && (captureKind == LCK_ByCopy)) {
            OutputFormatHelper ofm;
            CodeGenerator      codeGenerator{ofm};
            codeGenerator.InsertArg(captureInit);
            inits.append(ofm.GetString());
        } else {
            inits.append(StrCat(((c.getCaptureKind() == LCK_StarThis) ? "*" : ""), varNamePlain));
        }

        if(!varType->isArrayType()) {
            ctor.append(StrCat(" _", varName));
            outputFormatHelper.AppendNewLine(" ", varName, ";");
        } else {
            outputFormatHelper.AppendNewLine(";");
        }

        ctorInits.append(StrCat(varName, "{_", varName, "}"));
    }

    ctor.append(")");
    inits.append("}");

    if(ctorRequired) {
        outputFormatHelper.AppendNewLine("");
        outputFormatHelper.AppendNewLine(ctor);
        outputFormatHelper.AppendNewLine(ctorInits);
        outputFormatHelper.AppendNewLine("{}");
    }

    // close the class scope
    outputFormatHelper.CloseScope();

    if(!vd && !callExpr) {
        outputFormatHelper.Append(" ", GetLambdaName(*lambda), inits);
    }

    outputFormatHelper.AppendNewLine(";");
    outputFormatHelper.AppendNewLine();

    // insert before decl stmt
    auto& sm = GetSM(result);
    while(!IsNewLine(*sm.getCharacterData(locStart))) {
        locStart = locStart.getLocWithOffset(-1);
    }
    locStart = locStart.getLocWithOffset(1);

    mRewrite.InsertText(locStart, outputFormatHelper.GetString());

    if(vd || callExpr) {
        // replace lamda expr by __VAR;
        const std::string replacement{StrCat(lambdaTypeName, inits)};

        mRewrite.ReplaceText(lambda->getSourceRange(), replacement);
    }
}
//-----------------------------------------------------------------------------

void LambdaHandler::InsertMethod(const Decl*          d,
                                 OutputFormatHelper&  outputFormatHelper,
                                 const CXXMethodDecl& md,
                                 bool /*skipConstexpr*/)
{
    if(const auto* m = dyn_cast_or_null<CXXMethodDecl>(d)) {
        InsertAccessModifierAndNameWithReturnType(outputFormatHelper, *m, SkipConstexpr::Yes);
        outputFormatHelper.AppendNewLine();

        LambdaCodeGenerator lambdaCodeGenerator{outputFormatHelper};
        CodeGenerator&      codeGenerator{lambdaCodeGenerator};
        codeGenerator.InsertArg(md.getBody());
        outputFormatHelper.AppendNewLine();
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
