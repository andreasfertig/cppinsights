/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include <array>
#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>

#include "CodeGenerator.h"
#include "DPrint.h"
#include "Insights.h"
#include "version.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace clang::insights;
//-----------------------------------------------------------------------------

static InsightsOptions gInsightsOptions{};
//-----------------------------------------------------------------------------

const InsightsOptions& GetInsightsOptions()
{
    return gInsightsOptions;
}
//-----------------------------------------------------------------------------

static llvm::cl::OptionCategory gInsightCategory("Insights"sv);
//-----------------------------------------------------------------------------

static llvm::cl::OptionCategory gInsightEduCategory(
    "Insights-Educational"sv,
    "This transformations are only for education purposes. The resulting code most likely does not compile."sv);
//-----------------------------------------------------------------------------

static llvm::cl::opt<bool> gStdinMode("stdin",
                                      llvm::cl::desc("Override source file's content (in the overlaying\n"
                                                     "virtual file system) with input from <stdin> and run\n"
                                                     "the tool on the new content with the compilation\n"
                                                     "options of the source file. This mode is currently\n"
                                                     "used for editor integration."sv),
                                      llvm::cl::init(false),
                                      llvm::cl::cat(gInsightCategory));
//-----------------------------------------------------------------------------

static llvm::cl::opt<bool>
    gUseLibCpp("use-libc++", llvm::cl::desc("Use libc++."sv), llvm::cl::init(false), llvm::cl::cat(gInsightCategory));
//-----------------------------------------------------------------------------

#define INSIGHTS_OPT(option, name, deflt, description, category)                                                       \
    static llvm::cl::opt<bool, true> g##name(option,                                                                   \
                                             llvm::cl::desc(std::string_view{description}),                            \
                                             llvm::cl::NotHidden,                                                      \
                                             llvm::cl::location(gInsightsOptions.name),                                \
                                             llvm::cl::init(deflt),                                                    \
                                             llvm::cl::cat(category));
//-----------------------------------------------------------------------------

#include "InsightsOptions.def"
//-----------------------------------------------------------------------------

static const ASTContext* gAST{};
const ASTContext&        GetGlobalAST()
{
    return *gAST;
}
//-----------------------------------------------------------------------------

static const CompilerInstance* gCI{};
const CompilerInstance&        GetGlobalCI()
{
    return *gCI;
}
//-----------------------------------------------------------------------------

namespace clang::insights {
std::string EmitGlobalVariableCtors();

using GlobalInsertMap = std::pair<bool, std::string_view>;

static constinit std::array<GlobalInsertMap, static_cast<size_t>(GlobalInserts::MAX)> gGlobalInserts{};

void AddGLobalInsertMapEntry(GlobalInserts idx, std::string_view value)
{
    gGlobalInserts[static_cast<size_t>(idx)] = {false, value};
}

void EnableGlobalInsert(GlobalInserts idx)
{
    gGlobalInserts[static_cast<size_t>(idx)].first = true;
}

}  // namespace clang::insights

using IncludeData = std::pair<const SourceLocation, std::string>;

class FindIncludes : public PPCallbacks
{
    SourceManager&            mSm;
    Preprocessor&             mPP;
    std::vector<IncludeData>& mIncludes;

public:
    FindIncludes(SourceManager& sm, Preprocessor& pp, std::vector<IncludeData>& incData)
    : PPCallbacks{}
    , mSm{sm}
    , mPP{pp}
    , mIncludes{incData}
    {
    }

    void InclusionDirective(SourceLocation hashLoc,
                            const Token& /*IncludeTok*/,
                            StringRef fileName,
                            bool      isAngled,
                            CharSourceRange /*FilenameRange*/,
                            OptionalFileEntryRef /*file*/,
                            StringRef /*SearchPath*/,
                            StringRef /*RelativePath*/,
                            const Module* /*Imported*/,
                            SrcMgr::CharacteristicKind /*FileType*/) override
    {
        auto expansionLoc = mSm.getExpansionLoc(hashLoc);

        if(expansionLoc.isInvalid() or mSm.isInSystemHeader(expansionLoc)) {
            return;
        }

        // XXX: distinguish between include and import via the IncludeTok
        if(isAngled) {
            mIncludes.emplace_back(expansionLoc, StrCat("#include <"sv, fileName, ">\n"sv));

        } else {
            mIncludes.emplace_back(expansionLoc, StrCat("#include \""sv, fileName, "\"\n"sv));
        }
    }

