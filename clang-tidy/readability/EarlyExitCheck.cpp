//===--- EarlyExitCheck.cpp - clang-tidy-----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "EarlyExitCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {

void EarlyExitCheck::registerMatchers(MatchFinder *Finder) {
  auto Compound =
      compoundStmt(
          forEach(ifStmt(hasCondition(expr().bind("cond")),
                         hasThen(compoundStmt().bind("then")),
                         unless(hasConditionVariableStatement(declStmt())),
                         unless(hasElse(stmt())))
                      .bind("if")))
          .bind("comp");
  Finder->addMatcher(functionDecl(has(Compound)).bind("func"), this);
  Finder->addMatcher(forStmt(has(Compound)).bind("for"), this);
}

void EarlyExitCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *If = Result.Nodes.getNodeAs<IfStmt>("if");
  const auto *Compound = Result.Nodes.getNodeAs<CompoundStmt>("comp");
  if (If != Compound->body_back()) return;

  const auto *Condition = Result.Nodes.getNodeAs<Expr>("cond");
  const auto FixCondStart =
      FixItHint::CreateInsertion(Condition->getLocStart(), "!(");

  const auto FixCondEnd = FixItHint::CreateInsertion(
      Lexer::getLocForEndOfToken(Condition->getLocEnd(), 0,
                                 *Result.SourceManager,
                                 Result.Context->getLangOpts()),
      ")");

  const auto *Then = Result.Nodes.getNodeAs<CompoundStmt>("then");
  const auto OpeningBraceRange = CharSourceRange::getTokenRange(
      SourceRange(Then->getLocStart(), Then->getLocStart()));
  const auto FixExit =
      FixItHint::CreateReplacement(OpeningBraceRange, "return;");

  const auto ClosingBraceRange = CharSourceRange::getTokenRange(
      SourceRange(Then->getLocEnd(), Then->getLocEnd()));
  const auto FixBrace = FixItHint::CreateRemoval(ClosingBraceRange);

  diag(If->getLocStart(), "use early exit") << FixCondStart << FixCondEnd
                                            << FixExit << FixBrace;
}

} // namespace tidy
} // namespace clang

