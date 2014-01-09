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

#include "llvm/ADT/SmallString.h"

namespace clang {
namespace tidy {

ClangTidyMessage::ClangTidyMessage(StringRef Message) : Message(Message) {}

ClangTidyMessage::ClangTidyMessage(StringRef Message,
                                   const SourceManager &Sources,
                                   SourceLocation Loc)
    : Message(Message) {
  FilePath = Sources.getFilename(Loc);
  FileOffset = Sources.getFileOffset(Loc);
}

ClangTidyError::ClangTidyError(const ClangTidyMessage &Message)
    : Message(Message) {}

DiagnosticBuilder ClangTidyContext::Diag(SourceLocation Loc,
                                         StringRef Message) {
  return DiagEngine->Report(
      Loc, DiagEngine->getCustomDiagID(DiagnosticsEngine::Warning, Message));
}

void ClangTidyContext::setDiagnosticsEngine(DiagnosticsEngine *Engine) {
  DiagEngine = Engine;
}

void ClangTidyContext::setSourceManager(SourceManager *SourceMgr) {
  DiagEngine->setSourceManager(SourceMgr);
}

/// \brief Store a \c ClangTidyError.
void ClangTidyContext::storeError(const ClangTidyError &Error) {
  Errors->push_back(Error);
}

ClangTidyDiagnosticConsumer::ClangTidyDiagnosticConsumer(ClangTidyContext &Ctx)
    : Context(Ctx) {
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
  Diags.reset(new DiagnosticsEngine(
      IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs), &*DiagOpts, this,
      /*ShouldOwnClient=*/false));
  Context.setDiagnosticsEngine(Diags.get());
}

void ClangTidyDiagnosticConsumer::HandleDiagnostic(
    DiagnosticsEngine::Level DiagLevel, const Diagnostic &Info) {
  // FIXME: Demultiplex diagnostics.
  // FIXME: Ensure that we don't get notes from user code related to errors
  // from non-user code.
  if (Diags->getSourceManager().isInSystemHeader(Info.getLocation()))
    return;
  if (DiagLevel != DiagnosticsEngine::Note) {
    Errors.push_back(ClangTidyError(getMessage(Info)));
  } else {
    assert(!Errors.empty() &&
           "A diagnostic note can only be appended to a message.");
    Errors.back().Notes.push_back(getMessage(Info));
  }
  addFixes(Info, Errors.back());
}

// Flushes the internal diagnostics buffer to the ClangTidyContext.
void ClangTidyDiagnosticConsumer::finish() {
  for (unsigned i = 0, e = Errors.size(); i != e; ++i) {
    Context.storeError(Errors[i]);
  }
  Errors.clear();
}

void ClangTidyDiagnosticConsumer::addFixes(const Diagnostic &Info,
                                           ClangTidyError &Error) {
  if (!Info.hasSourceManager())
    return;
  SourceManager &SourceMgr = Info.getSourceManager();
  tooling::Replacements Replacements;
  for (unsigned i = 0, e = Info.getNumFixItHints(); i != e; ++i) {
    Error.Fix.insert(tooling::Replacement(
        SourceMgr, Info.getFixItHint(i).RemoveRange.getBegin(), 0,
        Info.getFixItHint(i).CodeToInsert));
  }
}

ClangTidyMessage
ClangTidyDiagnosticConsumer::getMessage(const Diagnostic &Info) const {
  SmallString<100> Buf;
  Info.FormatDiagnostic(Buf);
  if (!Info.hasSourceManager()) {
    return ClangTidyMessage(Buf.str());
  }
  return ClangTidyMessage(Buf.str(), Info.getSourceManager(),
                          Info.getLocation());
}

} // namespace tidy
} // namespace clang
