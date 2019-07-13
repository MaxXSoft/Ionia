#include "front/parser.h"

#include <iostream>

using namespace ionia;

ASTPtr Parser::PrintError(const char *message) {
  std::cerr << "error(parser): " << message << std::endl;
  ++error_num_;
  return nullptr;
}

ASTPtr Parser::ParseDefine(const std::string &id) {
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
          return ParseFunCall(std::make_unique<IdAST>(id));
        }
        else if (IsTokenChar('=')) {
          return ParseDefine(id);
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

ASTPtr Parser::ParseFunCall(ASTPtr callee) {
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
  // create AST
  auto ast = std::make_unique<FunCallAST>(std::move(callee),
                                          std::move(args));
  // check if need to continue
  if (IsTokenChar('(')) {
    return ParseFunCall(std::move(ast));
  }
  else {
    return ast;
  }
}

void Parser::Reset() {
  lexer_.Reset();
  error_num_ = 0;
  NextToken();
}

ASTPtr Parser::ParseNext() {
  if (cur_token_ == Token::End) return nullptr;
  if (cur_token_ != Token::Id) return PrintError("expected id");
  auto id = lexer_.id_val();
  NextToken();
  // get statement
  ASTPtr stat;
  if (IsTokenChar('=')) {
    stat = ParseDefine(id);
  }
  else if (IsTokenChar('(')) {
    stat = ParseFunCall(std::make_unique<IdAST>(id));
  }
  else {
    return PrintError("invalid statment");
  }
  if (!stat) return nullptr;
  return stat;
}
