#ifndef IONIA_VM_VM_H_
#define IONIA_VM_VM_H_

#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <cstdint>

#include "vm/define.h"

namespace ionia::vm {

class VM {
 public:
  // definition of value stack
  using ValueStack = std::stack<Value>;
  // definition of external function
  using ExtFunc = std::function<bool(ValueStack &, Value &)>;
  // definition of symbol error handler
  using ErrorHandler = std::function<bool(const std::string &, Value &)>;

  VM() { Reset(); }

  bool LoadProgram(const std::string &file);
  bool LoadProgram(const std::vector<std::uint8_t> &buffer);

  // register an external function
  bool RegisterFunction(const std::string &name, ExtFunc func);
  // register an external function
  // if success, return function value
  bool RegisterFunction(const std::string &name, ExtFunc func,
                        Value &ret);
  // register an anonymous function
  void RegisterAnonFunc(ExtFunc func, Value &ret);
  // call a global function in vitrual machine
  bool CallFunction(const std::string &name,
                    const std::vector<Value> &args, Value &ret);
  // call a function by value
  bool CallFunction(const Value &func, const std::vector<Value> &args,
                    Value &ret);

  // reset VM's status (except symbol table, FPT, GFT and EFT)
  void Reset();
  // run current program
  bool Run();

  // setters
  // set handler that will be called when a symbol error occurs
  void set_sym_error_handler(ErrorHandler handler) {
    sym_error_handler_ = handler;
  }

 private:
  // supported operators by Ionia VM
  enum class Operator {
    Equal, NotEqual, Less, LessEqual, Great, GreatEqual,
    Add, Sub, Mul, Div, Mod,
    And, Or, Not, Xor, Shl, Shr,
    LogicAnd, LogicOr, LogicNot,
  };

  // print error message
  bool PrintError(const char *message);
  bool PrintError(const char *message, const char *symbol);

  // initialize external function table
  // add all Ionia standard functions, like 'is', '?', 'eq', '+'...
  void InitExtFuncs();
  // get value from current environment, return false if not found
  bool GetEnvValue(Inst *inst, Value &value);

  // call a VM function
  bool DoCall(const Value &func);
  // tail call a VM function
  bool DoTailCall(const Value &func);

  // bind member functions to external function
  template <typename Func, typename... Args>
  void BindExtFunc(const std::string &name, Func func, Args... args) {
    using namespace std::placeholders;
    RegisterFunction(name, std::bind(func, this, _1, _2, args...));
  }

  // Ionia standard fucntions
  bool IonPrint(ValueStack &vals, Value &ret);
  bool IonInput(ValueStack &vals, Value &ret);
  bool IonIf(ValueStack &vals, Value &ret);
  bool IonIs(ValueStack &vals, Value &ret);
  bool IonCalcOp(ValueStack &vals, Value &ret, Operator op);

  std::vector<std::uint8_t> rom_;
  // internal status
  std::uint32_t pc_;
  Value val_reg_;
  ValueStack vals_;
  std::stack<EnvPtr> envs_;
  EnvPtr root_, ext_;
  // tables
  SymbolTable sym_table_;
  FuncPCTable pc_table_;
  GlobalFuncTable global_funcs_;
  std::unordered_map<std::uint32_t, ExtFunc> ext_funcs_;
  // symbol error handler
  ErrorHandler sym_error_handler_;
};

}  // namespace ionia::vm

#endif  // IONIA_VM_VM_H_
