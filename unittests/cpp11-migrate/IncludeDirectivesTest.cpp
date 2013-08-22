//===- cpp11-migrate/IncludeDirectivesTest.cpp ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Core/IncludeDirectives.h"
#include "gtest/gtest.h"
#include "VirtualFileHelper.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "llvm/Support/Path.h"

using namespace llvm;
using namespace clang;

/// \brief A convenience method around \c tooling::runToolOnCodeWithArgs() that
/// adds the current directory to the include search paths.
static void applyActionOnCode(FrontendAction *ToolAction, StringRef Code) {
  SmallString<128> CurrentDir;
  ASSERT_FALSE(llvm::sys::fs::current_path(CurrentDir));

  // Add the current directory to the header search paths so angled includes can
  // find them.
  std::vector<std::string> Args;
  Args.push_back("-I");
  Args.push_back(CurrentDir.str().str());

  // mapVirtualFile() needs absolute path for the input file as well.
  SmallString<128> InputFile(CurrentDir);
  sys::path::append(InputFile, "input.cc");

  ASSERT_TRUE(
      tooling::runToolOnCodeWithArgs(ToolAction, Code, Args, InputFile.str()));
}

namespace {
class TestAddIncludeAction : public PreprocessOnlyAction {
public:
  TestAddIncludeAction(StringRef Include, tooling::Replacements &Replaces,
                       const char *HeaderToModify = 0)
      : Include(Include), Replaces(Replaces), HeaderToModify(HeaderToModify) {
    // some headers that the tests can include
    mapVirtualHeader("foo-inner.h", "#pragma once\n");
    mapVirtualHeader("foo.h", "#pragma once\n"
                              "#include <virtual/foo-inner.h>\n");
    mapVirtualHeader("bar-inner.h", "#pragma once\n");
    mapVirtualHeader("bar.h", "#pragma once\n"
                              "#include <virtual/bar-inner.h>\n");
    mapVirtualHeader("xmacro.def", "X(Val1)\n"
                                   "X(Val2)\n"
                                   "X(Val3)\n");
  }

  /// \brief Make \p FileName an absolute path.
  ///
  /// Header files are mapped in the 'virtual/' subdirectory of the current
  /// working directory. The current working directory is used because it's
  /// important to map files with absolute paths.
  ///
  /// When used in conjunction with \c applyActionOnCode() (which adds the
  /// current working directory to the header search paths) it is possible to
  /// refer to the headers by using 'virtual/\<FileName\>'.
  std::string makeHeaderFileName(StringRef FileName) const {
    SmallString<128> Path;
    llvm::error_code EC = llvm::sys::fs::current_path(Path);
    assert(!EC);
    (void)EC;

    sys::path::append(Path, "virtual");
    sys::path::append(Path, FileName);
    return Path.str().str();
  }

  /// \brief Map additional header files.
  ///
  /// \sa makeHeaderFileName()
  void mapVirtualHeader(StringRef FileName, StringRef Content) {
    VFHelper.mapFile(makeHeaderFileName(FileName), Content);
  }

private:
  virtual bool BeginSourceFileAction(CompilerInstance &CI,
                                     StringRef FileName) LLVM_OVERRIDE {
    if (!PreprocessOnlyAction::BeginSourceFileAction(CI, FileName))
      return false;
    VFHelper.mapVirtualFiles(CI.getSourceManager());

    FileToModify =
        HeaderToModify ? makeHeaderFileName(HeaderToModify) : FileName.str();

    FileIncludes.reset(new IncludeDirectives(CI));
    return true;
  }

  virtual void EndSourceFileAction() LLVM_OVERRIDE {
    const tooling::Replacement &Replace =
        FileIncludes->addAngledInclude(FileToModify, Include);
    if (Replace.isApplicable())
      Replaces.insert(Replace);
  }

