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
/// See user documentation for usage instructions.
///
//===----------------------------------------------------------------------===//

#include "Core/FileOverrides.h"
#include "Core/PerfSupport.h"
#include "Core/SyntaxCheck.h"
#include "Core/Transform.h"
#include "Core/Transforms.h"
#include "Core/Reformatting.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Signals.h"

namespace cl = llvm::cl;
using namespace clang;
using namespace clang::tooling;

TransformOptions GlobalOptions;

static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::opt<std::string> BuildPath(
    "p", cl::desc("Build Path"), cl::Optional);
static cl::list<std::string> SourcePaths(
    cl::Positional, cl::desc("<source0> [... <sourceN>]"), cl::OneOrMore);
static cl::extrahelp MoreHelp(
    "EXAMPLES:\n\n"
    "Apply all transforms on a given file, no compilation database:\n\n"
    "  cpp11-migrate path/to/file.cpp -- -Ipath/to/include/\n"
    "\n"
    "Convert for loops to the new ranged-based for loops on all files in a "
    "subtree\nand reformat the code automatically using the LLVM style:\n\n"
    "  find path/in/subtree -name '*.cpp' -exec \\\n"
    "    cpp11-migrate -p build/path -format-style=LLVM -loop-convert {} ';'\n"
    "\n"
    "Make use of both nullptr and the override specifier, using git ls-files:\n"
    "\n"
    "  git ls-files '*.cpp' | xargs -I{} cpp11-migrate -p build/path \\\n"
    "    -use-nullptr -add-override -override-macros {}\n"
    "\n"
    "Apply all transforms supported by both clang >= 3.0 and gcc >= 4.7:\n\n"
    "  cpp11-migrate -for-compilers=clang-3.0,gcc-4.7 foo.cpp -- -Ibar\n");

static cl::opt<RiskLevel, /*ExternalStorage=*/true> MaxRiskLevel(
    "risk", cl::desc("Select a maximum risk level:"),
    cl::values(clEnumValN(RL_Safe, "safe", "Only safe transformations"),
               clEnumValN(RL_Reasonable, "reasonable",
                          "Enable transformations that might change "
                          "semantics (default)"),
               clEnumValN(RL_Risky, "risky",
                          "Enable transformations that are likely to "
                          "change semantics"),
               clEnumValEnd),
    cl::location(GlobalOptions.MaxRiskLevel),
    cl::init(RL_Reasonable));

static cl::opt<bool> FinalSyntaxCheck(
    "final-syntax-check",
    cl::desc("Check for correct syntax after applying transformations"),
    cl::init(false));

static cl::opt<std::string> FormatStyleOpt(
    "format-style",
    cl::desc("Coding style to use on the replacements, either a builtin style\n"
             "or a YAML config file (see: clang-format -dump-config).\n"
             "Currently supports 4 builtins style:\n"
             "  LLVM, Google, Chromium, Mozilla.\n"),
    cl::value_desc("string"));

static cl::opt<bool>
SummaryMode("summary", cl::desc("Print transform summary"),
            cl::init(false));

const char NoTiming[] = "no_timing";
static cl::opt<std::string> TimingDirectoryName(
    "perf", cl::desc("Capture performance data and output to specified "
                     "directory. Default: ./migrate_perf"),
    cl::init(NoTiming), cl::ValueOptional, cl::value_desc("directory name"));

// TODO: Remove cl::Hidden when functionality for acknowledging include/exclude
// options are implemented in the tool.
static cl::opt<std::string>
IncludePaths("include", cl::Hidden,
             cl::desc("Comma seperated list of paths to consider to be "
                      "transformed"));
static cl::opt<std::string>
ExcludePaths("exclude", cl::Hidden,
             cl::desc("Comma seperated list of paths that can not "
                      "be transformed"));
static cl::opt<std::string>
IncludeFromFile("include-from", cl::Hidden, cl::value_desc("filename"),
                cl::desc("File containing a list of paths to consider to "
                         "be transformed"));
