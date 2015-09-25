//===--- AwesomeFunctionsCheck.cpp - clang-tidy----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AwesomeFunctionsCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {

void AwesomeFunctionsCheck::registerMatchers(MatchFinder *Finder) {
  // FIXME: Add matchers.
  Finder->addMatcher(implicitCastExpr(hasSourceExpression(
                                          expr(hasType(arrayType())).bind("x")),
                                      unless(hasParent(arraySubscriptExpr()))),
                     this);
}

void AwesomeFunctionsCheck::check(const MatchFinder::MatchResult &Result) {
  // FIXME: Add callback implementation.
  const auto *Matched = Result.Nodes.getNodeAs<Expr>("x");
  diag(Matched->getSourceRange().getBegin(), "array to pointer")
      << FixItHint::CreateReplacement(
          Matched->getSourceRange(),
          ("&" + Lexer::getSourceText(
                     CharSourceRange::getTokenRange(Matched->getSourceRange()),
                     *Result.SourceManager, Result.Context->getLangOpts()) +
           "[0]")
              .str());
}

} // namespace tidy
} // namespace clang

