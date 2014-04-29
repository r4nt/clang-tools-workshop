//===--- tools/extra/clang-tidy/ClangTidy.cpp - Clang tidy tool -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
///  \file This file implements a clang-tidy tool.
///
///  This tool uses the Clang Tooling infrastructure, see
///    http://clang.llvm.org/docs/HowToSetupToolingForLLVM.html
///  for details on setting it up with LLVM source tree.
///
//===----------------------------------------------------------------------===//

#include "ClangTidy.h"
#include "ClangTidyDiagnosticConsumer.h"
#include "ClangTidyModuleRegistry.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/MultiplexConsumer.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Rewrite/Frontend/FixItRewriter.h"
#include "clang/Rewrite/Frontend/FrontendActions.h"
#include "clang/StaticAnalyzer/Frontend/AnalysisConsumer.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Signals.h"
#include <algorithm>
#include <utility>
#include <vector>

using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

namespace clang {
namespace tidy {

namespace {
static const char *AnalyzerCheckNamePrefix = "clang-analyzer-";

static StringRef StaticAnalyzerChecks[] = {
#define GET_CHECKERS
#define CHECKER(FULLNAME, CLASS, DESCFILE, HELPTEXT, GROUPINDEX, HIDDEN)       \
  FULLNAME,
#include "../../../lib/StaticAnalyzer/Checkers/Checkers.inc"
#undef CHECKER
#undef GET_CHECKERS
};

class AnalyzerDiagnosticConsumer : public ento::PathDiagnosticConsumer {
public:
  AnalyzerDiagnosticConsumer(ClangTidyContext &Context) : Context(Context) {}

  void FlushDiagnosticsImpl(std::vector<const ento::PathDiagnostic *> &Diags,
                            FilesMade *filesMade) override {
    for (const ento::PathDiagnostic *PD : Diags) {
      SmallString<64> CheckName(AnalyzerCheckNamePrefix);
      CheckName += PD->getCheckName();
      Context.diag(CheckName, PD->getLocation().asLocation(),
                   PD->getShortDescription())
          << PD->path.back()->getRanges();

      for (const auto &DiagPiece :
           PD->path.flatten(/*ShouldFlattenMacros=*/true)) {
        Context.diag(CheckName, DiagPiece->getLocation().asLocation(),
                     DiagPiece->getString(), DiagnosticIDs::Note)
            << DiagPiece->getRanges();
      }
    }
  }

  StringRef getName() const override { return "ClangTidyDiags"; }
  bool supportsLogicalOpControlFlow() const override { return true; }
  bool supportsCrossFileDiagnostics() const override { return true; }

private:
  ClangTidyContext &Context;
};

class ErrorReporter {
public:
  ErrorReporter(bool ApplyFixes)
      : Files(FileSystemOptions()), DiagOpts(new DiagnosticOptions()),
        DiagPrinter(new TextDiagnosticPrinter(llvm::outs(), &*DiagOpts)),
        Diags(IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs), &*DiagOpts,
              DiagPrinter),
        SourceMgr(Diags, Files), Rewrite(SourceMgr, LangOpts),
        ApplyFixes(ApplyFixes), TotalFixes(0), AppliedFixes(0) {
    DiagOpts->ShowColors = llvm::sys::Process::StandardOutHasColors();
    DiagPrinter->BeginSourceFile(LangOpts);
  }

  void reportDiagnostic(const ClangTidyMessage &Message,
                        DiagnosticsEngine::Level Level,
                        const tooling::Replacements *Fixes = nullptr) {
    SourceLocation Loc = getLocation(Message.FilePath, Message.FileOffset);
    // Contains a pair for each attempted fix: location and whether the fix was
    // applied successfully.
    SmallVector<std::pair<SourceLocation, bool>, 4> FixLocations;
    {
      DiagnosticBuilder Diag =
          Diags.Report(Loc, Diags.getCustomDiagID(Level, "%0"))
          << Message.Message;
      if (Fixes != NULL) {
        for (const tooling::Replacement &Fix : *Fixes) {
          SourceLocation FixLoc =
              getLocation(Fix.getFilePath(), Fix.getOffset());
          SourceLocation FixEndLoc = FixLoc.getLocWithOffset(Fix.getLength());
          Diag << FixItHint::CreateReplacement(SourceRange(FixLoc, FixEndLoc),
                                               Fix.getReplacementText());
          ++TotalFixes;
          if (ApplyFixes) {
            bool Success = Fix.isApplicable() && Fix.apply(Rewrite);
            if (Success)
              ++AppliedFixes;
            FixLocations.push_back(std::make_pair(FixLoc, Success));
          }
        }
      }
    }
    for (auto Fix : FixLocations) {
      Diags.Report(Fix.first, Fix.second ? diag::note_fixit_applied
                                         : diag::note_fixit_failed);
    }
  }