    void MacroDefined(const Token& macroNameTok, const MacroDirective* md) override
    {
        const auto loc = md->getLocation();
        if(not mSm.isWrittenInMainFile(loc)) {
            return;
        }

        auto name = mPP.getSpelling(macroNameTok);

        if(not name.starts_with("INSIGHTS_"sv)) {
            return;
        }

        mIncludes.emplace_back(loc, StrCat("#define "sv, name, "\n"sv));
    }
};

class CppInsightASTConsumer final : public ASTConsumer
{
    Rewriter&                 mRewriter;
    std::vector<IncludeData>& mIncludes;

public:
    explicit CppInsightASTConsumer(Rewriter& rewriter, std::vector<IncludeData>& includes)
    : ASTConsumer{}
    , mRewriter{rewriter}
    , mIncludes{includes}
    {
        if(GetInsightsOptions().UseShow2C) {
            gInsightsOptions.ShowLifetime = true;
        }

        if(GetInsightsOptions().ShowLifetime) {
            gInsightsOptions.UseShowInitializerList = true;
        }
    }

    void HandleTranslationUnit(ASTContext& context) override
    {
        gAST     = &context;
        auto& sm = context.getSourceManager();

        auto isExpansionInSystemHeader = [&sm](const Decl* d) {
            auto expansionLoc = sm.getExpansionLoc(d->getLocation());

            return expansionLoc.isInvalid() or sm.isInSystemHeader(expansionLoc);
        };

        const auto& mainFileId = sm.getMainFileID();

        mRewriter.ReplaceText({sm.getLocForStartOfFile(mainFileId), sm.getLocForEndOfFile(mainFileId)}, "");

        OutputFormatHelper   outputFormatHelper{};
        CodeGeneratorVariant codeGenerator{outputFormatHelper};

        auto include = mIncludes.begin();

        auto insertBlankLineIfRequired = [&](std::optional<SourceLocation>& lastLoc, SourceLocation nextLoc) {
            if(lastLoc.has_value() and
               (2 <= (sm.getSpellingLineNumber(nextLoc) - sm.getSpellingLineNumber(lastLoc.value())))) {
                outputFormatHelper.AppendNewLine();
            }

            lastLoc = nextLoc;
        };

        for(std::optional<SourceLocation> lastLoc{}; const auto* d : context.getTranslationUnitDecl()->decls()) {
            if(isExpansionInSystemHeader(d)) {
                continue;
            }

            // includes before this decl
            for(; (mIncludes.end() != include) and (include->first < d->getLocation()); include = std::next(include)) {
                insertBlankLineIfRequired(lastLoc, include->first);
                outputFormatHelper.Append(include->second);
            }

            // ignore includes inside this decl
            include = std::find_if_not(include, mIncludes.end(), [&](auto& inc) {
                return ((inc.first >= d->getLocation()) and (inc.first <= d->getEndLoc()));
            });

            if(isa<LinkageSpecDecl>(d) and d->isImplicit()) {
                continue;

                // Only handle explicit specializations here. Implicit ones are handled by the `VarTemplateDecl`
                // itself.
            } else if(const auto* vdspec = dyn_cast_or_null<VarTemplateSpecializationDecl>(d);
                      vdspec and (TSK_ExplicitSpecialization != vdspec->getSpecializationKind())) {
                continue;
            }

            insertBlankLineIfRequired(lastLoc, d->getLocation());

            codeGenerator->InsertArg(d);
        }

        std::string insightsIncludes{};

        if(GetInsightsOptions().ShowCoroutineTransformation) {
            insightsIncludes.append(
                R"(/*************************************************************************************
 * NOTE: The coroutine transformation you've enabled is a hand coded transformation! *
 *       Most of it is _not_ present in the AST. What you see is an approximation.   *
 *************************************************************************************/
)"sv);
        } else if(GetInsightsOptions().UseShow2C or GetInsightsOptions().ShowLifetime) {
            insightsIncludes.append(
                R"(/*************************************************************************************
 * NOTE: This an educational hand-rolled transformation. Things can be incorrect or  *
 * buggy.                                                                            *
 *************************************************************************************/
)"sv);
        }

        // Check whether we had static local variables which we transformed. Then for the placement-new we need to
        // include the header <new>.
        std::string inserts{};
        for(const auto& [active, value] : gGlobalInserts) {
            if(not active) {
                continue;
            }

            inserts.append(value);
            inserts.append("\n"sv);
        }

        if(not inserts.empty()) {
            insightsIncludes.append(inserts);
            insightsIncludes.append("\n");
        }

        outputFormatHelper.InsertAt(0, insightsIncludes);

        mRewriter.InsertText(sm.getLocForStartOfFile(mainFileId), outputFormatHelper.GetString());

        if(GetInsightsOptions().UseShow2C) {
            const auto& fileEntry = sm.getFileEntryForID(mainFileId);
            auto        cxaStart  = EmitGlobalVariableCtors();
            const auto  cxaLoc    = sm.translateFileLineCol(fileEntry, fileEntry->getSize(), 1);

            mRewriter.InsertText(cxaLoc, cxaStart);
        }
    }
};
//-----------------------------------------------------------------------------

