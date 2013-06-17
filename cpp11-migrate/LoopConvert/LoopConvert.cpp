//===-- LoopConvert/LoopConvert.cpp - C++11 for-loop migration --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file provides the implementation of the LoopConvertTransform
/// class.
///
//===----------------------------------------------------------------------===//

#include "LoopConvert.h"
#include "LoopActions.h"
#include "LoopMatchers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"

using clang::ast_matchers::MatchFinder;
using namespace clang::tooling;
using namespace clang;

int LoopConvertTransform::apply(const FileOverrides &InputStates,
                                const CompilationDatabase &Database,
                                const std::vector<std::string> &SourcePaths,
                                FileOverrides &ResultStates) {
  RefactoringTool LoopTool(Database, SourcePaths);

  StmtAncestorASTVisitor ParentFinder;
  StmtGeneratedVarNameMap GeneratedDecls;
  ReplacedVarsMap ReplacedVars;
  unsigned AcceptedChanges = 0;
  unsigned DeferredChanges = 0;
  unsigned RejectedChanges = 0;

  MatchFinder Finder;
  LoopFixer ArrayLoopFixer(&ParentFinder, &LoopTool.getReplacements(),
                           &GeneratedDecls, &ReplacedVars, &AcceptedChanges,
                           &DeferredChanges, &RejectedChanges,
                           Options().MaxRiskLevel, LFK_Array);
  Finder.addMatcher(makeArrayLoopMatcher(), &ArrayLoopFixer);
  LoopFixer IteratorLoopFixer(&ParentFinder, &LoopTool.getReplacements(),
                              &GeneratedDecls, &ReplacedVars,
                              &AcceptedChanges, &DeferredChanges,
                              &RejectedChanges,
                              Options().MaxRiskLevel, LFK_Iterator);
  Finder.addMatcher(makeIteratorLoopMatcher(), &IteratorLoopFixer);
  LoopFixer PseudoarrrayLoopFixer(&ParentFinder, &LoopTool.getReplacements(),
                                  &GeneratedDecls, &ReplacedVars,
                                  &AcceptedChanges, &DeferredChanges,
                                  &RejectedChanges,
                                  Options().MaxRiskLevel, LFK_PseudoArray);
  Finder.addMatcher(makePseudoArrayLoopMatcher(), &PseudoarrrayLoopFixer);

  setOverrides(InputStates);

  if (int result = LoopTool.run(createActionFactory(Finder))) {
    llvm::errs() << "Error encountered during translation.\n";
    return result;
  }

  RewriterContainer Rewrite(LoopTool.getFiles(), InputStates);

  // FIXME: Do something if some replacements didn't get applied?
  LoopTool.applyAllReplacements(Rewrite.getRewriter());

  collectResults(Rewrite.getRewriter(), InputStates, ResultStates);

  setAcceptedChanges(AcceptedChanges);
  setRejectedChanges(RejectedChanges);
  setDeferredChanges(DeferredChanges);

  return 0;
}
