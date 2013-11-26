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

#include "clang/Basic/SourceManager.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

cl::opt<std::string> From(cl::Positional, cl::desc("<from>"));

cl::opt<std::string> To(cl::Positional, cl::desc("<to>"));

namespace {
class RenameCallback : public MatchFinder::MatchCallback {
public:
  RenameCallback(Replacements *Replace) : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    const TypeLoc *Loc = Result.Nodes.getNodeAs<TypeLoc>("loc");
    assert(Loc != NULL);
    Replace->insert(Replacement(*Result.SourceManager, Loc, To));
  }

private:
  Replacements *Replace;
};
} // end anonymous namespace

int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv);
  RefactoringTool Tool(OptionsParser.getCompilations(),
                       OptionsParser.getSourcePathList());

  MatchFinder Finder;
  RenameCallback Callback(&Tool.getReplacements());
  Finder.addMatcher(
      loc(qualType(unless(elaboratedType()),
                   hasDeclaration(namedDecl(hasName(From))))).bind("loc"),
      &Callback);

  return Tool.runAndSave(newFrontendActionFactory(&Finder));
}