class CppInsightFrontendAction final : public ASTFrontendAction
{
    Rewriter                 mRewriter{};
    std::vector<IncludeData> mIncludes{};

public:
    CppInsightFrontendAction() = default;
    void EndSourceFileAction() override
    {
        mRewriter.getEditBuffer(mRewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
    }

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& CI, StringRef /*file*/) override
    {
        gCI = &CI;

        Preprocessor& pp = CI.getPreprocessor();
        pp.addPPCallbacks(std::make_unique<FindIncludes>(CI.getSourceManager(), pp, mIncludes));

        mRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        return std::make_unique<CppInsightASTConsumer>(mRewriter, mIncludes);
    }
};
//-----------------------------------------------------------------------------

#include "clang/Basic/Version.h"

static void PrintVersion(raw_ostream& ostream)
{
    ostream << "cpp-insights " << INSIGHTS_VERSION << " https://cppinsights.io (" << GIT_REPO_URL << " "
            << GIT_COMMIT_HASH << ")"
            << "\n";

#ifdef INSIGHTS_DEBUG
    ostream << "  Build with debug enabled\n";
#endif
    ostream << "  LLVM  Revision: " << clang::getLLVMRevision() << '\n';
    ostream << "  Clang Revision: " << clang::getClangFullCPPVersion() << '\n';
}
//-----------------------------------------------------------------------------

