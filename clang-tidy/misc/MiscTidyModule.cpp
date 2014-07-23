//===--- MiscTidyModule.cpp - clang-tidy ----------------------------------===//
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
#include "ArgumentCommentCheck.h"
#include "BoolPointerImplicitConversion.h"
#include "RedundantSmartptrGet.h"
#include "SwappedArgumentsCheck.h"
#include "UnusedRAII.h"
#include "UseOverride.h"

namespace clang {
namespace tidy {

class MiscModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.addCheckFactory(
        "misc-argument-comment",
        new ClangTidyCheckFactory<ArgumentCommentCheck>());
    CheckFactories.addCheckFactory(
        "misc-bool-pointer-implicit-conversion",
        new ClangTidyCheckFactory<BoolPointerImplicitConversion>());
    CheckFactories.addCheckFactory(
        "misc-redundant-smartptr-get",
        new ClangTidyCheckFactory<RedundantSmartptrGet>());
    CheckFactories.addCheckFactory(
        "misc-swapped-arguments",
        new ClangTidyCheckFactory<SwappedArgumentsCheck>());
    CheckFactories.addCheckFactory(
        "misc-unused-raii",
        new ClangTidyCheckFactory<UnusedRAIICheck>());
    CheckFactories.addCheckFactory(
        "misc-use-override",
        new ClangTidyCheckFactory<UseOverride>());
  }
};

// Register the MiscTidyModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<MiscModule>
X("misc-module", "Adds miscellaneous lint checks.");

// This anchor is used to force the linker to link in the generated object file
// and thus register the MiscModule.
volatile int MiscModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
