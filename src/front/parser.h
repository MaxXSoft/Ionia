#ifndef IONIA_FRONT_PARSER_H_
#define IONIA_FRONT_PARSER_H_

#include "front/lexer.h"
#include "define/ast.h"

namespace ionia {

class Parser {
 public:
  Parser(Lexer &lexer) : lexer_(lexer) { Reset(); }

  void Reset();
  ASTPtr ParseNext();

  unsigned int error_num() const { return error_num_; }

 private:
  using Token = Lexer::Token;

  Token NextToken() { return cur_token_ = lexer_.NextToken(); }
  bool IsTokenChar(char c) const {
    return cur_token_ == Token::Char && lexer_.char_val() == c;
  }

  ASTPtr PrintError(const char *message);
  ASTPtr ParseDefine();
  ASTPtr ParseExpr();
  ASTPtr ParseFunc();
  ASTPtr ParseFunCall();

  Lexer &lexer_;
  Token cur_token_;
  unsigned int error_num_;
};

}  // namespace ionia

#endif  // IONIA_FRONT_PARSER_H_
