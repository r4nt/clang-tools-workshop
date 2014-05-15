//===--- tools/extra/clang-tidy/ClangTidyDiagnosticConsumer.cpp ----------=== //
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
///  \file This file implements ClangTidyDiagnosticConsumer, ClangTidyMessage,
///  ClangTidyContext and ClangTidyError classes.
///
///  This tool uses the Clang Tooling infrastructure, see
///    http://clang.llvm.org/docs/HowToSetupToolingForLLVM.html
///  for details on setting it up with LLVM source tree.
///
//===----------------------------------------------------------------------===//

#include "ClangTidyDiagnosticConsumer.h"
#include "ClangTidyOptions.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Frontend/DiagnosticRenderer.h"
#include "llvm/ADT/SmallString.h"
#include <set>
#include <tuple>
using namespace clang;
using namespace tidy;

namespace {
class ClangTidyDiagnosticRenderer : public DiagnosticRenderer {
public:
  ClangTidyDiagnosticRenderer(const LangOptions &LangOpts,
                              DiagnosticOptions *DiagOpts,
                              ClangTidyError &Error)
      : DiagnosticRenderer(LangOpts, DiagOpts), Error(Error) {}

protected:
  void emitDiagnosticMessage(SourceLocation Loc, PresumedLoc PLoc,
                             DiagnosticsEngine::Level Level, StringRef Message,
                             ArrayRef<CharSourceRange> Ranges,
                             const SourceManager *SM,
                             DiagOrStoredDiag Info) override {
    ClangTidyMessage TidyMessage = Loc.isValid()
                                       ? ClangTidyMessage(Message, *SM, Loc)
                                       : ClangTidyMessage(Message);
    if (Level == DiagnosticsEngine::Note) {
      Error.Notes.push_back(TidyMessage);
      return;
    }
    assert(Error.Message.Message.empty() &&
           "Overwriting a diagnostic message");
    Error.Message = TidyMessage;
  }

  void emitDiagnosticLoc(SourceLocation Loc, PresumedLoc PLoc,
                         DiagnosticsEngine::Level Level,
                         ArrayRef<CharSourceRange> Ranges,
                         const SourceManager &SM) override {}

  void emitBasicNote(StringRef Message) override {
    Error.Notes.push_back(ClangTidyMessage(Message));
  }

  void emitCodeContext(SourceLocation Loc, DiagnosticsEngine::Level Level,
                       SmallVectorImpl<CharSourceRange> &Ranges,
                       ArrayRef<FixItHint> Hints,
                       const SourceManager &SM) override {
    assert(Loc.isValid());
    for (const auto &FixIt : Hints) {
      CharSourceRange Range = FixIt.RemoveRange;
      assert(Range.getBegin().isValid() && Range.getEnd().isValid() &&
             "Invalid range in the fix-it hint.");
      assert(Range.getBegin().isFileID() && Range.getEnd().isFileID() &&
             "Only file locations supported in fix-it hints.");

      Error.Fix.insert(tooling::Replacement(SM, Range, FixIt.CodeToInsert));
    }
  }

  void emitIncludeLocation(SourceLocation Loc, PresumedLoc PLoc,
                           const SourceManager &SM) override {}

  void emitImportLocation(SourceLocation Loc, PresumedLoc PLoc,
                          StringRef ModuleName,
                          const SourceManager &SM) override {}

  void emitBuildingModuleLocation(SourceLocation Loc, PresumedLoc PLoc,
                                  StringRef ModuleName,
                                  const SourceManager &SM) override {}

  void endDiagnostic(DiagOrStoredDiag D,
                     DiagnosticsEngine::Level Level) override {
    assert(!Error.Message.Message.empty() && "Message has not been set");
  }

private:
  ClangTidyError &Error;
};
} // end anonymous namespace

ClangTidyMessage::ClangTidyMessage(StringRef Message) : Message(Message) {}

ClangTidyMessage::ClangTidyMessage(StringRef Message,
                                   const SourceManager &Sources,
                                   SourceLocation Loc)
    : Message(Message) {
  assert(Loc.isValid() && Loc.isFileID());
  FilePath = Sources.getFilename(Loc);
  FileOffset = Sources.getFileOffset(Loc);
}