  StringRef Include;
  VirtualFileHelper VFHelper;
  tooling::Replacements &Replaces;
  OwningPtr<IncludeDirectives> FileIncludes;
  std::string FileToModify;
  // if non-null, add the include directives in this file instead of the main
  // file.
  const char *HeaderToModify;
};

std::string addIncludeInCode(StringRef Include, StringRef Code) {
  tooling::Replacements Replaces;

  applyActionOnCode(new TestAddIncludeAction(Include, Replaces), Code);

  if (::testing::Test::HasFailure())
    return "<<unexpected error from applyActionOnCode()>>";

  return tooling::applyAllReplacements(Code, Replaces);
}
} // end anonymous namespace

TEST(IncludeDirectivesTest2, endOfLinesVariants) {
  EXPECT_EQ("#include <virtual/foo.h>\n"
            "#include <bar>\n",
            addIncludeInCode("bar", "#include <virtual/foo.h>\n"));
  EXPECT_EQ("#include <virtual/foo.h>\r\n"
            "#include <bar>\r\n",
            addIncludeInCode("bar", "#include <virtual/foo.h>\r\n"));
  EXPECT_EQ("#include <virtual/foo.h>\r"
            "#include <bar>\r",
            addIncludeInCode("bar", "#include <virtual/foo.h>\r"));
}

TEST(IncludeDirectivesTest, ppToken) {
  EXPECT_EQ("#define FOO <virtual/foo.h>\n"
            "#include FOO\n"
            "#include <bar>\n"
            "int i;\n",
            addIncludeInCode("bar", "#define FOO <virtual/foo.h>\n"
                                    "#include FOO\n"
                                    "int i;\n"));
}

TEST(IncludeDirectivesTest, noFileHeader) {
  EXPECT_EQ("#include <bar>\n"
            "\n"
            "int foo;\n",
            addIncludeInCode("bar", "int foo;\n"));
}

TEST(IncludeDirectivesTest, commentBeforeTopMostCode) {
  EXPECT_EQ("#include <bar>\n"
            "\n"
            "// Foo\n"
            "int foo;\n",
            addIncludeInCode("bar", "// Foo\n"
                                    "int foo;\n"));
}

TEST(IncludeDirectivesTest, multiLineComment) {
  EXPECT_EQ("#include <virtual/foo.h> /* \n */\n"
            "#include <bar>\n",
            addIncludeInCode("bar", "#include <virtual/foo.h> /* \n */\n"));
  EXPECT_EQ("#include <virtual/foo.h> /* \n */"
            "\n#include <bar>",
            addIncludeInCode("bar", "#include <virtual/foo.h> /* \n */"));
}

TEST(IncludeDirectivesTest, multilineCommentWithTrailingSpace) {
  EXPECT_EQ("#include <virtual/foo.h> /*\n*/ \n"
            "#include <bar>\n",
            addIncludeInCode("bar", "#include <virtual/foo.h> /*\n*/ \n"));
  EXPECT_EQ("#include <virtual/foo.h> /*\n*/ "
            "\n#include <bar>",
            addIncludeInCode("bar", "#include <virtual/foo.h> /*\n*/ "));
}

TEST(IncludeDirectivesTest, fileHeaders) {
  EXPECT_EQ("// this is a header\n"
            "// some license stuff here\n"
            "\n"
            "#include <bar>\n"
            "\n"
            "/// \\brief Foo\n"
            "int foo;\n",
            addIncludeInCode("bar", "// this is a header\n"
                                    "// some license stuff here\n"
                                    "\n"
                                    "/// \\brief Foo\n"
                                    "int foo;\n"));
}

TEST(IncludeDirectivesTest, preferablyAngledNextToAngled) {
  EXPECT_EQ("#include <virtual/foo.h>\n"
            "#include <bar>\n"
            "#include \"virtual/bar.h\"\n",
            addIncludeInCode("bar", "#include <virtual/foo.h>\n"
                                    "#include \"virtual/bar.h\"\n"));
  EXPECT_EQ("#include \"virtual/foo.h\"\n"
            "#include \"virtual/bar.h\"\n"
            "#include <bar>\n",
            addIncludeInCode("bar", "#include \"virtual/foo.h\"\n"
                                    "#include \"virtual/bar.h\"\n"));
}

