#include <fstream>

#include "front/lexer.h"
#include "front/parser.h"
#include "back/interpreter/interpreter.h"

using namespace std;

int main(int argc, const char *argv[]) {
  // open file
  if (argc < 2) return 1;
  ifstream ifs(argv[1]);
  // create lexer, parser and interpreter
  Lexer lexer(ifs);
  Parser parser(lexer);
  Interpreter interpreter(parser);
  // parse and eval
  while (auto val = interpreter.EvalNext());
  // get error count
  auto err =
      lexer.error_num() + parser.error_num() + interpreter.error_num();
  return err;
}