ClangTidyError::ClangTidyError(StringRef CheckName) : CheckName(CheckName) {}

// Returns true if GlobList starts with the negative indicator ('-'), removes it
// from the GlobList.
static bool ConsumeNegativeIndicator(StringRef &GlobList) {
  if (GlobList.startswith("-")) {
    GlobList = GlobList.substr(1);
    return true;
  }
  return false;
}
// Converts first glob from the comma-separated list of globs to Regex and
// removes it and the trailing comma from the GlobList.
static llvm::Regex ConsumeGlob(StringRef &GlobList) {
  StringRef Glob = GlobList.substr(0, GlobList.find(','));
  GlobList = GlobList.substr(Glob.size() + 1);
  llvm::SmallString<128> RegexText("^");
  StringRef MetaChars("()^$|*+?.[]\\{}");
  for (char C : Glob) {
    if (C == '*')
      RegexText.push_back('.');
    else if (MetaChars.find(C) != StringRef::npos)
      RegexText.push_back('\\');
    RegexText.push_back(C);
  }
  RegexText.push_back('$');
  return llvm::Regex(RegexText);
}

ChecksFilter::ChecksFilter(StringRef GlobList)
    : Positive(!ConsumeNegativeIndicator(GlobList)),
      Regex(ConsumeGlob(GlobList)),
      NextFilter(GlobList.empty() ? nullptr : new ChecksFilter(GlobList)) {}

bool ChecksFilter::isCheckEnabled(StringRef Name, bool Enabled) {
  if (Regex.match(Name))
    Enabled = Positive;

  if (NextFilter)
    Enabled = NextFilter->isCheckEnabled(Name, Enabled);
  return Enabled;
}

ClangTidyContext::ClangTidyContext(const ClangTidyOptions &Options)
    : DiagEngine(nullptr), Options(Options), Filter(Options.Checks) {}

DiagnosticBuilder ClangTidyContext::diag(
    StringRef CheckName, SourceLocation Loc, StringRef Description,
    DiagnosticIDs::Level Level /* = DiagnosticIDs::Warning*/) {
  assert(Loc.isValid());
  bool Invalid;
  const char *CharacterData =
      DiagEngine->getSourceManager().getCharacterData(Loc, &Invalid);
  if (!Invalid) {
    const char *P = CharacterData;
    while (*P != '\0' && *P != '\r' && *P != '\n')
      ++P;
    StringRef RestOfLine(CharacterData, P - CharacterData + 1);
    // FIXME: Handle /\bNOLINT\b(\([^)]*\))?/ as cpplint.py does.
    if (RestOfLine.find("NOLINT") != StringRef::npos) {
      Level = DiagnosticIDs::Ignored;
      ++Stats.ErrorsIgnoredNOLINT;
    }
  }
  unsigned ID = DiagEngine->getDiagnosticIDs()->getCustomDiagID(
      Level, (Description + " [" + CheckName + "]").str());
  if (CheckNamesByDiagnosticID.count(ID) == 0)
    CheckNamesByDiagnosticID.insert(std::make_pair(ID, CheckName.str()));
  return DiagEngine->Report(Loc, ID);
}

void ClangTidyContext::setDiagnosticsEngine(DiagnosticsEngine *Engine) {
  DiagEngine = Engine;
}

void ClangTidyContext::setSourceManager(SourceManager *SourceMgr) {
  DiagEngine->setSourceManager(SourceMgr);
}

/// \brief Store a \c ClangTidyError.
void ClangTidyContext::storeError(const ClangTidyError &Error) {
  Errors.push_back(Error);
}

StringRef ClangTidyContext::getCheckName(unsigned DiagnosticID) const {
  llvm::DenseMap<unsigned, std::string>::const_iterator I =
      CheckNamesByDiagnosticID.find(DiagnosticID);
  if (I != CheckNamesByDiagnosticID.end())
    return I->second;
  return "";
}

ClangTidyDiagnosticConsumer::ClangTidyDiagnosticConsumer(ClangTidyContext &Ctx)
    : Context(Ctx), HeaderFilter(Ctx.getOptions().HeaderFilterRegex),
      LastErrorRelatesToUserCode(false) {
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
  Diags.reset(new DiagnosticsEngine(
      IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs), &*DiagOpts, this,
      /*ShouldOwnClient=*/false));
  Context.setDiagnosticsEngine(Diags.get());
}