static cl::opt<std::string>
ExcludeFromFile("exclude-from", cl::Hidden, cl::value_desc("filename"),
                cl::desc("File containing a list of paths that can not be "
                         "transforms"));

// Header modifications will probably be always on eventually. For now, they
// need to be explicitly enabled.
static cl::opt<bool, /*ExternalStorage=*/true> EnableHeaderModifications(
    "headers",
    cl::Hidden, // Experimental feature for now.
    cl::desc("Enable modifications to headers"),
    cl::location(GlobalOptions.EnableHeaderModifications),
    cl::init(false));

static cl::opt<bool> YAMLOnly("yaml-only",
                              cl::Hidden, // Associated with -headers
                              cl::desc("Don't change headers on disk. Write "
                                       "changes to change description files "
                                       "only."),
                              cl::init(false));

cl::opt<std::string> SupportedCompilers(
    "for-compilers", cl::value_desc("string"),
    cl::desc("Select transforms targeting the intersection of\n"
             "language features supported by the given compilers.\n"
             "Takes a comma-seperated list of <compiler>-<version>.\n"
             "\t<compiler> can be any of: clang, gcc, icc, msvc\n"
             "\t<version> is <major>[.<minor>]\n"));

/// \brief Extract the minimum compiler versions as requested on the command
/// line by the switch \c -for-compilers.
///
/// \param ProgName The name of the program, \c argv[0], used to print errors.
/// \param Error If an error occur while parsing the versions this parameter is
/// set to \c true, otherwise it will be left untouched.
static CompilerVersions handleSupportedCompilers(const char *ProgName,
                                                 bool &Error) {
  if (SupportedCompilers.getNumOccurrences() == 0)
    return CompilerVersions();
  CompilerVersions RequiredVersions;
  llvm::SmallVector<llvm::StringRef, 4> Compilers;

  llvm::StringRef(SupportedCompilers).split(Compilers, ",");

  for (llvm::SmallVectorImpl<llvm::StringRef>::iterator I = Compilers.begin(),
                                                        E = Compilers.end();
       I != E; ++I) {
    llvm::StringRef Compiler, VersionStr;
    llvm::tie(Compiler, VersionStr) = I->split('-');
    Version *V = llvm::StringSwitch<Version *>(Compiler)
        .Case("clang", &RequiredVersions.Clang)
        .Case("gcc", &RequiredVersions.Gcc).Case("icc", &RequiredVersions.Icc)
        .Case("msvc", &RequiredVersions.Msvc).Default(NULL);

    if (V == NULL) {
      llvm::errs() << ProgName << ": " << Compiler
                   << ": unsupported platform\n";
      Error = true;
      continue;
    }
    if (VersionStr.empty()) {
      llvm::errs() << ProgName << ": " << *I
                   << ": missing version number in platform\n";
      Error = true;
      continue;
    }

    Version Version = Version::getFromString(VersionStr);
    if (Version.isNull()) {
      llvm::errs()
          << ProgName << ": " << *I
          << ": invalid version, please use \"<major>[.<minor>]\" instead of \""
          << VersionStr << "\"\n";
      Error = true;
      continue;
    }
    // support the lowest version given
    if (V->isNull() || Version < *V)
      *V = Version;
  }
  return RequiredVersions;
}

/// \brief Creates the Reformatter if the format style option is provided,
/// return a null pointer otherwise.
///
/// \param ProgName The name of the program, \c argv[0], used to print errors.
/// \param Error If the \c -format-style is provided but with wrong parameters
/// this is parameter is set to \c true, left untouched otherwise. An error
/// message is printed with an explanation.
static Reformatter *handleFormatStyle(const char *ProgName, bool &Error) {
  if (FormatStyleOpt.getNumOccurrences() > 0) {
    format::FormatStyle Style;
    if (!format::getPredefinedStyle(FormatStyleOpt, &Style)) {
      llvm::StringRef ConfigFilePath = FormatStyleOpt;
      llvm::OwningPtr<llvm::MemoryBuffer> Text;
      llvm::error_code ec;

      ec = llvm::MemoryBuffer::getFile(ConfigFilePath, Text);
      if (!ec)
        ec = parseConfiguration(Text->getBuffer(), &Style);

      if (ec) {
        llvm::errs() << ProgName << ": invalid format style " << FormatStyleOpt
                     << ": " << ec.message() << "\n";
        Error = true;
        return 0;
      }
    }

    // force mode to C++11
    Style.Standard = clang::format::FormatStyle::LS_Cpp11;
    return new Reformatter(Style);
  }
  return 0;
}

