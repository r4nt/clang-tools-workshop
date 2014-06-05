//===--- ClangTidyDiagnosticConsumer.h - clang-tidy -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CLANG_TIDY_DIAGNOSTIC_CONSUMER_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CLANG_TIDY_DIAGNOSTIC_CONSUMER_H

#include "ClangTidyOptions.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Tooling/Refactoring.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/Regex.h"

namespace clang {

class CompilerInstance;
namespace ast_matchers {
class MatchFinder;
}
namespace tooling {
class CompilationDatabase;
}

namespace tidy {

/// \brief A message from a clang-tidy check.
///
/// Note that this is independent of a \c SourceManager.
struct ClangTidyMessage {
  ClangTidyMessage(StringRef Message = "");
  ClangTidyMessage(StringRef Message, const SourceManager &Sources,
                   SourceLocation Loc);
  std::string Message;
  std::string FilePath;
  unsigned FileOffset;
};

/// \brief A detected error complete with information to display diagnostic and
/// automatic fix.
///
/// This is used as an intermediate format to transport Diagnostics without a
/// dependency on a SourceManager.
///
/// FIXME: Make Diagnostics flexible enough to support this directly.
struct ClangTidyError {
  enum Level {
    Warning = DiagnosticsEngine::Warning,
    Error = DiagnosticsEngine::Error
  };

  ClangTidyError(StringRef CheckName, Level DiagLevel);

  std::string CheckName;
  ClangTidyMessage Message;
  tooling::Replacements Fix;
  SmallVector<ClangTidyMessage, 1> Notes;

  Level DiagLevel;
};

/// \brief Filters checks by name.
class ChecksFilter {
public:
  /// \brief \p GlobList is a comma-separated list of globs (only '*'
  /// metacharacter is supported) with optional '-' prefix to denote exclusion.
  ChecksFilter(StringRef GlobList);

  /// \brief Returns \c true if the check with the specified \p Name should be
  /// enabled. The result is the last matching glob's Positive flag. If \p Name
  /// is not matched by any globs, the check is not enabled.
  bool isCheckEnabled(StringRef Name) { return isCheckEnabled(Name, false); }

private:
  bool isCheckEnabled(StringRef Name, bool Enabled);

  bool Positive;
  llvm::Regex Regex;
  std::unique_ptr<ChecksFilter> NextFilter;
};

/// \brief Contains displayed and ignored diagnostic counters for a ClangTidy
/// run.
struct ClangTidyStats {
  ClangTidyStats()
      : ErrorsDisplayed(0), ErrorsIgnoredCheckFilter(0), ErrorsIgnoredNOLINT(0),
        ErrorsIgnoredNonUserCode(0), ErrorsIgnoredLineFilter(0) {}

  unsigned ErrorsDisplayed;
  unsigned ErrorsIgnoredCheckFilter;
  unsigned ErrorsIgnoredNOLINT;
  unsigned ErrorsIgnoredNonUserCode;
  unsigned ErrorsIgnoredLineFilter;

  unsigned errorsIgnored() const {
    return ErrorsIgnoredNOLINT + ErrorsIgnoredCheckFilter +
           ErrorsIgnoredNonUserCode + ErrorsIgnoredLineFilter;
  }
};

/// \brief Every \c ClangTidyCheck reports errors through a \c DiagnosticEngine
/// provided by this context.
///
/// A \c ClangTidyCheck always has access to the active context to report
/// warnings like:
/// \code
/// Context->Diag(Loc, "Single-argument constructors must be explicit")
///     << FixItHint::CreateInsertion(Loc, "explicit ");
/// \endcode
class ClangTidyContext {
public:
  /// \brief Initializes \c ClangTidyContext instance.
  ///
  /// Takes ownership of the \c OptionsProvider.
  ClangTidyContext(ClangTidyOptionsProvider *OptionsProvider);

