/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <unordered_map>

#include "CodeGenerator.h"
#include "DPrint.h"
#include "InsightsBase.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "InsightsStrCat.h"
#include "OutputFormatHelper.h"

#include "AutoStmtHandler.h"
#include "ClassOperatorHandler.h"
#include "CompilerGeneratedHandler.h"
#include "IfStmtHandler.h"
#include "ImplicitCastHandler.h"
#include "LambdaHandler.h"
#include "NRVOHandler.h"
#include "RangeForStmtHandler.h"
#include "StaticAssertHandler.h"
#include "StaticHandler.h"
#include "StdInitializerListHandler.h"
#include "StructuredBindingsHandler.h"
#include "TemplateHandler.h"
#include "UserDefinedLiteralHandler.h"
#include "version.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;
using namespace clang::insights;
//-----------------------------------------------------------------------------

static llvm::cl::OptionCategory gInsightCategory("Insights");
//-----------------------------------------------------------------------------

static llvm::cl::opt<bool> gStdinMode("stdin",
                                      llvm::cl::desc("Override source file's content (in the overlaying\n"
                                                     "virtual file system) with input from <stdin> and run\n"
                                                     "the tool on the new content with the compilation\n"
                                                     "options of the source file. This mode is currently\n"
                                                     "used for editor integration."),
                                      llvm::cl::init(false),
                                      llvm::cl::cat(gInsightCategory));
//-----------------------------------------------------------------------------

class CppInsightASTConsumer final : public ASTConsumer
{
public:
    explicit CppInsightASTConsumer(Rewriter& rewriter)
    : mMatcher{}
    , mRangeForStmtHandler{rewriter, mMatcher}
    , mStructuredBindingsHandler{rewriter, mMatcher}
    , mClassOperatorHandler{rewriter, mMatcher}
    , mLambdaHandler{rewriter, mMatcher}
    , mCompilerGeneratedHandler{rewriter, mMatcher}
    , mStaticHandler{rewriter, mMatcher}
    , mStaticAssertHandler{rewriter, mMatcher}
    , mTemplateHandler{rewriter, mMatcher}
    , mImplicitCastHandler{rewriter, mMatcher}
    , mIfStmtHandler{rewriter, mMatcher}
    , mAutoStmtHandler{rewriter, mMatcher}
    , mNrvoHandler{rewriter, mMatcher}
    , mUserDefinedLiteralHandler{rewriter, mMatcher}
    , mStdInitializerListHandler{rewriter, mMatcher}
    {
    }

    void HandleTranslationUnit(ASTContext& context) override { mMatcher.matchAST(context); }

private:
    MatchFinder               mMatcher;
    RangeForStmtHandler       mRangeForStmtHandler;
    StructuredBindingsHandler mStructuredBindingsHandler;
    ClassOperatorHandler      mClassOperatorHandler;
    LambdaHandler             mLambdaHandler;
    CompilerGeneratedHandler  mCompilerGeneratedHandler;
    StaticHandler             mStaticHandler;
    StaticAssertHandler       mStaticAssertHandler;
    TemplateHandler           mTemplateHandler;
    ImplicitCastHandler       mImplicitCastHandler;
    IfStmtHandler             mIfStmtHandler;
    AutoStmtHandler           mAutoStmtHandler;
    NRVOHandler               mNrvoHandler;
    UserDefinedLiteralHandler mUserDefinedLiteralHandler;
    StdInitializerListHandler mStdInitializerListHandler;
};
//-----------------------------------------------------------------------------

class CppInsightFrontendAction final : public ASTFrontendAction
{
public:
    CppInsightFrontendAction() = default;
    void EndSourceFileAction() override
    {
        mRewriter.getEditBuffer(mRewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
    }

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& CI, StringRef /*file*/) override
    {
        mRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        return llvm::make_unique<CppInsightASTConsumer>(mRewriter);
    }

private:
    Rewriter mRewriter;
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
    llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
    llvm::cl::HideUnrelatedOptions(gInsightCategory);
    llvm::cl::SetVersionPrinter(&PrintVersion);

    CommonOptionsParser op(argc, argv, gInsightCategory);
    ClangTool           tool(op.getCompilations(), op.getSourcePathList());

    llvm::StringRef sourceFilePath = op.getSourcePathList().front();
    // In STDINMode, we override the file content with the <stdin> input.
    // Since `tool.mapVirtualFile` takes `StringRef`, we define `Code` outside of
    // the if-block so that `Code` is not released after the if-block.
    std::unique_ptr<llvm::MemoryBuffer> inMemoryCode{};

    if(gStdinMode) {
        assert(op.getSourcePathList().size() == 1 && "Expect exactly one file path in STDINMode.");
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

        tool.mapVirtualFile(sourceFilePath, inMemoryCode->getBuffer());
    }

    return tool.run(newFrontendActionFactory<CppInsightFrontendAction>().get());
}
//-----------------------------------------------------------------------------
