//===-- cpp11-migrate/Cpp11Migrate.cpp - Main file C++11 migration tool ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file implements the C++11 feature migration tool main function
/// and transformation framework.
///
/// Usage:
/// cpp11-migrate [-p <build-path>] <file1> <file2> ... [-- [compiler-options]]
///
/// Where <build-path> is a CMake build directory containing a file named
/// compile_commands.json which provides compiler options for building each
/// sourc file. If <build-path> is not provided the compile_commands.json file
/// is searched for through all parent directories.
///
/// Alternatively, one can provide compile options to be applied to every source
/// file after the optional '--'.
///
/// <file1>... specify the paths of files in the CMake source tree, with the
/// same requirements as other tools built on LibTooling.
///
//===----------------------------------------------------------------------===//

#include "Transforms.h"
#include "Transform.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

namespace cl = llvm::cl;
using namespace clang::tooling;

static cl::opt<RiskLevel> MaxRiskLevel(
    "risk", cl::desc("Select a maximum risk level:"),
    cl::values(clEnumValN(RL_Safe, "safe", "Only safe transformations"),
               clEnumValN(RL_Reasonable, "reasonable",
                          "Enable transformations that might change "
                          "semantics (default)"),
               clEnumValN(RL_Risky, "risky",
                          "Enable transformations that are likely to "
                          "change semantics"),
               clEnumValEnd),
    cl::init(RL_Reasonable));

int main(int argc, const char **argv) {
  Transforms TransformManager;

  TransformManager.createTransformOpts();

  // This causes options to be parsed.
  CommonOptionsParser OptionsParser(argc, argv);

  TransformManager.createSelectedTransforms();

  if (TransformManager.begin() == TransformManager.end()) {
    llvm::errs() << "No selected transforms\n";
    return 1;
  }

  // Initial syntax check.
  ClangTool SyntaxTool(OptionsParser.getCompilations(),
                       OptionsParser.getSourcePathList());

  // First, let's check to make sure there were no errors.
  if (SyntaxTool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>()) !=
      0) {
    return 1;
  }

  unsigned int NumFiles = OptionsParser.getSourcePathList().size();

  FileContentsByPath FileStates1, FileStates2,
      *InputFileStates = &FileStates1, *OutputFileStates = &FileStates2;
  FileStates1.reserve(NumFiles);
  FileStates2.reserve(NumFiles);

  // Apply transforms.
  for (Transforms::const_iterator I = TransformManager.begin(),
                                  E = TransformManager.end();
       I != E; ++I) {
    if ((*I)->apply(*InputFileStates, MaxRiskLevel,
                    OptionsParser.getCompilations(),
                    OptionsParser.getSourcePathList(), *OutputFileStates) !=
        0) {
      // FIXME: Improve ClangTool to not abort if just one file fails.
      return 1;
    }
    std::swap(InputFileStates, OutputFileStates);
    OutputFileStates->clear();
  }

  // Final state of files is pointed at by InputFileStates.

  // Final Syntax check.
  ClangTool EndSyntaxTool(OptionsParser.getCompilations(),
                          OptionsParser.getSourcePathList());
  for (FileContentsByPath::const_iterator I = InputFileStates->begin(),
                                          E = InputFileStates->end();
       I != E; ++I) {
    EndSyntaxTool.mapVirtualFile(I->first, I->second);
  }

  if (EndSyntaxTool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>()) !=
      0) {
    return 1;
  }

  // Syntax check passed, write results to file.
  for (FileContentsByPath::const_iterator I = InputFileStates->begin(),
                                          E = InputFileStates->end();
       I != E; ++I) {
    std::string ErrorInfo;
    llvm::raw_fd_ostream FileStream(I->first.c_str(), ErrorInfo,
                                    llvm::raw_fd_ostream::F_Binary);
    FileStream << I->second;
  }

  return 0;
}
