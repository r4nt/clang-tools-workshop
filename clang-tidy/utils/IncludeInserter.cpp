//===-------- IncludeInserter.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "IncludeInserter.h"

namespace clang {
namespace tidy {

class IncludeInserterCallback : public PPCallbacks {
public:
  explicit IncludeInserterCallback(IncludeInserter *Inserter)
      : Inserter(Inserter) {}
  // Implements PPCallbacks::InclusionDerective(). Records the names and source
  // locations of the inclusions in the main source file being processed.
  void InclusionDirective(SourceLocation HashLocation,
                          const Token & /*include_token*/,
                          StringRef FileNameRef, bool IsAngled,
                          CharSourceRange FileNameRange,
                          const FileEntry * /*IncludedFile*/,
                          StringRef /*SearchPath*/, StringRef /*RelativePath*/,
                          const Module * /*ImportedModule*/) override {
    Inserter->AddInclude(FileNameRef, IsAngled, HashLocation,
                         FileNameRange.getEnd());
  }

private:
  IncludeInserter *Inserter;
};

IncludeInserter::IncludeInserter(const SourceManager &SourceMgr,
                                 const LangOptions &LangOpts,
                                 IncludeSorter::IncludeStyle Style)
    : SourceMgr(SourceMgr), LangOpts(LangOpts), Style(Style) {}

IncludeInserter::~IncludeInserter() = default;

std::unique_ptr<PPCallbacks> IncludeInserter::CreatePPCallbacks() {
  return llvm::make_unique<IncludeInserterCallback>(this);
}

llvm::Optional<FixItHint>
IncludeInserter::CreateIncludeInsertion(FileID FileID, StringRef Header,
                                        bool IsAngled) {
  // We assume the same Header will never be included both angled and not
  // angled.
  if (!InsertedHeaders[FileID].insert(Header).second)
    return llvm::None;

  if (IncludeSorterByFile.find(FileID) == IncludeSorterByFile.end()) {
    // This may happen if there have been no preprocessor directives in this
    // file.
    IncludeSorterByFile.insert(std::make_pair(
        FileID,
        llvm::make_unique<IncludeSorter>(
            &SourceMgr, &LangOpts, FileID,
            SourceMgr.getFilename(SourceMgr.getLocForStartOfFile(FileID)),
            Style)));
  }
  return IncludeSorterByFile[FileID]->CreateIncludeInsertion(Header, IsAngled);
}

void IncludeInserter::AddInclude(StringRef file_name, bool IsAngled,
                                 SourceLocation HashLocation,
                                 SourceLocation end_location) {
  FileID FileID = SourceMgr.getFileID(HashLocation);
  if (IncludeSorterByFile.find(FileID) == IncludeSorterByFile.end()) {
    IncludeSorterByFile.insert(std::make_pair(
        FileID, llvm::make_unique<IncludeSorter>(
                    &SourceMgr, &LangOpts, FileID,
                    SourceMgr.getFilename(HashLocation), Style)));
  }
  IncludeSorterByFile[FileID]->AddInclude(file_name, IsAngled, HashLocation,
                                          end_location);
}

} // namespace tidy
} // namespace clang