  /// \brief Report any errors detected using this method.
  ///
  /// This is still under heavy development and will likely change towards using
  /// tablegen'd diagnostic IDs.
  /// FIXME: Figure out a way to manage ID spaces.
  DiagnosticBuilder diag(StringRef CheckName, SourceLocation Loc,
                         StringRef Message,
                         DiagnosticIDs::Level Level = DiagnosticIDs::Warning);

  /// \brief Sets the \c SourceManager of the used \c DiagnosticsEngine.
  ///
  /// This is called from the \c ClangTidyCheck base class.
  void setSourceManager(SourceManager *SourceMgr);

  /// \brief Should be called when starting to process new translation unit.
  void setCurrentFile(StringRef File);

  /// \brief Returns the name of the clang-tidy check which produced this
  /// diagnostic ID.
  StringRef getCheckName(unsigned DiagnosticID) const;

  /// \brief Returns check filter for the \c CurrentFile.
  ChecksFilter &getChecksFilter();

  /// \brief Returns global options.
  const ClangTidyGlobalOptions &getGlobalOptions() const;

  /// \brief Returns options for \c CurrentFile.
  const ClangTidyOptions &getOptions() const;

  /// \brief Returns \c ClangTidyStats containing issued and ignored diagnostic
  /// counters.
  const ClangTidyStats &getStats() const { return Stats; }

  /// \brief Returns all collected errors.
  const std::vector<ClangTidyError> &getErrors() const { return Errors; }

  /// \brief Clears collected errors.
  void clearErrors() { Errors.clear(); }

private:
  // Calls setDiagnosticsEngine() and storeError().
  friend class ClangTidyDiagnosticConsumer;

  /// \brief Sets the \c DiagnosticsEngine so that Diagnostics can be generated
  /// correctly.
  void setDiagnosticsEngine(DiagnosticsEngine *Engine);

  /// \brief Store an \p Error.
  void storeError(const ClangTidyError &Error);

  std::vector<ClangTidyError> Errors;
  DiagnosticsEngine *DiagEngine;
  std::unique_ptr<ClangTidyOptionsProvider> OptionsProvider;

  std::string CurrentFile;
  std::unique_ptr<ChecksFilter> CheckFilter;

  ClangTidyStats Stats;

  llvm::DenseMap<unsigned, std::string> CheckNamesByDiagnosticID;
};

/// \brief A diagnostic consumer that turns each \c Diagnostic into a
/// \c SourceManager-independent \c ClangTidyError.
//
// FIXME: If we move away from unit-tests, this can be moved to a private
// implementation file.
class ClangTidyDiagnosticConsumer : public DiagnosticConsumer {
public:
  ClangTidyDiagnosticConsumer(ClangTidyContext &Ctx);

  // FIXME: The concept of converting between FixItHints and Replacements is
  // more generic and should be pulled out into a more useful Diagnostics
  // library.
  void HandleDiagnostic(DiagnosticsEngine::Level DiagLevel,
                        const Diagnostic &Info) override;

  /// \brief Sets \c HeaderFilter to the value configured for this file.
  void BeginSourceFile(const LangOptions &LangOpts,
                       const Preprocessor *PP) override;

  /// \brief Flushes the internal diagnostics buffer to the ClangTidyContext.
  void finish() override;

private:
  void finalizeLastError();

  /// \brief Updates \c LastErrorRelatesToUserCode and LastErrorPassesLineFilter
  /// according to the diagnostic \p Location.
  void checkFilters(SourceLocation Location);
  bool passesLineFilter(StringRef FileName, unsigned LineNumber) const;

  ClangTidyContext &Context;
  std::unique_ptr<DiagnosticsEngine> Diags;
  SmallVector<ClangTidyError, 8> Errors;
  std::unique_ptr<llvm::Regex> HeaderFilter;
  bool LastErrorRelatesToUserCode;
  bool LastErrorPassesLineFilter;
};

} // end namespace tidy
} // end namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CLANG_TIDY_DIAGNOSTIC_CONSUMER_H
