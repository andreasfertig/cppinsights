/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

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

static MemberExpr* AccessMember(std::string_view name, const ValueDecl* vd, QualType type)
{
    auto* rhsDeclRef    = mkVarDeclRefExpr(name, type);
    auto* rhsMemberExpr = AccessMember(rhsDeclRef, vd);

    return rhsMemberExpr;
}
//-----------------------------------------------------------------------------

CodeGeneratorVariant::CodeGenerators::CodeGenerators(OutputFormatHelper&             _outputFormatHelper,
                                                     CodeGenerator::LambdaStackType& lambdaStack)
{
    if(GetInsightsOptions().UseShow2C) {
        new(&cfcg) CfrontCodeGenerator{_outputFormatHelper, lambdaStack, CodeGenerator::LambdaInInitCapture::No};
    } else {
        new(&cg) CodeGenerator{_outputFormatHelper, lambdaStack, CodeGenerator::LambdaInInitCapture::No};
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

    EnableGlobalInsert(GlobalInserts::FuncMalloc);

    if(stmt->isArray()) {
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

    if(body) {
        bodyStmts.AddBodyStmts(body);
    }

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
                bodyStmts.AddBodyStmts(CallConstructor(baseType, Ptr(baseType), nullptr, {}, DoCast::Yes));

                insertFields(baseType->getAsRecordDecl());
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

        auto* lhsDeclRef = mkVarDeclRefExpr(kwInternalThis, Ptr(ctorDecl->getType()));

        bodyStmts.AddBodyStmts(Return(lhsDeclRef));

        body    = mkCompoundStmt({bodyStmts});
        retType = parentType;

        // copy and move assignment op
    } else if(IsCopyOrMoveAssign(stmt)) {
        if(not stmt->doesThisDeclarationHaveABody() or stmt->isDefaulted()) {

            // we don't want the default generated body
            bodyStmts.clear();

            processBaseClassesAndFields(stmt->getParent());
        }

        auto* lhsDeclRef = mkVarDeclRefExpr(kwInternalThis, Ptr(stmt->getType()));

        bodyStmts.AddBodyStmts(Return(lhsDeclRef));

        body    = mkCompoundStmt({bodyStmts});
        retType = parentType;

    } else if(const auto* dtor = dyn_cast_or_null<CXXDestructorDecl>(stmt)) {
        // Based on: https://www.dre.vanderbilt.edu/~schmidt/PDF/C++-translation.pdf

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
                                     const CastKind&)
{
    // C does not have a rvalue notation and we already transformed the temporary into an object. Skip the cast to &&.
    if(not castDestType->isRValueReferenceType()) {
        mOutputFormatHelper.Append("(", GetName(castDestType), ")");
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

void CfrontCodeGenerator::InsertArg(const CXXRecordDecl* stmt)
{
    auto* recordDecl = Struct(GetName(*stmt));

    if(stmt->hasDefinition()) {
        auto AddField = [&](const FieldDecl* field) {
            auto* fieldDecl = mkFieldDecl(recordDecl, GetName(*field), field->getType());

            recordDecl->addDecl(fieldDecl);
        };

        // Insert field from base classes
        for(const auto& base : stmt->bases()) {
            // XXX: ignoring TemplateSpecializationType
            if(const auto* rd = base.getType().getCanonicalType()->getAsRecordDecl()) {
                for(const auto* d : rd->fields()) {
                    AddField(d);
                }
            }
        }

        // insert own fields
        for(const auto* d : stmt->fields()) {
            AddField(d);
        }

        if(recordDecl->field_empty()) {
            AddField(mkFieldDecl(recordDecl, "__dummy"sv, GetGlobalAST().CharTy));
        }

        recordDecl->completeDefinition();

#if 0
    // TypedefDecl above is not called
    auto& ctx = GetGlobalAST();
    auto et = ctx.getElaboratedType(ElaboratedTypeKeyword::ETK_Struct, nullptr, GetRecordDeclType(recordDecl), nullptr);
    auto* typedefDecl = Typedef(GetName(*stmt),et);
#endif

        mOutputFormatHelper.Append(kwTypedefSpace);
    }

    // use our freshly created recordDecl
    CodeGenerator::InsertArg(recordDecl);
    //    CodeGenerator::InsertArg(typedefDecl);

    // insert member functions except for the special member functions and classes defined inside this class
    OnceTrue firstRecordDecl{};
    for(const auto* d : stmt->decls()) {
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

void CfrontCodeGenerator::InsertArg(const CXXMemberCallExpr* stmt)
{
    if(const auto* me = dyn_cast_or_null<MemberExpr>(stmt->getCallee())) {
        auto* obj = me->getBase();

        const bool isReference = IsReferenceType(dyn_cast_or_null<VarDecl>(obj->getReferencedDeclOfCallee()));

        if(not obj->getType()->isPointerType() and not isReference) {
            obj = Ref(obj);
        }

        if(const auto* matExpr = dyn_cast_or_null<MaterializeTemporaryExpr>(me->getBase())) {
            if(const auto* tmpExpr = dyn_cast_or_null<CXXBindTemporaryExpr>(matExpr->getSubExpr())) {
                if(const auto* tmpObjExpr = dyn_cast_or_null<CXXTemporaryObjectExpr>(tmpExpr->getSubExpr())) {
                    obj = const_cast<CXXTemporaryObjectExpr*>(tmpObjExpr);
                }
            }
        }

        if(const auto* ar = dyn_cast_or_null<ConstantArrayType>(obj->getType())) {
            if(const auto* dtor = dyn_cast_or_null<CXXDestructorDecl>(me->getMemberDecl())) {
                // ignore the reference
                InsertArg(CallVecDtor(dyn_cast_or_null<UnaryOperator>(obj)->getSubExpr(), ar));
                return;
            }
        }

        SmallVector<Expr*, 16> params{obj};
        auto*                  ncStmt = const_cast<CXXMemberCallExpr*>(stmt);
        params.append(ncStmt->arg_begin(), ncStmt->arg_end());

        InsertArg(Call(GetSpecialMemberName(me->getMemberDecl()), params));

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

    auto  ctorName = GetSpecialMemberName(stmt->getConstructor());
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

    // For an array we need to call __vec_new
    if(const auto* ar = dyn_cast_or_null<ConstantArrayType>(stmt->getType())) {
        if(not HasCtor(ar->getElementType())) {
            mInsertSemi = false;
            return;
        }

        InsertArg(CallVecCtor(ctorName, vd, ar->getElementType(), Int32(GetSize(ar))));

    } else if(const auto* tmpObjectExpr = dyn_cast_or_null<CXXTemporaryObjectExpr>(stmt); vd and not tmpObjectExpr) {
        if(not HasCtor(vd->getType())) {
            // InsertArg(Ref(vd));
        }

    } else if(tmpObjectExpr) {
        auto* varNameRef = Ref(mkVarDeclRefExpr(GetName(*tmpObjectExpr), stmt->getType()));

        SmallVector<Expr*, 16> args{Cast(varNameRef, Ptr(stmt->getType()))};

        if(not HasCtor(stmt->getType())) {
            InsertArg(varNameRef);
            return;
        }

        for(auto* arg : stmt->arguments()) {
            if(IsCopyOrMoveCtor(stmt->getConstructor())) {
                args.push_back(Ref(arg));

            } else {
                args.push_back(const_cast<Expr*>(arg));
            }
        }

        InsertArg(Call(ctorName, args));

    } else {
        InsertArg(CallConstructor(stmt->getType(),
                                  Ptr(GetRecordDeclType(stmt->getConstructor())),
                                  nullptr,
                                  ArgsToExprVector(stmt),
                                  DoCast::Yes));
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights
