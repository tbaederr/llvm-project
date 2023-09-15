//===--- CodeSnippetHighlighter.h - Code snippet highlighting ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_FRONTEND_CODESNIPPETHIGHLIGHTER_H
#define LLVM_CLANG_FRONTEND_CODESNIPPETHIGHLIGHTER_H

#include "clang/Basic/LangOptions.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

namespace clang {

struct StyleRange {
  unsigned Start;
  unsigned End;
  enum llvm::raw_ostream::Colors Color;
  StyleRange(unsigned S, unsigned E, enum llvm::raw_ostream::Colors C)
      : Start(S), End(E), Color(C){};
};

class Preprocessor;
class FileID;
class SourceManager;

class CodeSnippetHighlighter final {
public:
  CodeSnippetHighlighter() = default;

  /// Produce StyleRanges for the given line.
  /// The returned vector contains non-overlapping style ranges. They are sorted
  /// from beginning of the line to the end.
  llvm::SmallVector<StyleRange>
  highlightLine(unsigned LineNumber, const Preprocessor *PP,
                const LangOptions &LangOpts, FileID FID,
                const SourceManager &SM, const char *LineStart);
};

} // namespace clang

#endif
