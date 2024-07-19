/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include <clang/AST/VTableBuilder.h>

#include <algorithm>
#include <vector>

#include "CodeGenerator.h"
#include "DPrint.h"
#include "Insights.h"
#include "InsightsHelpers.h"
#include "InsightsOnce.h"
#include "InsightsStrCat.h"
#include "NumberIterator.h"

#include "ASTHelpers.h"
//-----------------------------------------------------------------------------

namespace clang::insights {

using namespace asthelpers;
//-----------------------------------------------------------------------------

//! Store the `this` pointer offset from derived to base class.
static llvm::DenseMap<std::pair<const CXXRecordDecl*, const CXXRecordDecl*>, int> mThisPointerOffset{};
//-----------------------------------------------------------------------------

static MemberExpr* AccessMember(std::string_view name, const ValueDecl* vd, QualType type)
{
    auto* rhsDeclRef    = mkVarDeclRefExpr(name, type);
    auto* rhsMemberExpr = AccessMember(rhsDeclRef, vd);

    return rhsMemberExpr;
}
//-----------------------------------------------------------------------------

CfrontCodeGenerator::CfrontVtableData::CfrontVtableData()
: vptpTypedef{Typedef("__vptp"sv, Function("vptp"sv, GetGlobalAST().IntTy, {})->getType())}
, vtableRecorDecl{}
{
    vtableRecorDecl = Struct("__mptr"sv);
    auto AddField   = [&](FieldDecl* field) { vtableRecorDecl->addDecl(field); };

    d = mkFieldDecl(vtableRecorDecl, "d"sv, GetGlobalAST().ShortTy);
    AddField(d);
    AddField(mkFieldDecl(vtableRecorDecl, "i"sv, GetGlobalAST().ShortTy));
    f = mkFieldDecl(vtableRecorDecl, "f"sv, vptpTypedef);
    AddField(f);

    vtableRecorDecl->completeDefinition();

    vtableRecordType = QualType{vtableRecorDecl->getTypeForDecl(), 0u};
}
//-----------------------------------------------------------------------------

VarDecl* CfrontCodeGenerator::CfrontVtableData::VtblArrayVar(int size)
{
    return Variable("__vtbl_array"sv, ContantArrayTy(Ptr(vtableRecordType), size));
}
//-----------------------------------------------------------------------------

FieldDecl* CfrontCodeGenerator::CfrontVtableData::VtblPtrField(const CXXRecordDecl* parent)
{
    return mkFieldDecl(const_cast<CXXRecordDecl*>(parent), StrCat("__vptr"sv, GetName(*parent)), Ptr(vtableRecordType));
}
//-----------------------------------------------------------------------------

CfrontCodeGenerator::CfrontVtableData& CfrontCodeGenerator::VtableData()
{
    static CfrontVtableData data{};

    return data;
}
//-----------------------------------------------------------------------------

CodeGeneratorVariant::CodeGenerators::CodeGenerators(OutputFormatHelper&                      _outputFormatHelper,
                                                     CodeGenerator::LambdaStackType&          lambdaStack,
                                                     CodeGenerator::ProcessingPrimaryTemplate processingPrimaryTemplate)
{
    if(GetInsightsOptions().UseShow2C) {
        new(&cfcg) CfrontCodeGenerator{
            _outputFormatHelper, lambdaStack, CodeGenerator::LambdaInInitCapture::No, processingPrimaryTemplate};
    } else {
        new(&cg) CodeGenerator{
            _outputFormatHelper, lambdaStack, CodeGenerator::LambdaInInitCapture::No, processingPrimaryTemplate};
    }
}
//-----------------------------------------------------------------------------

CodeGeneratorVariant::CodeGenerators::CodeGenerators(OutputFormatHelper&                _outputFormatHelper,
                                                     CodeGenerator::LambdaInInitCapture lambdaInitCapture)
{
    if(GetInsightsOptions().UseShow2C) {
        new(&cfcg) CfrontCodeGenerator{_outputFormatHelper, lambdaInitCapture};
    } else {
        new(&cg) CodeGenerator{_outputFormatHelper, lambdaInitCapture};
    }
}
//-----------------------------------------------------------------------------

CodeGeneratorVariant::CodeGenerators::~CodeGenerators()
{
    if(GetInsightsOptions().UseShow2C) {
        cfcg.~CfrontCodeGenerator();
    } else {
        cg.~CodeGenerator();
    }
}
//-----------------------------------------------------------------------------

void CodeGeneratorVariant::Set()
{
    if(GetInsightsOptions().UseShow2C) {
        cg = &cgs.cfcg;
    } else {
        cg = &cgs.cg;
    }
}
//-----------------------------------------------------------------------------

static bool IsCopyOrMoveCtor(const CXXConstructorDecl* ctor)
{
    return ctor and (ctor->isCopyConstructor() or ctor->isMoveConstructor());
}
//-----------------------------------------------------------------------------

static bool IsCopyOrMoveAssign(const CXXMethodDecl* stmt)
{
    return stmt and (stmt->isCopyAssignmentOperator() or stmt->isMoveAssignmentOperator());
}
//-----------------------------------------------------------------------------

static std::string GetSpecialMemberName(const ValueDecl* vd, QualType type)
{
    if(const auto* md = dyn_cast_or_null<CXXMethodDecl>(vd)) {
        // GetName below would return a[4] for example. To avoid the array part we go for the underlying type.
        if(const auto* ar = dyn_cast_or_null<ArrayType>(type)) {
            type = ar->getElementType();
        }

        auto rdName = GetName(type);

        if(const auto* ctor = dyn_cast_or_null<CXXConstructorDecl>(md)) {
            if(ctor->isCopyConstructor()) {
                return StrCat("CopyConstructor_"sv, rdName);
            } else if(ctor->isMoveConstructor()) {
                return StrCat("MoveConstructor_"sv, rdName);
            }

            return StrCat("Constructor_"sv, rdName);

        } else if(isa<CXXDestructorDecl>(md)) {
            return StrCat("Destructor_"sv, rdName);
        }
    }

    return GetName(*vd);
}
//-----------------------------------------------------------------------------

std::string GetSpecialMemberName(const ValueDecl* vd)
{
    if(const auto* md = dyn_cast_or_null<CXXMethodDecl>(vd)) {
        return GetSpecialMemberName(vd, GetRecordDeclType(md));
    }

    return {};
}
//-----------------------------------------------------------------------------

void CfrontCodeGenerator::InsertArg(const CXXThisExpr*)
{
    mOutputFormatHelper.Append(kwInternalThis);
}
//-----------------------------------------------------------------------------

static bool HasCtor(QualType t)
{
    if(auto* cxxRecordDecl = t->getAsCXXRecordDecl(); cxxRecordDecl and not cxxRecordDecl->isTrivial()) {
        return true;
    }

    return false;
}
//-----------------------------------------------------------------------------

static bool HasDtor(QualType t)
{
    if(auto* cxxRecordDecl = t->getAsCXXRecordDecl(); cxxRecordDecl and not cxxRecordDecl->hasTrivialDestructor()) {
        return true;
    }

    return false;
}
//-----------------------------------------------------------------------------

static auto* CallVecDeleteOrDtor(Expr* objectParam, QualType allocatedType, std::string_view name, uint64_t size)
{
    auto dtorName = GetSpecialMemberName(allocatedType->getAsCXXRecordDecl()->getDestructor());

    SmallVector<Expr*, 4> args{objectParam,
                               Sizeof(allocatedType),
                               Int32(size),  // XXX what is the correct value?
                               CastToVoidFunPtr(dtorName)};

    return Call(name, args);
}
//-----------------------------------------------------------------------------

static auto* CallVecDelete(Expr* objectParam, QualType allocatedType)
{
    EnableGlobalInsert(GlobalInserts::FuncCxaVecDel);

    return CallVecDeleteOrDtor(objectParam, allocatedType, "__cxa_vec_delete"sv, 0);
}
//-----------------------------------------------------------------------------

static auto* CallVecDtor(Expr* objectParam, const ConstantArrayType* ar)
{
    EnableGlobalInsert(GlobalInserts::FuncCxaVecDtor);

    return CallVecDeleteOrDtor(objectParam, ar->getElementType(), "__cxa_vec_dtor"sv, GetSize(ar));
}
//-----------------------------------------------------------------------------

void CfrontCodeGenerator::InsertArg(const CXXDeleteExpr* stmt)
{
    auto  destroyedType = stmt->getDestroyedType();
    auto* arg           = const_cast<Expr*>(stmt->getArgument());

    StmtsContainer bodyStmts{};

    if(const auto hasDtor{HasDtor(destroyedType)}; stmt->isArrayForm() and hasDtor) {
        bodyStmts.Add(CallVecDelete(arg, destroyedType));
    } else if(hasDtor) {
        bodyStmts.Add(Call(StrCat("Destructor_"sv, GetName(destroyedType)), {arg}));
    }

    EnableGlobalInsert(GlobalInserts::FuncFree);

    bodyStmts.Add(Call("free"sv, {arg}));

    InsertArg(If(arg, {bodyStmts}));
    mInsertSemi = false;  // Since we tamper with a CompoundStmt we signal _not_ to insert the usual semi
}
//-----------------------------------------------------------------------------

static auto* CallVecNewOrCtor(std::string_view ctorName,
                              Expr*            objectParam,
                              QualType         allocatedType,
                              Expr*            arraySizeExpr,
                              std::string_view funName)
{
    // According to "Inside the C++ Object Model" the dtor is required in case one of the array element ctors fails the
    // already constructed one must be destroyed.
    // See also: https://itanium-cxx-abi.github.io/cxx-abi/abi.html#array-ctor
    auto dtorName = [&]() {
        if(HasDtor(allocatedType)) {
            return GetSpecialMemberName(allocatedType->getAsCXXRecordDecl()->getDestructor());
        }

        return std::string{kwNull};
    }();

    SmallVector<Expr*, 6> args{objectParam,
                               Sizeof(allocatedType),
                               arraySizeExpr,
                               Int32(0),  // XXX what is the correct value?
                               CastToVoidFunPtr(ctorName),
                               CastToVoidFunPtr(dtorName)};

    return Call(funName, args);
}
//-----------------------------------------------------------------------------

static auto* CallVecNew(std::string_view ctorName, Expr* objectParam, QualType allocatedType, Expr* arraySizeExpr)
{
    EnableGlobalInsert(GlobalInserts::FuncCxaVecNew);

    return CallVecNewOrCtor(ctorName, objectParam, allocatedType, arraySizeExpr, "__cxa_vec_new"sv);
}
//-----------------------------------------------------------------------------

static auto*
CallVecCtor(std::string_view ctorName, const VarDecl* objectParam, QualType allocatedType, Expr* arraySizeExpr)
{
    EnableGlobalInsert(GlobalInserts::FuncCxaVecCtor);

    return CallVecNewOrCtor(ctorName, mkDeclRefExpr(objectParam), allocatedType, arraySizeExpr, "__cxa_vec_ctor"sv);
}
//-----------------------------------------------------------------------------

void CfrontCodeGenerator::InsertArg(const CXXNewExpr* cstmt)
{
    auto* stmt = const_cast<CXXNewExpr*>(cstmt);

    auto allocatedType = stmt->getAllocatedType();
    auto ctorName      = StrCat("Constructor_"sv, GetName(allocatedType));

    if(stmt->isArray()) {
        EnableGlobalInsert(GlobalInserts::FuncMalloc);

        auto* arraySizeExpr = stmt->getArraySize().value();
        auto* callMalloc    = Call("malloc"sv, {Mul(Sizeof(allocatedType), arraySizeExpr)});

        Expr* assign{};

        // According to "Inside the C++ Object Model" a trivial and literal type does use plain malloc as it does not
        // require a ctor/dtor.
        if(not HasCtor(allocatedType) and not HasDtor(allocatedType)) {
            assign = Cast(callMalloc, Ptr(allocatedType));

        } else {
            if(allocatedType->getAsCXXRecordDecl()->ctors().empty()) {
                EnableGlobalInsert(GlobalInserts::HeaderStddef);

                ctorName = kwNull;
            }

            assign = Cast(CallVecNew(ctorName, callMalloc, allocatedType, arraySizeExpr), Ptr(allocatedType));
        }

        InsertArg(assign);

        if(stmt->hasInitializer() and allocatedType->isBuiltinType()) {  // Ignore CXXConstructExpr
            // The resulting code here is slightly different from the cfront code. For
            //
            // int* p = new int(2);
            //
            // C++ Insights generates:
            //
            // int* p = (int*)malloc(sizeof(int));
            // if(p) { *p = 2; }
            //
            // However, due to Cs requirement to only declare variables and delay initialization the cfront code is
            //
            // int* p;
            // if(p = (int*)malloc(sizeof(int))) { *p = 2; }
            mOutputFormatHelper.AppendSemiNewLine();

            if(auto* inits = dyn_cast_or_null<InitListExpr>(stmt->getInitializer())) {
                auto* expr = [&]() -> const Expr* {
                    if(auto* vd = dyn_cast_or_null<VarDecl>(mLastDecl)) {
                        return mkDeclRefExpr(vd);
                    }

                    return mLastExpr;
                }();

                StmtsContainer bodyStmts{};

                for(auto idx : NumberIterator(inits->getNumInits())) {
                    bodyStmts.Add(Assign(ArraySubscript(expr, idx, allocatedType), inits->getInit(idx)));
                }

                InsertArg(If(expr, {bodyStmts}));

                mInsertSemi = false;  // Since we tamper with a CompoundStmt we signal _not_ to insert the usual semi
            }
        }

        return;
    }

    auto* mallocCall = [&]() -> Expr* {
        if(stmt->getNumPlacementArgs()) {
            return stmt->getPlacementArg(0);
        }

        EnableGlobalInsert(GlobalInserts::FuncMalloc);

        return Call("malloc"sv, {Sizeof(allocatedType)});
    }();

    mallocCall = Cast(mallocCall, Ptr(allocatedType));

    if(allocatedType->isBuiltinType()) {
        InsertArg(mallocCall);

        if(stmt->hasInitializer()) {
            // The resulting code here is slightly different from the cfront code. For
            //
            // int* p = new int(2);
            //
            // C++ Insights generates:
            //
            // int* p = (int*)malloc(sizeof(int));
            // if(p) { *p = 2; }
            //
            // However, due to Cs requirement to only declare variables and delay initialization the cfront code is
            //
            // int* p;
            // if(p = (int*)malloc(sizeof(int))) { *p = 2; }
            mOutputFormatHelper.AppendSemiNewLine();

            auto* varDeclRefExpr = mkDeclRefExpr(dyn_cast_or_null<VarDecl>(mLastDecl));

            StmtsContainer bodyStmts{Assign(Dref(varDeclRefExpr), stmt->getInitializer())};

            InsertArg(If(varDeclRefExpr, {bodyStmts}));

            mInsertSemi = false;  // Since we tamper with a CompoundStmt we signal _not_ to insert the usual semi
        }

        return;
    }

    SmallVector<Expr*, 16> args{mallocCall};
    if(auto* ncCtorExpr = const_cast<CXXConstructExpr*>(stmt->getConstructExpr())) {
        args.append(ncCtorExpr->arg_begin(), ncCtorExpr->arg_end());
    }

    InsertArg(Call(ctorName, args));
}
//-----------------------------------------------------------------------------

void CfrontCodeGenerator::InsertArg(const CXXOperatorCallExpr* stmt)
{
    const auto* callee = stmt->getDirectCallee();

    if(const auto* cxxCallee = dyn_cast_or_null<CXXMethodDecl>(callee); IsCopyOrMoveAssign(cxxCallee)) {
        SmallVector<Expr*, 16> args{};

        for(const auto& arg : stmt->arguments()) {
            args.push_back(Ref(arg));
        }

        InsertArg(Call(callee, args));

    } else {
        InsertArg(dyn_cast_or_null<CallExpr>(stmt));
    }
}
//-----------------------------------------------------------------------------

static void InsertVtblPtr(const CXXMethodDecl* stmt, const CXXRecordDecl* cur, StmtsContainer& bodyStmts)
{
    if(cur->isPolymorphic() and (0 == cur->getNumBases())) {
        auto* fieldDecl     = CfrontCodeGenerator::VtableData().VtblPtrField(cur);
        auto* lhsMemberExpr = AccessMember(kwInternalThis, fieldDecl, Ptr(GetRecordDeclType(cur)));

        // struct __mptr *__ptbl_vec__c___src_C_[]
        auto* vtablAr = CfrontCodeGenerator::VtableData().VtblArrayVar(1);
        auto* vtblArrayPos =
            ArraySubscript(mkDeclRefExpr(vtablAr), GetGlobalVtablePos(stmt->getParent(), cur), fieldDecl->getType());

        bodyStmts.AddBodyStmts(Assign(lhsMemberExpr, fieldDecl, vtblArrayPos));

    } else if(cur->isPolymorphic() and (0 < cur->getNumBases()) and (cur != stmt->getParent())) {
        for(const auto& base : cur->bases()) {
            InsertVtblPtr(stmt, base.getType()->getAsCXXRecordDecl(), bodyStmts);
        }
    }
}
//-----------------------------------------------------------------------------

void CfrontCodeGenerator::InsertCXXMethodDecl(const CXXMethodDecl* stmt, SkipBody)
{
    OutputFormatHelper initOutputFormatHelper{};
    initOutputFormatHelper.SetIndent(mOutputFormatHelper, OutputFormatHelper::SkipIndenting::Yes);

    auto recordDeclType = GetRecordDeclType(stmt);
    if(stmt->isConst()) {
        recordDeclType.addConst();
    }

    auto           parentType = Ptr(recordDeclType);
    auto*          body       = stmt->getBody();
    StmtsContainer bodyStmts{};

    auto retType = stmt->getReturnType();

    auto processBaseClassesAndFields = [&](const CXXRecordDecl* parent) {
        auto* thisOfParent = mkVarDeclRefExpr(kwInternalThis, parentType);

        auto insertFields = [&](const RecordDecl* rd) {
            for(auto* fieldDecl : rd->fields()) {
                if(const auto* cxxRecordDecl = fieldDecl->getType()->getAsCXXRecordDecl()) {
                    // We don't need any checks like isDefaultConstructible. We would not reach this
                    // point in any other case.

                    auto  lvalueRefType = GetGlobalAST().getLValueReferenceType(parentType);
                    auto* lhsMemberExpr = AccessMember(kwInternalThis, fieldDecl, lvalueRefType);
                    auto* rhsMemberExpr = AccessMember("__rhs"sv, fieldDecl, lvalueRefType);

                    // Add call to ctor
                    bodyStmts.AddBodyStmts(Call(GetSpecialMemberName(stmt, GetRecordDeclType(cxxRecordDecl)),
                                                {Ref(lhsMemberExpr), Ref(rhsMemberExpr)}));

                } else {
                    auto* rhsMemberExpr = AccessMember("__rhs"sv, fieldDecl, parentType);

                    bodyStmts.AddBodyStmts(Assign(thisOfParent, fieldDecl, rhsMemberExpr));
                }
            }
        };

        for(const auto& base : parent->bases()) {
            const auto rd = base.getType()->getAsRecordDecl();

            insertFields(rd);

            auto* lhsCast    = StaticCast(GetRecordDeclType(rd), thisOfParent, true);
            auto* rhsDeclRef = mkVarDeclRefExpr("__rhs"sv, parentType);
            auto* rhsCast    = StaticCast(GetRecordDeclType(rd), rhsDeclRef, true);

            auto* callAssignBase = Call(GetSpecialMemberName(stmt, GetRecordDeclType(rd)), {lhsCast, rhsCast});

            bodyStmts.AddBodyStmts(callAssignBase);
        }

        // insert own fields
        insertFields(parent);
    };

    // Skip ctor for trivial types
    if(const auto* ctorDecl = dyn_cast_or_null<CXXConstructorDecl>(stmt)) {

        const auto* parent = stmt->getParent();

        if(not stmt->doesThisDeclarationHaveABody()) {
            if(IsCopyOrMoveCtor(ctorDecl)) {
                if(const bool showSpecialMemberFunc{stmt->isUserProvided() or stmt->isExplicitlyDefaulted()};
                   not showSpecialMemberFunc) {
                    return;
                }

                processBaseClassesAndFields(parent);
            }
        } else if(ctorDecl->isDefaultConstructor()) {
            auto insertFields = [&](const RecordDecl* rd) {
                for(auto* fieldDecl : rd->fields()) {
                    if(auto* initializer = fieldDecl->getInClassInitializer();
                       initializer and fieldDecl->hasInClassInitializer()) {
                        const bool isConstructorExpr{isa<CXXConstructExpr>(initializer) or
                                                     isa<ExprWithCleanups>(initializer)};

                        if(not isConstructorExpr) {
                            auto* lhsMemberExpr = AccessMember(kwInternalThis, fieldDecl, Ptr(GetRecordDeclType(rd)));

                            bodyStmts.AddBodyStmts(Assign(lhsMemberExpr, fieldDecl, initializer));

                        } else {
                            bodyStmts.AddBodyStmts(CallConstructor(fieldDecl->getType(),
                                                                   Ptr(GetRecordDeclType(rd)),
                                                                   fieldDecl,
                                                                   ArgsToExprVector(initializer),
                                                                   DoCast::No,
                                                                   AsReference::Yes));
                        }

                    } else if(const auto* cxxRecordDecl = fieldDecl->getType()->getAsCXXRecordDecl()) {
                        // We don't need any checks like isDefaultConstructible. We would not reach this
                        // point in any other cause.

                        if(auto lhsType = fieldDecl->getType(); HasCtor(lhsType)) {
                            bodyStmts.AddBodyStmts(CallConstructor(GetRecordDeclType(cxxRecordDecl),
                                                                   Ptr(GetRecordDeclType(cxxRecordDecl)),
                                                                   fieldDecl,
                                                                   {},
                                                                   DoCast::No,
                                                                   AsReference::Yes));

                        } else {
                            bodyStmts.Add(
                                Comment(StrCat(GetName(*fieldDecl), " // trivial type, maybe uninitialized"sv)));
                            // Nothing to do here, we are looking at an uninitialized variable
                        }
                    }
                }
            };

            for(const auto& base : parent->bases()) {
                auto baseType = base.getType();

                if(const auto* baseRd = baseType->getAsCXXRecordDecl();
                   baseRd and baseRd->hasNonTrivialDefaultConstructor()) {
                    bodyStmts.AddBodyStmts(CallConstructor(baseType, Ptr(baseType), nullptr, {}, DoCast::Yes));
                }

                insertFields(baseType->getAsRecordDecl());
            }

            // insert our vtable pointer
            InsertVtblPtr(stmt, stmt->getParent(), bodyStmts);

            // in case of multi inheritance insert additional vtable pointers
            for(const auto& base : parent->bases()) {
                InsertVtblPtr(stmt, base.getType()->getAsCXXRecordDecl(), bodyStmts);
            }

            // insert own fields
            insertFields(parent);

        } else {
            // user ctor
            for(const auto* init : ctorDecl->inits()) {
                auto* member = init->getMember();

                if(not isa<CXXConstructExpr>(init->getInit())) {
                    if(not init->getAnyMember()) {
                        continue;
                    }

                    auto* lhsMemberExpr = AccessMember(kwInternalThis, member, Ptr(GetRecordDeclType(parent)));

                    bodyStmts.AddBodyStmts(Assign(lhsMemberExpr, member, init->getInit()));
                    continue;

                } else if(init->isBaseInitializer()) {
                    bodyStmts.AddBodyStmts(init->getInit());
                    continue;
                }

                auto ctorType = member->getType();

                auto* lhsMemberExpr = AccessMember(kwInternalThis, member, ctorType);

                auto callParams{ArgsToExprVector(init->getInit())};
                callParams.insert(callParams.begin(), Ref(lhsMemberExpr));

                bodyStmts.AddBodyStmts(
                    Call(GetSpecialMemberName(stmt, GetRecordDeclType(ctorType->getAsRecordDecl())), callParams));
            }
        }

        if(body) {
            bodyStmts.AddBodyStmts(body);
        }

        bodyStmts.AddBodyStmts(Return(mkVarDeclRefExpr(kwInternalThis, Ptr(ctorDecl->getType()))));

        body    = mkCompoundStmt({bodyStmts});
        retType = parentType;

        // copy and move assignment op
    } else if(IsCopyOrMoveAssign(stmt)) {
        if(not stmt->doesThisDeclarationHaveABody() or stmt->isDefaulted()) {

            // we don't want the default generated body
            bodyStmts.clear();

            processBaseClassesAndFields(stmt->getParent());
        } else if(body) {
            bodyStmts.AddBodyStmts(body);
        }

        bodyStmts.AddBodyStmts(Return(mkVarDeclRefExpr(kwInternalThis, Ptr(stmt->getType()))));

        body    = mkCompoundStmt({bodyStmts});
        retType = parentType;

    } else if(const auto* dtor = dyn_cast_or_null<CXXDestructorDecl>(stmt)) {
        // Based on: https://www.dre.vanderbilt.edu/~schmidt/PDF/C++-translation.pdf

        if(body) {
            bodyStmts.AddBodyStmts(body);
        }

        if(not HasDtor(GetRecordDeclType(dtor->getParent()))) {
            return;
        }

        for(const auto& base : llvm::reverse(dtor->getParent()->bases())) {
            if(not dtor->isVirtual()) {
                continue;
            }

            auto* lhsDeclRef = mkVarDeclRefExpr(kwInternalThis, Ptr(base.getType()));
            auto* cast       = Cast(lhsDeclRef, lhsDeclRef->getType());

            bodyStmts.Add(
                Call(GetSpecialMemberName(stmt, GetRecordDeclType(base.getType()->getAsRecordDecl())), {cast}));

            body = mkCompoundStmt({bodyStmts});
        }
    }

    params_store params{};
    params.reserve(stmt->getNumParams() + 1);

    if(not IsStaticStorageClass(stmt) and not stmt->isStatic()) {
        params.emplace_back(kwInternalThis, parentType);
    }

    for(const auto& param : stmt->parameters()) {
        std::string name{GetName(*param)};
        auto        type = param->getType();

        // at least in case of a copy constructor modify the parameters
        if((0 == name.length()) and
           (IsCopyOrMoveCtor(dyn_cast_or_null<CXXConstructorDecl>(stmt)) or IsCopyOrMoveAssign(stmt))) {
            name = "__rhs"sv;
            type = Ptr(type.getNonReferenceType());
        }

        params.emplace_back(name, type);
    }

    auto* callSpecialMemberFn = Function(GetSpecialMemberName(stmt), retType, to_params_view(params));
    callSpecialMemberFn->setInlineSpecified(stmt->isInlined());
    callSpecialMemberFn->setStorageClass((stmt->isStatic() or IsStaticStorageClass(stmt)) ? SC_Static : SC_None);
    callSpecialMemberFn->setBody(body);

    InsertArg(callSpecialMemberFn);

    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

void CfrontCodeGenerator::FormatCast(const std::string_view,
                                     const QualType& castDestType,
                                     const Expr*     subExpr,
                                     const CastKind& kind)
{
    // C does not have a rvalue notation and we already transformed the temporary into an object. Skip the cast to &&.
    // Ignore CK_UncheckedDerivedToBase which would lead to (A)c where neither A nor c is a pointer.
    if(not castDestType->isRValueReferenceType() and not(CastKind::CK_UncheckedDerivedToBase == kind)) {
        mOutputFormatHelper.Append("(", GetName(castDestType), ")");

        // ARM p 221:
        // C* pc = new C;
        // B* pb = pc -> pc = (B*) ((char*)pc+delta(B))
        if(is{kind}.any_of(CastKind::CK_DerivedToBase, CastKind::CK_BaseToDerived)) {
            // We have to switch in case of a base to derived cast
            auto [key,
                  sign] = [&]() -> std::pair<std::pair<const CXXRecordDecl*, const CXXRecordDecl*>, std::string_view> {
                auto plainType = [](QualType t) {
                    if(const auto* pt = dyn_cast_or_null<PointerType>(t.getTypePtrOrNull())) {
                        return pt->getPointeeType()->getAsCXXRecordDecl();
                    }

                    return t->getAsCXXRecordDecl();
                };

                auto base    = plainType(castDestType);
                auto derived = plainType(subExpr->getType());

                if((CastKind::CK_BaseToDerived == kind)) {
                    return {{base, derived}, "-"sv};
                }

                return {{derived, base}, "+"sv};
            }();

            if(auto off = mThisPointerOffset[key]) {
                mOutputFormatHelper.Append("((char*)"sv);
                InsertArg(subExpr);
                mOutputFormatHelper.Append(sign, off, ")"sv);

                return;
            }
        }
    }

    InsertArg(subExpr);
}
//-----------------------------------------------------------------------------

void CfrontCodeGenerator::InsertArg(const CXXNullPtrLiteralExpr*)
{
    EnableGlobalInsert(GlobalInserts::HeaderStddef);

    mOutputFormatHelper.Append(kwNull);
}
//-----------------------------------------------------------------------------

void CfrontCodeGenerator::InsertArg(const StaticAssertDecl* stmt)
{
    EnableGlobalInsert(GlobalInserts::HeaderAssert);

    mOutputFormatHelper.Append("_Static_assert"sv);

    WrapInParens([&] {
        InsertArg(stmt->getAssertExpr());

        if(stmt->getMessage()) {
            mOutputFormatHelper.Append(", "sv);
            InsertArg(stmt->getMessage());
        }
    });

    mOutputFormatHelper.AppendSemiNewLine();
}
//-----------------------------------------------------------------------------

void CfrontCodeGenerator::InsertArg(const TypedefDecl* stmt)
{
    mOutputFormatHelper.AppendSemiNewLine(kwTypedefSpace, GetName(stmt->getUnderlyingType()), " "sv, GetName(*stmt));
    mOutputFormatHelper.AppendNewLine();
}
//-----------------------------------------------------------------------------

static void ProcessFields(CXXRecordDecl* recordDecl, const CXXRecordDecl* rd)
{
    RETURN_IF(not rd->hasDefinition())

    auto AddField = [&](const FieldDecl* field) {
        recordDecl->addDecl(mkFieldDecl(recordDecl, GetName(*field), field->getType()));
    };

    // Insert field from base classes
    for(const auto& base : rd->bases()) {
        // XXX: ignoring TemplateSpecializationType
        if(const auto* rdBase = dyn_cast_or_null<CXXRecordDecl>(base.getType().getCanonicalType()->getAsRecordDecl())) {
            ProcessFields(recordDecl, rdBase);
        }
    }

    // insert vtable pointer if required
    if(rd->isPolymorphic() and (rd->getNumBases() == 0)) {
        recordDecl->addDecl(CfrontCodeGenerator::VtableData().VtblPtrField(rd));
    }

    // insert own fields
    for(const auto* d : rd->fields()) {
        AddField(d);
    }

    if(recordDecl->field_empty()) {
        AddField(mkFieldDecl(recordDecl, "__dummy"sv, GetGlobalAST().CharTy));
    }
}
//-----------------------------------------------------------------------------

static std::string GetFirstPolymorphicBaseName(const RecordDecl* decl, const RecordDecl* to)
{
    std::string ret{GetName(*decl)};

    if(const auto* rdecl = dyn_cast_or_null<CXXRecordDecl>(decl); rdecl->getNumBases() > 1) {
        for(const auto& base : rdecl->bases()) {
            if(const auto* rd = base.getType()->getAsRecordDecl(); rd == to) {
                ret += GetFirstPolymorphicBaseName(rd, to);
                break;
            }
        }
    }

    return ret;
}
//-----------------------------------------------------------------------------

void CfrontCodeGenerator::InsertArg(const CXXRecordDecl* stmt)
{
    auto* recordDecl = Struct(GetName(*stmt));

    if(stmt->hasDefinition() and stmt->isPolymorphic()) {
        if(auto* itctx =
               static_cast<ItaniumVTableContext*>(const_cast<ASTContext&>(GetGlobalAST()).getVTableContext())) {
#if 0
            // Get mangled RTTI name
            auto*                     mc = const_cast<ASTContext&>(GetGlobalAST()).createMangleContext(nullptr);
            SmallString<256>          rttiName{};
            llvm::raw_svector_ostream out(rttiName);
            mc->mangleCXXRTTI(QualType(stmt->getTypeForDecl(), 0), out);
            DPrint("name: %s\n", rttiName.c_str());
#endif

            SmallVector<Expr*, 16>   mInitExprs{};
            SmallVector<QualType, 5> baseList{};

            if(stmt->getNumBases() == 0) {
                baseList.push_back(GetRecordDeclType(stmt));
            }

            for(const auto& base : stmt->bases()) {
                baseList.push_back(base.getType());
            }

            llvm::DenseMap<uint64_t, ThunkInfo> thunkMap{};
            const VTableLayout&                 layout{itctx->getVTableLayout(stmt)};

            for(const auto& [idx, thunk] : layout.vtable_thunks()) {
                thunkMap[idx] = thunk;
            }

            unsigned clsIdx{};
            unsigned funIdx{};
            auto&    vtblData = VtableData();

            auto pushVtable = [&] {
                if(funIdx) {
                    EnableGlobalInsert(GlobalInserts::FuncVtableStruct);

                    //    struct __mptr __vtbl__A[] = {0, 0, 0, 0, 0, (__vptp)FunA, 0, 0, 0};
                    auto* thisRd = baseList[clsIdx - 1]->getAsCXXRecordDecl();
                    auto  vtableName{StrCat("__vtbl_"sv, GetFirstPolymorphicBaseName(stmt, thisRd))};
                    auto* vtabl = Variable(vtableName, ContantArrayTy(vtblData.vtableRecordType, funIdx));
                    vtabl->setInit(InitList(mInitExprs, vtblData.vtableRecordType));

                    PushVtableEntry(stmt, thisRd, vtabl);

                    funIdx = 0;
                }

                mInitExprs.clear();
            };

            for(unsigned i = 0; const auto& vc : layout.vtable_components()) {
                switch(vc.getKind()) {
                    case VTableComponent::CK_OffsetToTop: {
                        auto off = layout.getVTableOffset(clsIdx);
                        if(auto rem = (off % 4)) {
                            off += 4 - rem;  // sometimes the value is misaligned. Align to 4 bytes
                        }

                        mThisPointerOffset[{stmt, baseList[clsIdx]->getAsCXXRecordDecl()}] = off * 4;  // we need bytes

                        if(clsIdx >= 1) {
                            pushVtable();
                        }
                        ++clsIdx;
                    } break;

                    case VTableComponent::CK_RTTI:
                        break;

                        // Source: https://itanium-cxx-abi.github.io/cxx-abi/abi.html#vtable-components
                        // The entries for virtual destructors are actually pairs of entries. The first destructor,
                        // called the complete object destructor, performs the destruction without calling delete() on
                        // the object. The second destructor, called the deleting destructor, calls delete() after
                        // destroying the object.
                    case VTableComponent::CK_CompleteDtorPointer:
                        break;  // vc.getKind() == VTableComponent::CK_CompleteDtorPointer
                    case VTableComponent::CK_DeletingDtorPointer:
                    case VTableComponent::CK_FunctionPointer: {
                        auto* thunkOffset = [&] {
                            if(ThunkInfo thunk = thunkMap.lookup(i); not thunk.isEmpty() and not thunk.This.isEmpty()) {
                                return Int32(thunk.This.NonVirtual);
                            }

                            return Int32(0);
                        }();

                        const auto* md = dyn_cast_or_null<FunctionDecl>(vc.getFunctionDecl());

                        std::string name{};
                        if(md->isPureVirtual()) {
                            EnableGlobalInsert(GlobalInserts::HeaderStdlib);
                            EnableGlobalInsert(GlobalInserts::FuncCxaPureVirtual);

                            md = Function("__cxa_pure_virtual"sv, VoidTy(), params_vector{{kwInternalThis, VoidTy()}});

                            name = GetName(*md);
                        } else {
                            name = GetSpecialMemberName(md);
                        }

                        auto* reicast = ReinterpretCast(vtblData.vptpTypedef, mkVarDeclRefExpr(name, md->getType()));

                        mInitExprs.push_back(InitList({thunkOffset, Int32(0), reicast}, vtblData.vtableRecordType));

                        ++funIdx;
                        break;
                    }
                    default: break;
                }

                ++i;
            }

            pushVtable();
        }
    }

    if(stmt->hasDefinition()) {
        ProcessFields(recordDecl, stmt);
        recordDecl->completeDefinition();

        mOutputFormatHelper.Append(kwTypedefSpace);
    }

    // use our freshly created recordDecl
    CodeGenerator::InsertArg(recordDecl);

#if 0
    // TypedefDecl above is not called
    auto& ctx = GetGlobalAST();
    auto et = ctx.getElaboratedType(ElaboratedTypeKeyword::ETK_Struct, nullptr, GetRecordDeclType(recordDecl), nullptr);
    auto* typedefDecl = Typedef(GetName(*stmt),et);
    CodeGenerator::InsertArg(typedefDecl);
#endif

    // insert member functions except for the special member functions and classes defined inside this class
    for(OnceTrue firstRecordDecl{}; const auto* d : stmt->decls()) {
        if((isa<CXXRecordDecl>(d) and firstRecordDecl)          // skip the first record decl which are ourselves
           or (stmt->isLambda() and isa<CXXDestructorDecl>(d))  // skip dtor for lambdas
           or isa<FieldDecl>(d)                                 // skip fields
           or isa<AccessSpecDecl>(d)                            // skip access specifiers
        ) {
            continue;
        }

        // According to "Inside the C++ Object Model" a trivial and literal type has no ctor/dtor.
        if((stmt->isTrivial() and isa<CXXConstructorDecl>(d)) or
           (stmt->hasTrivialDestructor() and isa<CXXDestructorDecl>(d))) {
            continue;
        }

        InsertArg(d);
    }
}
//-----------------------------------------------------------------------------

///! Find the first polymorphic base class.
static const CXXRecordDecl* GetFirstPolymorphicBase(const RecordDecl* decl)
{
    if(const auto* rdecl = dyn_cast_or_null<CXXRecordDecl>(decl); rdecl->getNumBases() >= 1) {
        for(const auto& base : rdecl->bases()) {
            const auto* rd = base.getType()->getAsRecordDecl();

            if(const auto* cxxRd = dyn_cast_or_null<CXXRecordDecl>(rd); not cxxRd or not cxxRd->isPolymorphic()) {
                continue;
            } else if(const CXXRecordDecl* ret = GetFirstPolymorphicBase(rd)) {
                return ret;
            }

            break;
        }
    }

    return dyn_cast_or_null<CXXRecordDecl>(decl);
}
//-----------------------------------------------------------------------------

void CfrontCodeGenerator::InsertArg(const CXXMemberCallExpr* stmt)
{
    if(const auto* me = dyn_cast_or_null<MemberExpr>(stmt->getCallee())) {
        auto*      obj = me->getBase();
        const bool isPointer{obj->getType()->isPointerType()};

        if(const bool isReference = IsReferenceType(dyn_cast_or_null<VarDecl>(obj->getReferencedDeclOfCallee()));
           not isPointer and not isReference) {
            obj = Ref(obj);
        }

        if(const auto* matExpr = dyn_cast_or_null<MaterializeTemporaryExpr>(me->getBase())) {
            if(const auto* tmpExpr = dyn_cast_or_null<CXXBindTemporaryExpr>(matExpr->getSubExpr())) {
                if(const auto* tmpObjExpr = dyn_cast_or_null<CXXTemporaryObjectExpr>(tmpExpr->getSubExpr())) {
                    obj = const_cast<CXXTemporaryObjectExpr*>(tmpObjExpr);
                }
            }
        }

        auto* memDecl = me->getMemberDecl();

        if(const auto* ar = dyn_cast_or_null<ConstantArrayType>(obj->getType())) {
            if(const auto* dtor = dyn_cast_or_null<CXXDestructorDecl>(memDecl)) {
                // ignore the reference
                InsertArg(CallVecDtor(dyn_cast_or_null<UnaryOperator>(obj)->getSubExpr(), ar));
                return;
            }
        }

        SmallVector<Expr*, 16> params{obj};
        auto*                  ncStmt = const_cast<CXXMemberCallExpr*>(stmt);
        params.append(ncStmt->arg_begin(), ncStmt->arg_end());

        if(auto* md = dyn_cast_or_null<CXXMethodDecl>(memDecl); md and md->isVirtual()) {
            auto& vtblData    = VtableData();
            auto* cls         = md->getParent();
            auto  vRecordDecl = GetFirstPolymorphicBase(cls);
            auto* vtblField   = VtableData().VtblPtrField(vRecordDecl);

            // -- cast to function signature: void Fun(struct X*)

            auto destType = not isPointer ? Ptr(obj->getType()) : obj->getType();
            auto atype    = isPointer ? obj->getType()->getPointeeType() : obj->getType();
            auto idx      = mVirtualFunctions[{md, {atype->getAsCXXRecordDecl(), vRecordDecl}}];

            // a->__vptr[1];  #1
            auto* accessVptr   = AccessMember(Paren(obj), vtblField, true);
            auto* vtblArrayPos = ArraySubscript(accessVptr, idx, vtblField->getType());

            auto* p             = Paren(vtblArrayPos);                 // ( #1 ) #2
            auto* accessMemberF = AccessMember(p, vtblData.f, false);  // #2.f  #3

            // (void (*)(struct X*) (#3)
            params_vector ps{{kwInternalThis, destType}};
            auto*         funcPtrFuncDecl = Function("__dummy"sv, VoidTy(), ps);
            auto*         reicast         = ReinterpretCast(funcPtrFuncDecl->getType(), accessMemberF, true);

            auto* p4 = Paren(reicast);  // (#4)
            auto  p5 = Dref(p4);        // *#5
            auto* p6 = Paren(p5);       // (#5)  #6

            // -- call with possible this pointer adjustment

            auto* p7 = AccessMember(p, vtblData.d, false);                        // a->__vptr[1];  #7
            auto* p8 = ReinterpretCast(GetGlobalAST().CharTy, Paren(obj), true);  // (#7) #8

            auto* p9 = ReinterpretCast(destType, p8);

            auto* p10 = Paren(p9);
            auto* p11 = Plus(p10, p7);  // #7 + #8    #9
            auto* p12 = Paren(p11);

            // Use the modified object parameter
            params[0] = p12;
            InsertArg(CallExpr::Create(GetGlobalAST(), p6, params, p6->getType(), VK_LValue, {}, {}));

        } else {
            InsertArg(Call(GetSpecialMemberName(memDecl), params));
        }

    } else {
        CodeGenerator::InsertArg(stmt);
    }
}
//-----------------------------------------------------------------------------

void CfrontCodeGenerator::InsertArg(const FunctionDecl* stmt)
{
    if(not stmt->isMain()) {
        CodeGenerator::InsertArg(stmt);
        return;
    }

    params_store           params{};
    SmallVector<Expr*, 16> args{};

    for(auto* param : stmt->parameters()) {
        params.emplace_back(GetName(*param), param->getType());
        args.push_back(mkDeclRefExpr(param));
    }

    auto mainName{"main"sv};
    auto trampolinMainName{BuildInternalVarName(mainName)};

    auto* intMain = Function(trampolinMainName, stmt->getReturnType(), to_params_view(params));
    intMain->setBody(stmt->getBody());
    intMain->setHasImplicitReturnZero(true);

    InsertArg(intMain);
    mOutputFormatHelper.AppendNewLine();

    auto* mainRetVar = Variable("ret"sv, stmt->getReturnType());
    mainRetVar->setInit(Call(trampolinMainName, args));

    auto* mainRetVarDeclStmt = mkDeclStmt(mainRetVar);

    StmtsContainer bodyStmts{Call(cxaStart, {}), mainRetVarDeclStmt, Call(cxaAtExit, {}), Return(mainRetVar)};

    auto* body    = mkCompoundStmt({bodyStmts});
    auto* modMain = Function(mainName, stmt->getReturnType(), to_params_view(params));
    modMain->setBody(body);

    CodeGenerator::InsertArg(modMain);
}
//-----------------------------------------------------------------------------

void CfrontCodeGenerator::InsertArg(const CXXConstructExpr* stmt)
{
    if(P0315Visitor dt{*this}; not dt.TraverseType(stmt->getType())) {
        if(not mLambdaStack.empty()) {
            for(const auto& e : mLambdaStack) {
                RETURN_IF(LambdaCallerType::VarDecl == e.callerType());
            }
        }
    }

    auto  ctor     = stmt->getConstructor();
    auto  ctorName = GetSpecialMemberName(ctor);
    auto* vd       = dyn_cast_or_null<VarDecl>(mLastDecl);

#if 0   
   // XXX: An expression like C c[4]{4,5,6} with C having a ctor does show up wrong at the moment. 
    if(not ar) {
        ar = dyn_cast_or_null<ConstantArrayType>(vd->getType());

        if(ar) {
            mLastStmt->dump();
            openScope = true;
            // mOutputFormatHelper.CloseScopeWithSemi();
            mOutputFormatHelper.AppendSemiNewLine();
        }
    }
#endif

    auto InsertCallCtor = [&](Expr* varNameRef) {
        SmallVector<Expr*, 16> args{Cast(varNameRef, Ptr(stmt->getType()))};

        for(int i = 0; auto* arg : stmt->arguments()) {
            if(IsCopyOrMoveCtor(ctor) or ctor->getParamDecl(i)->getType()->isReferenceType()) {
                args.push_back(Ref(arg));

            } else {
                args.push_back(const_cast<Expr*>(arg));
            }

            ++i;
        }

        InsertArg(Call(ctorName, args));
    };

    // For an array we need to call __vec_new
    if(const auto* ar = dyn_cast_or_null<ConstantArrayType>(stmt->getType())) {
        if(not HasCtor(ar->getElementType())) {
            mInsertSemi = false;
            return;
        }

        InsertArg(CallVecCtor(ctorName, vd, ar->getElementType(), Int32(GetSize(ar))));

    } else if(const auto* tmpObjectExpr = dyn_cast_or_null<CXXTemporaryObjectExpr>(stmt); vd and not tmpObjectExpr) {
        if(not HasCtor(vd->getType())) {
            mInsertSemi = false;
        } else {
            auto* varNameRef = Ref(mkDeclRefExpr(vd));

            InsertCallCtor(varNameRef);
        }

    } else if(tmpObjectExpr) {
        auto* varNameRef = Ref(mkVarDeclRefExpr(GetName(*tmpObjectExpr), stmt->getType()));

        if(not HasCtor(stmt->getType())) {
            InsertArg(varNameRef);
            return;
        }

        InsertCallCtor(varNameRef);

    } else {
        InsertArg(CallConstructor(
            stmt->getType(), Ptr(GetRecordDeclType(ctor)), nullptr, ArgsToExprVector(stmt), DoCast::Yes));
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
