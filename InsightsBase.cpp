/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "InsightsBase.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "InsightsHelpers.h"
#include "InsightsStaticStrings.h"
#include "InsightsStrCat.h"
//-----------------------------------------------------------------------------

namespace clang::insights {
static const char* GetStorageClassAsString(const StorageClass& sc)
{
    switch(sc) {
        case SC_Static: return kwStatic;
        case SC_Extern: return kwExtern;
        default: return "";
    }
}
//-----------------------------------------------------------------------------

static std::string GetStorageClassAsStringWithSpace(const StorageClass& sc)
{
    std::string ret{GetStorageClassAsString(sc)};

    if(ret.length()) {
        ret.append(" ");
    }

    return ret;
}
//-----------------------------------------------------------------------------

void InsightsBase::GenerateFunctionPrototype(OutputFormatHelper& outputFormatHelper, const FunctionDecl& FD)
{
    if(FD.isFunctionTemplateSpecialization()) {
        outputFormatHelper.AppendNewLine("template<>");
    }

    if(FD.isInlined()) {
        outputFormatHelper.Append(kwInlineSpace);
    }

    // NB: const is part of the return type and hence inserted at that point
    // constepxr
    if(FD.isConstexpr()) {
        outputFormatHelper.Append(kwConstExprSpace);
    }

    // static / extern
    outputFormatHelper.Append(GetStorageClassAsStringWithSpace(FD.getStorageClass()));

    outputFormatHelper.Append(GetName(GetDesugarReturnType(FD)));
    outputFormatHelper.Append(" ");

    outputFormatHelper.Append(GetName(FD));

    if(FD.isFunctionTemplateSpecialization()) {
        CodeGenerator codeGenerator{outputFormatHelper};
        codeGenerator.InsertTemplateArgs(FD);
    }

    outputFormatHelper.Append("(");
    outputFormatHelper.AppendParameterList(FD.parameters());
    outputFormatHelper.Append(")", GetNoExcept(FD));
}
//-----------------------------------------------------------------------------

void InsightsBase::InsertAccessModifierAndNameWithReturnType(OutputFormatHelper&  outputFormatHelper,
                                                             const CXXMethodDecl& decl,
                                                             SkipConstexpr        skipConstexpr)
{

    outputFormatHelper.Append(AccessToString(decl), ": ");

    // types of conversion decls can be invalid to type at this place. So introduce a using
    if(isa<CXXConversionDecl>(decl)) {
        outputFormatHelper.AppendNewLine("using retType = ", GetName(GetDesugarReturnType(decl)), ";");
    }

    if(decl.isInlined()) {
        outputFormatHelper.Append(kwInlineSpace);
    }

    if(decl.isStatic()) {
        outputFormatHelper.Append(kwStaticSpace);
    }

    if(decl.isVirtual()) {
        outputFormatHelper.Append(kwVirtualSpace);
    }

    if(decl.isVolatile()) {
        outputFormatHelper.Append(kwVolatileSpace);
    }

    if(decl.isConstexpr()) {
        if(SkipConstexpr::Yes == skipConstexpr) {
            outputFormatHelper.Append("/*");
        }

        outputFormatHelper.Append(kwConstExprSpace);

        if(SkipConstexpr::Yes == skipConstexpr) {
            outputFormatHelper.Append("*/ ");
        }
    }

    if(!isa<CXXConstructorDecl>(decl) && !isa<CXXDestructorDecl>(decl)) {
        if(isa<CXXConversionDecl>(decl)) {
            outputFormatHelper.Append("operator retType (");
        } else {
            outputFormatHelper.Append(GetName(GetDesugarReturnType(decl)), " ");
        }
    }

    if(!isa<CXXConversionDecl>(decl)) {
        outputFormatHelper.Append(GetName(decl), "(");
    }

    outputFormatHelper.AppendParameterList(decl.parameters(), OutputFormatHelper::WithParameterName::Yes);
    outputFormatHelper.Append(")", GetConst(decl), GetNoExcept(decl));
}
//-----------------------------------------------------------------------------

const char* InsightsBase::AccessToString(const CXXMethodDecl& decl)
{
    switch(decl.getAccess()) {
        case AS_public: return "public";
        case AS_protected: return "protected";
        case AS_private: return "private";
        default: return "";
    }
}
//-----------------------------------------------------------------------------

bool InsightsBase::SkipIfAlreadySeen(const Stmt* stmt)
{
    const intptr_t addr{reinterpret_cast<intptr_t>(stmt)};
    if(mMap.find(addr) != mMap.end()) {
        DPrint("duplicate\n");
        return true;
    }

    mMap[addr] = true;

    return false;
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