int main(int argc, const char** argv)
{
    // Headers go first
    using enum GlobalInserts;
    AddGLobalInsertMapEntry(HeaderNew,
                            "#include <new> // for thread-safe static's placement new\n#include <stdint.h> // for "
                            "uint64_t under Linux/GCC"sv);
    AddGLobalInsertMapEntry(HeaderException, "#include <exception> // for noexcept transformation"sv);
    AddGLobalInsertMapEntry(HeaderUtility, "#include <utility> // std::move"sv);
    AddGLobalInsertMapEntry(HeaderStddef, "#include <stddef.h> // NULL and more"sv);
    AddGLobalInsertMapEntry(HeaderAssert, "#include <assert.h> // _Static_assert"sv);

    // Now all the forward declared functions
    AddGLobalInsertMapEntry(FuncCxaStart, "void __cxa_start(void);"sv);
    AddGLobalInsertMapEntry(FuncCxaAtExit, "void __cxa_atexit(void);"sv);
    AddGLobalInsertMapEntry(FuncMalloc, "void* malloc(unsigned int);"sv);
    AddGLobalInsertMapEntry(FuncFree, R"(extern "C" void free(void*);)"sv);
    AddGLobalInsertMapEntry(FuncMemset, R"(extern "C" void* memset(void*, int, unsigned int);)"sv);
    AddGLobalInsertMapEntry(FuncMemcpy, R"(void* memcpy(void*, const void*, unsigned int);)"sv);
    AddGLobalInsertMapEntry(
        FuncCxaVecNew,
        R"(extern "C" void* __cxa_vec_new(void*, unsigned int, unsigned int, unsigned int, void* (*)(void*), void* (*)(void*));)"sv);
    AddGLobalInsertMapEntry(
        FuncCxaVecCtor,
        R"(extern "C" void* __cxa_vec_ctor(void*, unsigned int, unsigned int, unsigned int, void* (*)(void*), void* (*)(void*));)"sv);
    AddGLobalInsertMapEntry(
        FuncCxaVecDel,
        R"(extern "C" void __cxa_vec_delete(void *, unsigned int, unsigned int, void* (*destructor)(void *) );)"sv);
    AddGLobalInsertMapEntry(
        FuncCxaVecDtor,
        R"(extern "C" void __cxa_vec_dtor(void *, unsigned int, unsigned int, void* (*destructor)(void *) );)"sv);

    llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
    llvm::cl::HideUnrelatedOptions(gInsightCategory);
    llvm::cl::SetVersionPrinter(&PrintVersion);

    auto opExpected = CommonOptionsParser::create(argc, argv, gInsightCategory);

    if(auto err = opExpected.takeError()) {
        llvm::errs() << toString(std::move(err)) << "\n";
        return 1;
    }

    // In STDINMode, we override the file content with the <stdin> input.
    // Since `tool.mapVirtualFile` takes `StringRef`, we define `Code` outside of
    // the if-block so that `Code` is not released after the if-block.
    std::unique_ptr<llvm::MemoryBuffer> inMemoryCode{};

    CommonOptionsParser& op{opExpected.get()};
    ClangTool            tool(op.getCompilations(), op.getSourcePathList());

    if(gStdinMode) {
        if(op.getSourcePathList().size() != 1) {
            llvm::errs() << "Expect exactly one file path in STDINMode.\n"sv;
            return 1;
        }

        llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> codeOrErr = llvm::MemoryBuffer::getSTDIN();

        if(const std::error_code errorCode = codeOrErr.getError()) {
            llvm::errs() << errorCode.message() << "\n";
            return 1;
        }

        inMemoryCode = std::move(codeOrErr.get());

        if(inMemoryCode->getBufferSize() == 0) {
            Error("empty file\n");
            return 1;  // Skip empty files.
        }

        llvm::StringRef sourceFilePath = op.getSourcePathList().front();
        tool.mapVirtualFile(sourceFilePath, inMemoryCode->getBuffer());
    }

    auto prependArgument = [&](auto arg) {
        tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(arg, ArgumentInsertPosition::BEGIN));
    };

    // Special handling to spare users to figure out what include paths to add.

    // For some reason, Clang on Apple seems to require an additional hint for the C++ headers.
#ifdef __APPLE__
    gUseLibCpp = true;
#endif /* __APPLE__ */

    if(gUseLibCpp) {
        prependArgument(INSIGHTS_LLVM_INCLUDE_DIR);
        prependArgument("-stdlib=libc++");

#if IS_CLANG_NEWER_THAN(15)
        prependArgument("-fexperimental-library");
#endif

#ifdef __APPLE__
        prependArgument("-nostdinc++");  // macos Monterey
#endif                                   /* __APPLE__ */
    }

    prependArgument(INSIGHTS_CLANG_RESOURCE_INCLUDE_DIR);
    prependArgument(INSIGHTS_CLANG_RESOURCE_DIR);

    if(GetInsightsOptions().UseShow2C) {
        EnableGlobalInsert(FuncCxaStart);
        EnableGlobalInsert(FuncCxaAtExit);
    }

    return tool.run(newFrontendActionFactory<CppInsightFrontendAction>().get());
}
//-----------------------------------------------------------------------------
