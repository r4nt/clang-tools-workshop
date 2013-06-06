#include "Core/Transform.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

void collectResults(clang::Rewriter &Rewrite,
                    const FileContentsByPath &InputStates,
                    FileContentsByPath &Results) {
  // Copy the contents of InputStates to be modified.
  Results = InputStates;

  for (Rewriter::buffer_iterator I = Rewrite.buffer_begin(),
                                 E = Rewrite.buffer_end();
       I != E; ++I) {
    const FileEntry *Entry = Rewrite.getSourceMgr().getFileEntryForID(I->first);
    assert(Entry != 0 && "Expected a FileEntry");
    assert(Entry->getName() != 0 &&
           "Unexpected NULL return from FileEntry::getName()");

    std::string ResultBuf;

    // Get a copy of the rewritten buffer from the Rewriter.
    llvm::raw_string_ostream StringStream(ResultBuf);
    I->second.write(StringStream);

    // Cause results to be written to ResultBuf.
    StringStream.str();

    // FIXME: Use move semantics to avoid copies of the buffer contents if
    // benchmarking shows the copies are expensive, especially for large source
    // files.
    Results[Entry->getName()] = ResultBuf;
  }
}

bool Transform::handleBeginSource(CompilerInstance &CI, StringRef Filename) {
  if (!Options().EnableTiming)
    return true;

  Timings.push_back(std::make_pair(Filename.str(), llvm::TimeRecord()));
  Timings.back().second -= llvm::TimeRecord::getCurrentTime(true);
  return true;
}

void Transform::handleEndSource() {
  if (!Options().EnableTiming)
    return;

  Timings.back().second += llvm::TimeRecord::getCurrentTime(false);
}

void Transform::addTiming(llvm::StringRef Label, llvm::TimeRecord Duration) {
  Timings.push_back(std::make_pair(Label.str(), Duration));
}