TEST(IncludeDirectivesTest, avoidDuplicates) {
  EXPECT_EQ("#include <virtual/foo.h>\n",
            addIncludeInCode("virtual/foo.h", "#include <virtual/foo.h>\n"));
}

// Tests includes in the middle of the code are ignored.
TEST(IncludeDirectivesTest, ignoreHeadersMeantForMultipleInclusion) {
  std::string Expected = "#include \"virtual/foo.h\"\n"
                         "#include <bar>\n"
                         "\n"
                         "enum Kind {\n"
                         "#define X(A) K_##A,\n"
                         "#include \"virtual/xmacro.def\"\n"
                         "#undef X\n"
                         "  K_NUM_KINDS\n"
                         "};\n";
  std::string Result =
      addIncludeInCode("bar", "#include \"virtual/foo.h\"\n"
                              "\n"
                              "enum Kind {\n"
                              "#define X(A) K_##A,\n"
                              "#include \"virtual/xmacro.def\"\n"
                              "#undef X\n"
                              "  K_NUM_KINDS\n"
                              "};\n");
  EXPECT_EQ(Expected, Result);
}

namespace {
TestAddIncludeAction *makeIndirectTestsAction(const char *HeaderToModify,
                                              tooling::Replacements &Replaces) {
  StringRef IncludeToAdd = "virtual/c.h";
  TestAddIncludeAction *TestAction =
      new TestAddIncludeAction(IncludeToAdd, Replaces, HeaderToModify);
  TestAction->mapVirtualHeader("c.h", "#pragma once\n");
  TestAction->mapVirtualHeader("a.h", "#pragma once\n"
                                      "#include <virtual/c.h>\n");
  TestAction->mapVirtualHeader("b.h", "#pragma once\n");
  return TestAction;
}
} // end anonymous namespace

TEST(IncludeDirectivesTest, indirectIncludes) {
  // In TestAddIncludeAction 'foo.h' includes 'foo-inner.h'. Check that we
  // aren't including foo-inner.h again.
  EXPECT_EQ(
      "#include <virtual/foo.h>\n",
      addIncludeInCode("virtual/foo-inner.h", "#include <virtual/foo.h>\n"));

  tooling::Replacements Replaces;
  StringRef Code = "#include <virtual/a.h>\n"
                   "#include <virtual/b.h>\n";

  // a.h already includes c.h
  {
    FrontendAction *Action = makeIndirectTestsAction("a.h", Replaces);
    ASSERT_NO_FATAL_FAILURE(applyActionOnCode(Action, Code));
    EXPECT_EQ(unsigned(0), Replaces.size());
  }

  // c.h is included before b.h but b.h doesn't include c.h directly, so check
  // that it will be inserted.
  {
    FrontendAction *Action = makeIndirectTestsAction("b.h", Replaces);
    ASSERT_NO_FATAL_FAILURE(applyActionOnCode(Action, Code));
    EXPECT_EQ("#include <virtual/c.h>\n\n\n",
              tooling::applyAllReplacements("\n", Replaces));
  }
}

/// \brief Convenience method to test header guards detection implementation.
static std::string addIncludeInGuardedHeader(StringRef IncludeToAdd,
                                             StringRef GuardedHeaderCode) {
  const char *GuardedHeaderName = "guarded.h";
  tooling::Replacements Replaces;
  TestAddIncludeAction *TestAction =
      new TestAddIncludeAction(IncludeToAdd, Replaces, GuardedHeaderName);
  TestAction->mapVirtualHeader(GuardedHeaderName, GuardedHeaderCode);

  applyActionOnCode(TestAction, "#include <virtual/guarded.h>\n");
  if (::testing::Test::HasFailure())
    return "<<unexpected error from applyActionOnCode()>>";

  return tooling::applyAllReplacements(GuardedHeaderCode, Replaces);
}

