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

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/system_error.h"
#include <string>
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
  /// \brief Allow all checks and no headers by default.
  ClangTidyOptions() : Checks("*"), AnalyzeTemporaryDtors(false) {}

  /// \brief Checks filter.
  std::string Checks;

  /// \brief Output warnings from headers matching this filter. Warnings from
  /// main files will always be displayed.
  std::string HeaderFilterRegex;

  /// \brief Turns on temporary destructor-based analysis.
  bool AnalyzeTemporaryDtors;
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
  const ClangTidyOptions &getOptions(llvm::StringRef) override {
    return DefaultOptions;
  }

private:
  ClangTidyGlobalOptions GlobalOptions;
  ClangTidyOptions DefaultOptions;
};

/// \brief Parses LineFilter from JSON and stores it to the \p Options.
std::error_code parseLineFilter(const std::string &LineFilter,
                                clang::tidy::ClangTidyGlobalOptions &Options);

/// \brief Parses configuration from JSON and stores it to the \p Options.
std::error_code parseConfiguration(const std::string &Config,
                                   clang::tidy::ClangTidyOptions &Options);

} // end namespace tidy
} // end namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CLANG_TIDY_OPTIONS_H
