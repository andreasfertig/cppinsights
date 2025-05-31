/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include <algorithm>

#include "ASTHelpers.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "Insights.h"
#include "InsightsHelpers.h"
#include "NumberIterator.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

using namespace asthelpers;
//-----------------------------------------------------------------------------

void LifetimeTracker::StartScope(bool funcStart)
{
    RETURN_IF(not GetInsightsOptions().ShowLifetime)

    ++scopeCounter;

    objects.push_back(
        {.funcStart = funcStart ? LifetimeEntry::FuncStart::Yes : LifetimeEntry::FuncStart::No, .scope = scopeCounter});
}
//-----------------------------------------------------------------------------

void LifetimeTracker::AddExtended(const VarDecl* decl, const ValueDecl* extending)
{
    // Search for the extending VarlDecl which is already in `objects`. Insert this decl _after_
    if(auto it = std::ranges::find_if(objects, [&](const auto& e) { return e.item == extending; });
       it != objects.end()) {
        const auto scope = (*it).scope;
        objects.insert(std::next(it), {decl, LifetimeEntry::FuncStart::No, scope});
    }
}
//-----------------------------------------------------------------------------

void LifetimeTracker::Add(const VarDecl* decl)
{
    RETURN_IF(not GetInsightsOptions().ShowLifetime)

    QualType type{decl->getType()};

    RETURN_IF(type->isPointerType() or type->isRValueReferenceType());

    // For life-time extended objects
    // XXX contains in C++23
    RETURN_IF(std::ranges::find_if(objects, [&](const auto& e) { return e.item == decl; }) != objects.end());

    objects.push_back({decl, LifetimeEntry::FuncStart::No, scopeCounter});
}
//-----------------------------------------------------------------------------

void LifetimeTracker::InsertDtorCall(const VarDecl* vd, OutputFormatHelper& ofm)
{
    QualType type{vd->getType()};

    if(const auto* ar = dyn_cast_or_null<ConstantArrayType>(type)) {
        type = ar->getElementType();
    }

    if(type->isLValueReferenceType()) {
        type = type.getNonReferenceType();
    }

    if(const auto& ctx = GetGlobalAST(); QualType::DK_cxx_destructor != vd->needsDestruction(ctx)) {
        CodeGeneratorVariant cg{ofm};
        cg->InsertArg(Comment(StrCat(GetName(*vd), " // lifetime ends here")));

        return;
    }

    auto*                dtorDecl = type->getAsCXXRecordDecl()->getDestructor();
    auto*                ic       = CastLToRValue(vd);
    CodeGeneratorVariant cg{ofm};

    auto insertDtor = [&](Expr* member) {
        auto* mem = AccessMember(member, dtorDecl, vd->getType()->isPointerType());

        if(GetInsightsOptions().UseShow2C) {
            cg->InsertArg(CallMemberFun(mem, dtorDecl->getType()));
            ofm.AppendSemiNewLine();

        } else {
            OutputFormatHelper   ofmTmp{};
            CodeGeneratorVariant cg2{ofmTmp};
            cg2->InsertArg(CallMemberFun(mem, dtorDecl->getType()));
            cg->InsertArg(Comment(ofmTmp.GetString()));
        }
    };

    if(const auto* ar = dyn_cast_or_null<ConstantArrayType>(vd->getType()); ar and not GetInsightsOptions().UseShow2C) {
        // not nice but call the destructor for each array element
        for(const auto& i : NumberIterator{GetSize(ar)}) {
            insertDtor(ArraySubscript(ic, i, type));
        }

        return;
    }

    insertDtor(ic);
}
//-----------------------------------------------------------------------------

bool LifetimeTracker::Return(OutputFormatHelper& ofm)
{
    RETURN_FALSE_IF(not GetInsightsOptions().ShowLifetime or objects.empty())

    bool ret{};

    for(OnceTrue needsSemi{}; auto& e : llvm::reverse(objects)) {
        if(LifetimeEntry::FuncStart::Yes == e.funcStart) {
            break;
        }

        if(nullptr == e.item) {
            continue;
        }

        if(needsSemi) {
            CodeGeneratorVariant cg{ofm};
            cg->InsertArg(mkNullStmt());
        }

        InsertDtorCall(e.item, ofm);
        ret = true;
    }

    return ret;
}
//-----------------------------------------------------------------------------

void LifetimeTracker::removeTop()
{
    objects.pop_back();

    auto it = std::ranges::remove_if(objects, [&](const LifetimeEntry& e) { return (e.scope == scopeCounter); });
    objects.erase(it.begin(), it.end());

    --scopeCounter;
}
//-----------------------------------------------------------------------------

bool LifetimeTracker::EndScope(OutputFormatHelper& ofm, bool coveredByReturn)
{
    RETURN_FALSE_IF(not GetInsightsOptions().ShowLifetime or objects.empty())

    bool ret{};

    if(not coveredByReturn) {
        for(auto& e : llvm::reverse(objects)) {
            if(e.scope != scopeCounter) {
                break;
            }

            if(nullptr == e.item) {
                break;
            }

            InsertDtorCall(e.item, ofm);
            ret = true;
        }
    }

    removeTop();

    return ret;
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
