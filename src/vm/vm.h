#ifndef IONIA_VM_VM_H_
#define IONIA_VM_VM_H_

#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <cstdint>

#include "vm/define.h"

class VM {
 public:
  // definition of value stack
  using ValueStack = std::stack<VMValue>;
  // definition of external function
  using ExtFunc = std::function<bool(ValueStack &, VMValue &)>;

  VM() { Reset(); }

  bool LoadProgram(const std::string &file);
  bool LoadProgram(const std::vector<std::uint8_t> &buffer);

  // register an external function
  bool RegisterFunction(const std::string &name, ExtFunc func);
  // register an external function
  // if success, return function value
  bool RegisterFunction(const std::string &name, ExtFunc func,
                        VMValue &ret);
  // call a global function in vitrual machine
  bool CallFunction(const std::string &name,
                    const std::vector<VMValue> &args, VMValue &ret);

  // reset VM's status (except symbol table, FPT, GFT and EFT)
  void Reset();
  // run current program
  bool Run();

 private:
  // supported operators by Ionia VM
  enum class Operator {
    Equal, NotEqual, Less, LessEqual, Great, GreatEqual,
    Add, Sub, Mul, Div, Mod,
    And, Or, Not, Xor, Shl, Shr,
    LogicAnd, LogicOr, LogicNot,
  };

  // initialize external function table
  // add all Ionia standard functions, like 'is', '?', 'eq', '+'...
  void InitExtFuncs();
  // get value from current environment, return false if not found
  bool GetEnvValue(VMInst *inst, VMValue &value);

  // call a VM function
  bool DoCall(const VMValue &func);
  // tail call a VM function
  bool DoTailCall(const VMValue &func);

  // bind member functions to external function
  template <typename Func, typename... Args>
  void BindExtFunc(const std::string &name, Func func, Args... args) {
    using namespace std::placeholders;
    ext_funcs_[name] = std::bind(func, this, _1, _2, args);
  }

  // Ionia standard fucntions
  bool IonPrint(ValueStack &vals, VMValue &ret);
  bool IonInput(ValueStack &vals, VMValue &ret);
  bool IonIf(ValueStack &vals, VMValue &ret);
  bool IonIs(ValueStack &vals, VMValue &ret);
  bool IonCalcOp(ValueStack &vals, VMValue &ret, Operator op);

  std::vector<std::uint8_t> rom_;
  // internal status
  std::uint32_t pc_;
  VMValue val_reg_;
  std::stack<VMValue> vals_;
  std::stack<VMEnvPtr> envs_;
  VMEnvPtr root_, ext_;
  // tables
  VMSymbolTable sym_table_;
  VMFuncPCTable pc_table_;
  VMGlobalFuncTable global_funcs_;
  std::unordered_map<std::uint32_t, ExtFunc> ext_funcs_;
};

#endif  // IONIA_VM_VM_H_
