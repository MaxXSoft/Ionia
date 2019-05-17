#include "front/parser.h"

#include <iostream>

using namespace ionia;

ASTPtr Parser::PrintError(const char *message) {
  std::cerr << "error(parser): " << message << std::endl;
  ++error_num_;
  return nullptr;
}

ASTPtr Parser::ParseDefine() {
  // get id
  auto id = lexer_.id_val();
  // eat '='
  NextToken();
  // get expression
  auto expr = ParseExpr();
  if (!expr) return nullptr;
  return std::make_unique<DefineAST>(id, std::move(expr));
}

ASTPtr Parser::ParseExpr() {
  if (IsTokenChar('(')) {
    return ParseFunc();
  }
  else {
    switch (cur_token_) {
      case Token::Id: {
        auto id = lexer_.id_val();
        NextToken();
        // check if is function call or define
        if (IsTokenChar('(')) {
          return ParseFunCall();
        }
        else if (IsTokenChar('=')) {
          return ParseDefine();
        }
        else {
          return std::make_unique<IdAST>(id);
        }
      }
      case Token::Num: {
        auto num = lexer_.num_val();
        NextToken();
        return std::make_unique<NumAST>(num);
      }
      default:;
    }
  }
  return PrintError("invalid expression");
}

ASTPtr Parser::ParseFunc() {
  // eat '('
  NextToken();
  // read argument list
  IdList args;
  if (cur_token_ == Token::Id) {
    args.push_back(lexer_.id_val());
    NextToken();
    // check ','
    while (IsTokenChar(',')) {
      NextToken();
      if (cur_token_ != Token::Id) return PrintError("expected id");
      args.push_back(lexer_.id_val());
      NextToken();
    }
  }
  // check ')'
  if (!IsTokenChar(')')) return PrintError("expected ')'");
  NextToken();
  // check ':'
  if (!IsTokenChar(':')) return PrintError("expected ':'");
  NextToken();
  // get expression
  auto expr = ParseExpr();
  if (!expr) return nullptr;
  return std::make_unique<FuncAST>(std::move(args), std::move(expr));
}

ASTPtr Parser::ParseFunCall() {
  // get id
  auto id = lexer_.id_val();
  // eat '('
  NextToken();
  // get arguments
  ASTPtrList args;
  if (!IsTokenChar(')')) {
    // get first argument
    auto expr = ParseExpr();
    if (!expr) return nullptr;
    args.push_back(std::move(expr));
    // get the rest
    while (IsTokenChar(',')) {
      NextToken();
      expr = ParseExpr();
      if (!expr) return nullptr;
      args.push_back(std::move(expr));
    }
  }
  // check ')'
  if (!IsTokenChar(')')) return PrintError("expected ')'");
  NextToken();
  return std::make_unique<FunCallAST>(id, std::move(args));
}

ASTPtr Parser::ParseNext() {
  if (cur_token_ == Token::End) return nullptr;
  if (cur_token_ != Token::Id) return PrintError("expected id");
  NextToken();
  // get statement
  ASTPtr stat;
  if (IsTokenChar('=')) {
    stat = ParseDefine();
  }
  else if (IsTokenChar('(')) {
    stat = ParseFunCall();
  }
  else {
    return PrintError("invalid statment");
  }
  if (!stat) return nullptr;
  return stat;
}