void ClangTidyDiagnosticConsumer::finalizeLastError() {
  if (!Errors.empty()) {
    ClangTidyError &Error = Errors.back();
    if (!Context.getChecksFilter().isCheckEnabled(Error.CheckName)) {
      ++Context.Stats.ErrorsIgnoredCheckFilter;
      Errors.pop_back();
    } else if (!LastErrorRelatesToUserCode) {
      ++Context.Stats.ErrorsIgnoredNonUserCode;
      Errors.pop_back();
    } else {
      ++Context.Stats.ErrorsDisplayed;
    }
  }
  LastErrorRelatesToUserCode = false;
}

void ClangTidyDiagnosticConsumer::HandleDiagnostic(
    DiagnosticsEngine::Level DiagLevel, const Diagnostic &Info) {
  if (DiagLevel == DiagnosticsEngine::Note) {
    assert(!Errors.empty() &&
           "A diagnostic note can only be appended to a message.");
  } else {
    // FIXME: Pass all errors here regardless of filters and non-user code.
    finalizeLastError();
    StringRef WarningOption =
        Context.DiagEngine->getDiagnosticIDs()->getWarningOptionForDiag(
            Info.getID());
    std::string CheckName = !WarningOption.empty()
                                ? ("clang-diagnostic-" + WarningOption).str()
                                : Context.getCheckName(Info.getID()).str();

    Errors.push_back(ClangTidyError(CheckName));
  }

  // FIXME: Provide correct LangOptions for each file.
  LangOptions LangOpts;
  ClangTidyDiagnosticRenderer Converter(
      LangOpts, &Context.DiagEngine->getDiagnosticOptions(), Errors.back());
  SmallString<100> Message;
  Info.FormatDiagnostic(Message);
  SourceManager *Sources = nullptr;
  if (Info.hasSourceManager())
    Sources = &Info.getSourceManager();
  Converter.emitDiagnostic(
      Info.getLocation(), DiagLevel, Message, Info.getRanges(),
      llvm::makeArrayRef(Info.getFixItHints(), Info.getNumFixItHints()),
      Sources);

  // Let argument parsing-related warnings through.
  if (relatesToUserCode(Info.getLocation())) {
    LastErrorRelatesToUserCode = true;
  }
}

bool ClangTidyDiagnosticConsumer::relatesToUserCode(SourceLocation Location) {
  // Invalid location may mean a diagnostic in a command line, don't skip these.
  if (!Location.isValid())
    return true;

  const SourceManager &Sources = Diags->getSourceManager();
  if (Sources.isInSystemHeader(Location))
    return false;

  // FIXME: We start with a conservative approach here, but the actual type of
  // location needed depends on the check (in particular, where this check wants
  // to apply fixes).
  FileID FID = Sources.getDecomposedExpansionLoc(Location).first;
  if (FID == Sources.getMainFileID())
    return true;

  const FileEntry *File = Sources.getFileEntryForID(FID);
  // -DMACRO definitions on the command line have locations in a virtual buffer
  // that doesn't have a FileEntry. Don't skip these as well.
  return !File || HeaderFilter.match(File->getName());
}

namespace {
struct LessClangTidyError {
  bool operator()(const ClangTidyError *LHS, const ClangTidyError *RHS) const {
    const ClangTidyMessage &M1 = LHS->Message;
    const ClangTidyMessage &M2 = RHS->Message;

    return std::tie(M1.FilePath, M1.FileOffset, M1.Message) <
           std::tie(M2.FilePath, M2.FileOffset, M2.Message);
  }
};
} // end anonymous namespace

// Flushes the internal diagnostics buffer to the ClangTidyContext.
void ClangTidyDiagnosticConsumer::finish() {
  finalizeLastError();
  std::set<const ClangTidyError*, LessClangTidyError> UniqueErrors;
  for (const ClangTidyError &Error : Errors)
    UniqueErrors.insert(&Error);

  for (const ClangTidyError *Error : UniqueErrors)
    Context.storeError(*Error);
  Errors.clear();
}
