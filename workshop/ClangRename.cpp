//===---- tools/extra/workshop/ClangRename.cpp - Rename tool --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  A C++ rename tool.
//
//===----------------------------------------------------------------------===//

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

namespace {
class RenameCallback : public MatchFinder::MatchCallback {
public:
  RenameCallback(Replacements *Replace) : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result) {}

private:
  Replacements *Replace;
};
} // end anonymous namespace

cl::opt<std::string> From(
  cl::Positional,
  cl::desc("<from>"));

cl::opt<std::string> To(
  cl::Positional,
  cl::desc("<to>"));

int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv);
  RefactoringTool Tool(OptionsParser.getCompilations(),
                       OptionsParser.getSourcePathList());

  return Tool.runAndSave(NULL);
}

