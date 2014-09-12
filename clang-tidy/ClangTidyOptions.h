//===--- ClangTidyOptions.h - clang-tidy ------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CLANG_TIDY_OPTIONS_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CLANG_TIDY_OPTIONS_H

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorOr.h"
#include <map>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace clang {
namespace tidy {

/// \brief Contains a list of line ranges in a single file.
struct FileFilter {
  /// \brief File name.
  std::string Name;

  /// \brief LineRange is a pair<start, end> (inclusive).
  typedef std::pair<unsigned, unsigned> LineRange;

  /// \brief A list of line ranges in this file, for which we show warnings.
  std::vector<LineRange> LineRanges;
};

/// \brief Global options. These options are neither stored nor read from
/// configuration files.
struct ClangTidyGlobalOptions {
  /// \brief Output warnings from certain line ranges of certain files only.
  /// If empty, no warnings will be filtered.
  std::vector<FileFilter> LineFilter;
};

/// \brief Contains options for clang-tidy. These options may be read from
/// configuration files, and may be different for different translation units.
struct ClangTidyOptions {
  /// \brief These options are used for all settings that haven't been
  /// overridden by the \c OptionsProvider.
  ///
  /// Allow no checks and no headers by default.
  static ClangTidyOptions getDefaults() {
    ClangTidyOptions Options;
    Options.Checks = "";
    Options.HeaderFilterRegex = "";
    Options.AnalyzeTemporaryDtors = false;
    return Options;
  }

  /// \brief Creates a new \c ClangTidyOptions instance combined from all fields
  /// of this instance overridden by the fields of \p Other that have a value.
  ClangTidyOptions mergeWith(const ClangTidyOptions &Other) const;

  /// \brief Checks filter.
  llvm::Optional<std::string> Checks;

  /// \brief Output warnings from headers matching this filter. Warnings from
  /// main files will always be displayed.
  llvm::Optional<std::string> HeaderFilterRegex;

  /// \brief Turns on temporary destructor-based analysis.
  llvm::Optional<bool> AnalyzeTemporaryDtors;

  typedef std::pair<std::string, std::string> StringPair;
  typedef std::map<std::string, std::string> OptionMap;

  /// \brief Key-value mapping used to store check-specific options.
  OptionMap CheckOptions;
};

/// \brief Abstract interface for retrieving various ClangTidy options.
class ClangTidyOptionsProvider {
public:
  virtual ~ClangTidyOptionsProvider() {}

  /// \brief Returns global options, which are independent of the file.
  virtual const ClangTidyGlobalOptions &getGlobalOptions() = 0;

  /// \brief Returns options applying to a specific translation unit with the
  /// specified \p FileName.
  virtual const ClangTidyOptions &getOptions(llvm::StringRef FileName) = 0;
};

/// \brief Implementation of the \c ClangTidyOptionsProvider interface, which
/// returns the same options for all files.
class DefaultOptionsProvider : public ClangTidyOptionsProvider {
public:
  DefaultOptionsProvider(const ClangTidyGlobalOptions &GlobalOptions,
                         const ClangTidyOptions &Options)
      : GlobalOptions(GlobalOptions), DefaultOptions(Options) {}
  const ClangTidyGlobalOptions &getGlobalOptions() override {
    return GlobalOptions;
  }
  const ClangTidyOptions &getOptions(llvm::StringRef /*FileName*/) override {
    return DefaultOptions;
  }

private:
  ClangTidyGlobalOptions GlobalOptions;
  ClangTidyOptions DefaultOptions;
};

/// \brief Implementation of the \c ClangTidyOptionsProvider interface, which
/// tries to find a .clang-tidy file in the closest parent directory of each
/// file.
class FileOptionsProvider : public DefaultOptionsProvider {
public:
  /// \brief Initializes the \c FileOptionsProvider instance.
  ///
  /// \param GlobalOptions are just stored and returned to the caller of
  /// \c getGlobalOptions.
  ///
  /// \param FallbackOptions are used in case there's no corresponding
  /// .clang-tidy file.
  ///
  /// If any of the \param OverrideOptions fields are set, they will override
  /// whatever options are read from the configuration file.
  FileOptionsProvider(const ClangTidyGlobalOptions &GlobalOptions,
                      const ClangTidyOptions &FallbackOptions,
                      const ClangTidyOptions &OverrideOptions);
  const ClangTidyOptions &getOptions(llvm::StringRef FileName) override;

private:
  /// \brief Try to read configuration file from \p Directory. If \p Directory
  /// is empty, use the fallback value.
  llvm::ErrorOr<ClangTidyOptions> TryReadConfigFile(llvm::StringRef Directory);

  llvm::StringMap<ClangTidyOptions> CachedOptions;
  ClangTidyOptions OverrideOptions;
};

/// \brief Parses LineFilter from JSON and stores it to the \p Options.
std::error_code parseLineFilter(llvm::StringRef LineFilter,
                                ClangTidyGlobalOptions &Options);

/// \brief Parses configuration from JSON and stores it to the \p Options.
std::error_code parseConfiguration(llvm::StringRef Config,
                                   ClangTidyOptions &Options);

/// \brief Serializes configuration to a YAML-encoded string.
std::string configurationAsText(const ClangTidyOptions &Options);

} // end namespace tidy
} // end namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CLANG_TIDY_OPTIONS_H
