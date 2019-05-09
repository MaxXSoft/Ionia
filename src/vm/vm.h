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

  // register an external function
  bool RegisterFunction(const std::string &name, ExtFunc func);
  // register an external function, returns function id
  bool RegisterFunction(const std::string &name, ExtFunc func,
                        std::uint32_t &id);
  // call a global function in vitrual machine
  bool CallFunction(const std::string &name,
                    const std::vector<VMValue> &args, VMValue &ret);

  // reset VM
  void Reset();
  // run current program
  bool Run();

 private:
  // Get value from current environment.
  // Returns symbol name if not found, otherwise returns nullptr.
  inline const char *GetEnvValue(VMInst *inst, VMValue &value);

  std::vector<std::uint8_t> rom_;
  // internal status
  std::uint32_t pc_;
  VMValue val_reg_;
  std::stack<VMEnvPtr> envs_;
  VMEnvPtr root_;
  // tables
  VMSymbolTable sym_table_;
  VMGlobalFuncTable global_funcs_;
  std::unordered_map<std::string, ExtFunc> ext_funcs_;
};

#endif  // IONIA_VM_VM_H_
