//===--- UnusedAliasDeclsCheck.cpp - clang-tidy----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "UnusedAliasDeclsCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {

// FIXME: Move this to ASTMatchers.h.
const ast_matchers::internal::VariadicDynCastAllOfMatcher<
    Decl, NamespaceAliasDecl> namespaceAliasDecl;

void UnusedAliasDeclsCheck::registerMatchers(MatchFinder *Finder) {
  // We cannot do anything about headers (yet), as the alias declarations
  // used in one header could be used by some other translation unit.
  Finder->addMatcher(namespaceAliasDecl(isExpansionInMainFile()).bind("alias"),
                     this);
  Finder->addMatcher(nestedNameSpecifier().bind("nns"), this);
}

void UnusedAliasDeclsCheck::check(const MatchFinder::MatchResult &Result) {
  if (const auto *AliasDecl = Result.Nodes.getNodeAs<Decl>("alias")) {
    FoundDecls[AliasDecl] = CharSourceRange::getCharRange(
        AliasDecl->getLocStart(),
        Lexer::findLocationAfterToken(
            AliasDecl->getLocEnd(), tok::semi, *Result.SourceManager,
            Result.Context->getLangOpts(),
            /*SkipTrailingWhitespaceAndNewLine=*/true));
    return;
  }

  if (const auto *NestedName =
          Result.Nodes.getNodeAs<NestedNameSpecifier>("nns")) {
    if (const auto *AliasDecl = NestedName->getAsNamespaceAlias()) {
      FoundDecls[AliasDecl] = CharSourceRange();
    }
  }
}

void UnusedAliasDeclsCheck::onEndOfTranslationUnit() {
  for (const auto &FoundDecl : FoundDecls) {
    if (!FoundDecl.second.isValid())
      continue;
    diag(FoundDecl.first->getLocation(), "this namespace alias decl is unused")
        << FixItHint::CreateRemoval(FoundDecl.second);
  }
}

} // namespace tidy
} // namespace clang
