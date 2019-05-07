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
  // definition of external function
  using ExtFunc = std::function<VMValue(const VMEnvPtr &)>;

  VM() { Reset(); }

  bool LoadProgram(const std::string &file);
  bool LoadProgram(const std::vector<std::uint8_t> &buffer);

  // register an external function, returns function id
  std::uint32_t RegisterFunction(const std::string &name, ExtFunc func);
  // call a global function in vitrual machine
  bool CallFunction(const std::string &name,
                    const std::vector<VMValue> &args, VMValue &ret);

  // run current program
  bool Run();

 private:
  void Reset();

  std::vector<std::uint8_t> rom_;
  // internal status
  std::uint32_t pc_;
  VMValue val_reg_;
  std::stack<VMEnvPtr> envs_;
  // tables
  VMSymbolTable sym_table_;
  VMGlobalFuncTable global_funcs_;
  std::unordered_map<std::uint32_t, ExtFunc> ext_funcs_;
};

#endif  // IONIA_VM_VM_H_
