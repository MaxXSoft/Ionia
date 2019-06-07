#include "back/interpreter/repl.h"

#include <sstream>
#include <iostream>
#include <string>
#include <cstdlib>

#include "readline/readline.h"
#include "readline/history.h"

#include "front/lexer.h"
#include "front/parser.h"

using namespace ionia;

void REPL::Run() {
  std::istringstream iss;
  Lexer lexer(iss);
  Parser parser(lexer);
  // run loop
  while (auto line = readline(prompt_)) {
    // reset string stream
    iss.str(line);
    iss.clear();
    // add current line to history
    if (*line) add_history(line);
    free(line);
    // parse current line
    parser.Reset();
    auto ast = parser.ParseNext();
    if (!ast) break;
    // evaluate and print
    auto val = ast->Eval(intp_);
    if (print_value_) {
      auto value_name = "$" + std::to_string(value_num_);
      std::cout << value_name << " = ";
      intp_.PrintValue(val);
      // add last value to root environment
      intp_.root()->AddSymbol(value_name, val);
    }
  }
  // print new line after EOF
  std::cout << std::endl;
}