TEST(IncludeDirectivesTest, insertInsideIncludeGuard) {
  EXPECT_EQ("#ifndef GUARD_H\n"
            "#define GUARD_H\n"
            "\n"
            "#include <foo>\n"
            "\n"
            "struct foo {};\n"
            "\n"
            "#endif // GUARD_H\n",
            addIncludeInGuardedHeader("foo", "#ifndef GUARD_H\n"
                                             "#define GUARD_H\n"
                                             "\n"
                                             "struct foo {};\n"
                                             "\n"
                                             "#endif // GUARD_H\n"));
}

TEST(IncludeDirectivesTest, guardAndHeader) {
  EXPECT_EQ("// File header\n"
            "\n"
            "#ifndef GUARD_H\n"
            "#define GUARD_H\n"
            "\n"
            "#include <foo>\n"
            "\n"
            "struct foo {};\n"
            "\n"
            "#endif // GUARD_H\n",
            addIncludeInGuardedHeader("foo", "// File header\n"
                                             "\n"
                                             "#ifndef GUARD_H\n"
                                             "#define GUARD_H\n"
                                             "\n"
                                             "struct foo {};\n"
                                             "\n"
                                             "#endif // GUARD_H\n"));
}

TEST(IncludeDirectivesTest, fullHeaderFitsAsAPreamble) {
  EXPECT_EQ("#ifndef GUARD_H\n"
            "#define GUARD_H\n"
            "\n"
            "#include <foo>\n"
            "\n"
            "#define FOO 1\n"
            "\n"
            "#endif // GUARD_H\n",
            addIncludeInGuardedHeader("foo", "#ifndef GUARD_H\n"
                                             "#define GUARD_H\n"
                                             "\n"
                                             "#define FOO 1\n"
                                             "\n"
                                             "#endif // GUARD_H\n"));
}

TEST(IncludeDirectivesTest, codeBeforeIfndef) {
  EXPECT_EQ("#include <foo>\n"
            "\n"
            "int bar;\n"
            "\n"
            "#ifndef GUARD_H\n"
            "#define GUARD_H\n"
            "\n"
            "struct foo;"
            "\n"
            "#endif // GUARD_H\n",
            addIncludeInGuardedHeader("foo", "int bar;\n"
                                             "\n"
                                             "#ifndef GUARD_H\n"
                                             "#define GUARD_H\n"
                                             "\n"
                                             "struct foo;"
                                             "\n"
                                             "#endif // GUARD_H\n"));
}

TEST(IncludeDirectivesTest, codeAfterEndif) {
  EXPECT_EQ("#include <foo>\n"
            "\n"
            "#ifndef GUARD_H\n"
            "#define GUARD_H\n"
            "\n"
            "struct foo;"
            "\n"
            "#endif // GUARD_H\n"
            "\n"
            "int bar;\n",
            addIncludeInGuardedHeader("foo", "#ifndef GUARD_H\n"
                                             "#define GUARD_H\n"
                                             "\n"
                                             "struct foo;"
                                             "\n"
                                             "#endif // GUARD_H\n"
                                             "\n"
                                             "int bar;\n"));
}

TEST(IncludeDirectivesTest, headerGuardWithInclude) {
  EXPECT_EQ("#ifndef GUARD_H\n"
            "#define GUARD_H\n"
            "\n"
            "#include <virtual/bar.h>\n"
            "#include <foo>\n"
            "\n"
            "struct foo;\n"
            "\n"
            "#endif // GUARD_H\n",
            addIncludeInGuardedHeader("foo", "#ifndef GUARD_H\n"
                                             "#define GUARD_H\n"
                                             "\n"
                                             "#include <virtual/bar.h>\n"
                                             "\n"
                                             "struct foo;\n"
                                             "\n"
                                             "#endif // GUARD_H\n"));
}
