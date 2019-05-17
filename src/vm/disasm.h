#ifndef IONIA_VM_DISASM_H_
#define IONIA_VM_DISASM_H_

#include <string>
#include <ostream>
#include <vector>
#include <cstdint>

#include "vm/define.h"

namespace ionia::vm {

class Disassembler {
 public:
  Disassembler() : error_num_(0) {}

  // load bytecode file to buffer
  bool LoadBytecode(const std::string &file);
  // disassemble loaded bytecode
  bool Disassemble(std::ostream &os);

  // error count
  unsigned int error_num() const { return error_num_; }

 private:
  // print global function table
  void PrintGlobalFuncs(std::ostream &os);

  std::vector<std::uint8_t> rom_;
  unsigned int error_num_, pc_;
  // tables
  SymbolTable sym_table_;
  FuncPCTable pc_table_;
  GlobalFuncTable global_funcs_;
};

}  // namespace ionia::vm

#endif  // IONIA_VM_DISASM_H_