  void Finish() {
    // FIXME: Run clang-format on changes.
    if (ApplyFixes && TotalFixes > 0) {
      llvm::errs() << "clang-tidy applied " << AppliedFixes << " of "
                   << TotalFixes << " suggested fixes.\n";
      Rewrite.overwriteChangedFiles();
    }
  }

private:
  SourceLocation getLocation(StringRef FilePath, unsigned Offset) {
    if (FilePath.empty())
      return SourceLocation();

    const FileEntry *File = SourceMgr.getFileManager().getFile(FilePath);
    FileID ID = SourceMgr.createFileID(File, SourceLocation(), SrcMgr::C_User);
    return SourceMgr.getLocForStartOfFile(ID).getLocWithOffset(Offset);
  }

  FileManager Files;
  LangOptions LangOpts; // FIXME: use langopts from each original file
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts;
  DiagnosticConsumer *DiagPrinter;
  DiagnosticsEngine Diags;
  SourceManager SourceMgr;
  Rewriter Rewrite;
  bool ApplyFixes;
  unsigned TotalFixes;
  unsigned AppliedFixes;
};

} // namespace

ClangTidyASTConsumerFactory::ClangTidyASTConsumerFactory(
    ClangTidyContext &Context)
    : Context(Context), CheckFactories(new ClangTidyCheckFactories) {
  for (ClangTidyModuleRegistry::iterator I = ClangTidyModuleRegistry::begin(),
                                         E = ClangTidyModuleRegistry::end();
       I != E; ++I) {
    std::unique_ptr<ClangTidyModule> Module(I->instantiate());
    Module->addCheckFactories(*CheckFactories);
  }

  CheckFactories->createChecks(Context.getChecksFilter(), Checks);

  for (ClangTidyCheck *Check : Checks) {
    Check->setContext(&Context);
    Check->registerMatchers(&Finder);
  }
}

ClangTidyASTConsumerFactory::~ClangTidyASTConsumerFactory() {
  for (ClangTidyCheck *Check : Checks)
    delete Check;
}

clang::ASTConsumer *ClangTidyASTConsumerFactory::CreateASTConsumer(
    clang::CompilerInstance &Compiler, StringRef File) {
  // FIXME: Move this to a separate method, so that CreateASTConsumer doesn't
  // modify Compiler.
  Context.setSourceManager(&Compiler.getSourceManager());
  for (ClangTidyCheck *Check : Checks)
    Check->registerPPCallbacks(Compiler);

  SmallVector<ASTConsumer *, 2> Consumers;
  if (!CheckFactories->empty())
    Consumers.push_back(Finder.newASTConsumer());

  AnalyzerOptionsRef Options = Compiler.getAnalyzerOpts();
  Options->CheckersControlList = getCheckersControlList();
  if (!Options->CheckersControlList.empty()) {
    Options->AnalysisStoreOpt = RegionStoreModel;
    Options->AnalysisDiagOpt = PD_NONE;
    Options->AnalyzeNestedBlocks = true;
    Options->eagerlyAssumeBinOpBifurcation = true;
    ento::AnalysisASTConsumer *AnalysisConsumer = ento::CreateAnalysisConsumer(
        Compiler.getPreprocessor(), Compiler.getFrontendOpts().OutputFile,
        Options, Compiler.getFrontendOpts().Plugins);
    AnalysisConsumer->AddDiagnosticConsumer(
        new AnalyzerDiagnosticConsumer(Context));
    Consumers.push_back(AnalysisConsumer);
  }
  return new MultiplexConsumer(Consumers);
}

std::vector<std::string> ClangTidyASTConsumerFactory::getCheckNames() {
  std::vector<std::string> CheckNames;
  for (const auto &CheckFactory : *CheckFactories) {
    if (Context.getChecksFilter().isCheckEnabled(CheckFactory.first))
      CheckNames.push_back(CheckFactory.first);
  }

  for (const auto &AnalyzerCheck : getCheckersControlList())
    CheckNames.push_back(AnalyzerCheckNamePrefix + AnalyzerCheck.first);

  std::sort(CheckNames.begin(), CheckNames.end());
  return CheckNames;
}

ClangTidyASTConsumerFactory::CheckersList
ClangTidyASTConsumerFactory::getCheckersControlList() {
  CheckersList List;

  bool AnalyzerChecksEnabled = false;
  for (StringRef CheckName : StaticAnalyzerChecks) {
    std::string Checker((AnalyzerCheckNamePrefix + CheckName).str());
    AnalyzerChecksEnabled |=
        Context.getChecksFilter().isCheckEnabled(Checker) &&
        !CheckName.startswith("debug");
  }

  if (AnalyzerChecksEnabled) {
    // Run our regex against all possible static analyzer checkers.  Note that
    // debug checkers print values / run programs to visualize the CFG and are
    // thus not applicable to clang-tidy in general.
    //
    // Always add all core checkers if any other static analyzer checks are
    // enabled. This is currently necessary, as other path sensitive checks
    // rely on the core checkers.
    for (StringRef CheckName : StaticAnalyzerChecks) {
      std::string Checker((AnalyzerCheckNamePrefix + CheckName).str());

      if (CheckName.startswith("core") ||
          (!CheckName.startswith("debug") &&
           Context.getChecksFilter().isCheckEnabled(Checker)))
        List.push_back(std::make_pair(CheckName, true));
    }
  }
  return List;
}

DiagnosticBuilder ClangTidyCheck::diag(SourceLocation Loc, StringRef Message,
                                       DiagnosticIDs::Level Level) {
  return Context->diag(CheckName, Loc, Message, Level);
}

void ClangTidyCheck::run(const ast_matchers::MatchFinder::MatchResult &Result) {
  Context->setSourceManager(Result.SourceManager);
  check(Result);
}

void ClangTidyCheck::setName(StringRef Name) {
  assert(CheckName.empty());
  CheckName = Name.str();
}

std::vector<std::string> getCheckNames(const ClangTidyOptions &Options) {
  SmallVector<ClangTidyError, 8> Errors;
  clang::tidy::ClangTidyContext Context(&Errors, Options);
  ClangTidyASTConsumerFactory Factory(Context);
  return Factory.getCheckNames();
}

void runClangTidy(const ClangTidyOptions &Options,
                  const tooling::CompilationDatabase &Compilations,
                  ArrayRef<std::string> Ranges,
                  SmallVectorImpl<ClangTidyError> *Errors) {
  // FIXME: Ranges are currently full files. Support selecting specific
  // (line-)ranges.
  ClangTool Tool(Compilations, Ranges);
  clang::tidy::ClangTidyContext Context(Errors, Options);
  ClangTidyDiagnosticConsumer DiagConsumer(Context);

  Tool.setDiagnosticConsumer(&DiagConsumer);

  class ActionFactory : public FrontendActionFactory {
  public:
    ActionFactory(ClangTidyASTConsumerFactory *ConsumerFactory)
        : ConsumerFactory(ConsumerFactory) {}
    FrontendAction *create() override { return new Action(ConsumerFactory); }

  private:
    class Action : public ASTFrontendAction {
    public:
      Action(ClangTidyASTConsumerFactory *Factory) : Factory(Factory) {}
      ASTConsumer *CreateASTConsumer(CompilerInstance &Compiler,
                                     StringRef File) override {
        return Factory->CreateASTConsumer(Compiler, File);
      }

    private:
      ClangTidyASTConsumerFactory *Factory;
    };

    ClangTidyASTConsumerFactory *ConsumerFactory;
  };

  Tool.run(new ActionFactory(new ClangTidyASTConsumerFactory(Context)));
}

void handleErrors(SmallVectorImpl<ClangTidyError> &Errors, bool Fix) {
  ErrorReporter Reporter(Fix);
  for (const ClangTidyError &Error : Errors) {
    Reporter.reportDiagnostic(Error.Message, DiagnosticsEngine::Warning,
                              &Error.Fix);
    for (const ClangTidyMessage &Note : Error.Notes)
      Reporter.reportDiagnostic(Note, DiagnosticsEngine::Note);
  }
  Reporter.Finish();
}

} // namespace tidy
} // namespace clang