int main(int argc, const char **argv) {
  llvm::sys::PrintStackTraceOnErrorSignal();
  Transforms TransformManager;

  TransformManager.registerTransforms();

  // Parse options and generate compilations.
  OwningPtr<CompilationDatabase> Compilations(
      FixedCompilationDatabase::loadFromCommandLine(argc, argv));
  cl::ParseCommandLineOptions(argc, argv);

  if (!Compilations) {
    std::string ErrorMessage;
    if (BuildPath.getNumOccurrences() > 0) {
      Compilations.reset(CompilationDatabase::autoDetectFromDirectory(
          BuildPath, ErrorMessage));
    } else {
      Compilations.reset(CompilationDatabase::autoDetectFromSource(
          SourcePaths[0], ErrorMessage));
      // If no compilation database can be detected from source then we create
      // a new FixedCompilationDatabase with c++11 support.
      if (!Compilations) {
        std::string CommandLine[] = {"-std=c++11"};
        Compilations.reset(new FixedCompilationDatabase(".", CommandLine));
      }
    }
    if (!Compilations)
      llvm::report_fatal_error(ErrorMessage);
  }

  // Since ExecutionTimeDirectoryName could be an empty string we compare
  // against the default value when the command line option is not specified.
  GlobalOptions.EnableTiming = (TimingDirectoryName != NoTiming);

  // Check the reformatting style option
  bool CmdSwitchError = false;
  llvm::OwningPtr<Reformatter> ChangesReformatter(
      handleFormatStyle(argv[0], CmdSwitchError));

  CompilerVersions RequiredVersions =
      handleSupportedCompilers(argv[0], CmdSwitchError);
  if (CmdSwitchError)
    return 1;

  // Populate the ModifiableHeaders structure if header modifications are
  // enabled.
  if (GlobalOptions.EnableHeaderModifications) {
    GlobalOptions.ModifiableHeaders
        .readListFromString(IncludePaths, ExcludePaths);
    GlobalOptions.ModifiableHeaders
        .readListFromFile(IncludeFromFile, ExcludeFromFile);
  }

  TransformManager.createSelectedTransforms(GlobalOptions, RequiredVersions);

  if (TransformManager.begin() == TransformManager.end()) {
    if (SupportedCompilers.empty())
      llvm::errs() << argv[0] << ": no selected transforms\n";
    else
      llvm::errs() << argv[0]
                   << ": no transforms available for specified compilers\n";
    return 1;
  }

  if (std::distance(TransformManager.begin(), TransformManager.end()) > 1 &&
      YAMLOnly) {
    llvm::errs() << "Header change description files requested for multiple "
                    "transforms.\nChanges from only one transform can be "
                    "recorded in a change description file.\n";
    return 1;
  }

  // if reformatting is enabled we wants to track file changes so that it's
  // possible to reformat them.
  bool TrackReplacements = static_cast<bool>(ChangesReformatter);
  FileOverrides FileStates(TrackReplacements);
  SourcePerfData PerfData;

  // Apply transforms.
  for (Transforms::const_iterator I = TransformManager.begin(),
                                  E = TransformManager.end();
       I != E; ++I) {
    Transform *T = *I;

    if (T->apply(FileStates, *Compilations, SourcePaths) != 0) {
      // FIXME: Improve ClangTool to not abort if just one file fails.
      return 1;
    }

    if (GlobalOptions.EnableTiming)
      collectSourcePerfData(*T, PerfData);

    if (SummaryMode) {
      llvm::outs() << "Transform: " << T->getName()
                   << " - Accepted: " << T->getAcceptedChanges();
      if (T->getChangesNotMade()) {
        llvm::outs() << " - Rejected: " << T->getRejectedChanges()
                     << " - Deferred: " << T->getDeferredChanges();
      }
      llvm::outs() << "\n";
    }
  }

  // Reformat changes if a reformatter is provided.
  if (ChangesReformatter)
    for (FileOverrides::const_iterator I = FileStates.begin(),
                                       E = FileStates.end();
         I != E; ++I) {
      SourceOverrides &Overrides = *I->second;
      ChangesReformatter->reformatChanges(Overrides);
    }

  if (FinalSyntaxCheck)
    if (!doSyntaxCheck(*Compilations, SourcePaths, FileStates))
      return 1;

  // Write results to file.
  for (FileOverrides::const_iterator I = FileStates.begin(),
                                     E = FileStates.end();
       I != E; ++I) {
    const SourceOverrides &Overrides = *I->second;
    if (Overrides.isSourceOverriden()) {
      std::string ErrorInfo;
      std::string MainFileName = I->getKey();
      llvm::raw_fd_ostream FileStream(MainFileName.c_str(), ErrorInfo,
                                      llvm::sys::fs::F_Binary);
      FileStream << Overrides.getMainFileContent();
    }

    for (HeaderOverrides::const_iterator HeaderI = Overrides.headers_begin(),
                                         HeaderE = Overrides.headers_end();
         HeaderI != HeaderE; ++HeaderI) {
      std::string ErrorInfo;
      if (YAMLOnly) {
        // Replacements for header files need to be written in a YAML file for
        // every transform and will be merged together with an external tool.
        llvm::SmallString<128> ReplacementsHeaderName;
        llvm::SmallString<64> Error;
        bool Result =
            generateReplacementsFileName(I->getKey(), HeaderI->getKey().data(),
                                         ReplacementsHeaderName, Error);
        if (!Result) {
          llvm::errs() << "Failed to generate replacements filename:" << Error
                       << "\n";
          continue;
        }

        llvm::raw_fd_ostream ReplacementsFile(
            ReplacementsHeaderName.c_str(), ErrorInfo, llvm::sys::fs::F_Binary);
        if (!ErrorInfo.empty()) {
          llvm::errs() << "Error opening file: " << ErrorInfo << "\n";
          continue;
        }
        llvm::yaml::Output YAML(ReplacementsFile);
        YAML << const_cast<TranslationUnitReplacements &>(
                    HeaderI->getValue().getReplacements());
      } else {
        // If -yaml-only was not specified, then change headers on disk.
        // FIXME: This is transitional behaviour. Remove this functionality
        // when header change description tool is ready.
        assert(!HeaderI->second.getContentOverride().empty() &&
               "A header override should not be empty");
        std::string HeaderFileName = HeaderI->getKey();
        llvm::raw_fd_ostream HeaderStream(HeaderFileName.c_str(), ErrorInfo,
                                          llvm::sys::fs::F_Binary);
        if (!ErrorInfo.empty()) {
          llvm::errs() << "Error opening file: " << ErrorInfo << "\n";
          continue;
        }
        HeaderStream << HeaderI->second.getContentOverride();
      }
    }
  }

  // Report execution times.
  if (GlobalOptions.EnableTiming && !PerfData.empty()) {
    std::string DirectoryName = TimingDirectoryName;
    // Use default directory name.
    if (DirectoryName.empty())
      DirectoryName = "./migrate_perf";
    writePerfDataJSON(DirectoryName, PerfData);
  }

  return 0;
}

// These anchors are used to force the linker to link the transforms
extern volatile int AddOverrideTransformAnchorSource;
extern volatile int LoopConvertTransformAnchorSource;
extern volatile int ReplaceAutoPtrTransformAnchorSource;
extern volatile int UseAutoTransformAnchorSource;
extern volatile int UseNullptrTransformAnchorSource;

static int TransformsAnchorsDestination[] = {
  AddOverrideTransformAnchorSource,
  LoopConvertTransformAnchorSource,
  ReplaceAutoPtrTransformAnchorSource,
  UseAutoTransformAnchorSource,
  UseNullptrTransformAnchorSource
};
