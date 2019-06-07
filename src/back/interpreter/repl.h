#ifndef IONIA_BACK_INTERPRETER_REPL_H_
#define IONIA_BACK_INTERPRETER_REPL_H_

#include "back/interpreter/interpreter.h"

namespace ionia {

class REPL {
 public:
  REPL(Interpreter &intp)
      : intp_(intp), print_value_(false),
        prompt_("ionia> "), value_num_(0) {}

  void Run();

  // setters
  void set_print_value(bool print_value) { print_value_ = print_value; }
  void set_prompt(const char *prompt) { prompt_ = prompt; }

 private:
  Interpreter &intp_;
  bool print_value_;
  const char *prompt_;
  unsigned int value_num_;
};

}  // namespace ionia

#endif  // IONIA_BACK_INTERPRETER_REPL_H_
