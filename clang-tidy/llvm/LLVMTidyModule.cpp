//===--- LLVMTidyModule.cpp - clang-tidy ----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "IncludeOrderCheck.h"
#include "NamespaceCommentCheck.h"

namespace clang {
namespace tidy {

class LLVMModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.addCheckFactory(
        "llvm-include-order", new ClangTidyCheckFactory<IncludeOrderCheck>());
    CheckFactories.addCheckFactory(
        "llvm-namespace-comment",
        new ClangTidyCheckFactory<NamespaceCommentCheck>());
  }
};

// Register the LLVMTidyModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<LLVMModule> X("llvm-module",
                                                  "Adds LLVM lint checks.");

// This anchor is used to force the linker to link in the generated object file
// and thus register the LLVMModule.
volatile int LLVMModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
