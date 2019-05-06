#include "front/lexer.h"

#include <iostream>
#include <cctype>
#include <cstdlib>

namespace {

inline bool IsIdChar(char c) {
  return c != '=' && c != '(' && c != ')' && c != ',' && c != ':' &&
         !std::isspace(c);
}

}  // namespace

Lexer::Token Lexer::PrintError(const char *message) {
  std::cerr << "error(lexer): " << message << std::endl;
  ++error_num_;
  return Token::Error;
}

Lexer::Token Lexer::HandleId() {
  std::string id;
  do {
    id += last_char_;
    NextChar();
  } while (!IsEOL() && IsIdChar(last_char_));
  id_val_ = id;
  return Token::Id;
}

Lexer::Token Lexer::HandleNum() {
  std::string num;
  do {
    num += last_char_;
    NextChar();
  } while (!IsEOL() && std::isdigit(last_char_));
  char *end_pos;
  num_val_ = std::strtol(num.c_str(), &end_pos, 10);
  return *end_pos || (num[0] == '0' && num.size() > 1)
             ? PrintError("invalid number") : Token::Num;
}

Lexer::Token Lexer::HandleComment() {
  NextChar();  // eat '#'
  while (!IsEOL()) NextChar();
  return NextToken();
}

Lexer::Token Lexer::HandleEOL() {
  do {
    NextChar();
  } while (IsEOL() && !in_.eof());
  return NextToken();
}

Lexer::Token Lexer::NextToken() {
  // end of file
  if (in_.eof()) return Token::End;
  // skip spaces
  while (!IsEOL() && std::isspace(last_char_)) NextChar();
  // skip comments
  if (last_char_ == '#') return HandleComment();
  // number
  if (std::isdigit(last_char_)) return HandleNum();
  // identifier
  if (IsIdChar(last_char_)) return HandleId();
  // end of line
  if (IsEOL()) return HandleEOL();
  // other characters
  char_val_ = last_char_;
  NextChar();
  return Token::Char;
}
