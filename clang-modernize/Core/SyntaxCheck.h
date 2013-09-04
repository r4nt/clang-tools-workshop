//===-- Core/SyntaxCheck.h --------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file exposes functionaliy for doing a syntax-only check on
/// files with overridden contents.
///
//===----------------------------------------------------------------------===//

#ifndef CLANG_MODERNIZE_SYNTAX_CHECK_H
#define CLANG_MODERNIZE_SYNTAX_CHECK_H

#include <string>
#include <vector>

// Forward Declarations
namespace clang {
namespace tooling {
class CompilationDatabase;
} // namespace tooling
} // namespace clang

class FileOverrides;

/// \brief Perform a syntax-only check over all files in \c SourcePaths using
/// options provided by \c Database using file contents from \c Overrides if
/// available.
extern bool doSyntaxCheck(const clang::tooling::CompilationDatabase &Database,
                          const std::vector<std::string> &SourcePaths,
                          const FileOverrides &Overrides);

#endif // CLANG_MODERNIZE_SYNTAX_CHECK_H
