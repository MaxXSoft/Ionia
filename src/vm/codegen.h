#ifndef IONIA_VM_BYTECODE_H_
#define IONIA_VM_BYTECODE_H_

#include <vector>
#include <map>
#include <string>
#include <forward_list>
#include <cstdint>

#include "vm/define.h"

namespace ionia::vm {

// code generator of Ionia VM
class CodeGen {
 public:
  CodeGen() { Reset(); }
  virtual ~CodeGen() = default;

  // Function returns start position of bytecode segment.
  // If error, returns -1.
  static int ParseBytecode(const std::vector<std::uint8_t> &buffer,
                           VMSymbolTable &sym_table,
                           VMFuncPCTable &pc_table,
                           VMGlobalFuncTable &global_funcs);

  // generate bytecode vector
  std::vector<std::uint8_t> GenerateBytecode();
  // generate bytecode to file
  void GenerateBytecodeFile(const std::string &file);
  // reset generator
  void Reset();

  // generate instructions
  void GET(const std::string &name);
  void SET(const std::string &name);
  void FUN();
  void CNST(std::uint32_t num);
  void CNSH(std::uint32_t num);
  void PUSH();
  void POP();
  void SWAP();
  void RET();
  void CALL(const std::string &name);
  void TCAL(const std::string &name);

  // create a new named label
  void LABEL(const std::string &label);

  // get function value (pseudo instruction)
  void GetFuncValue(const std::string &name);
  // define function (pseudo instruction)
  void DefineFunction(const std::string &name);
  // set constant value (pseudo instruction)
  void SetConst(std::int32_t num);
  // automatic generate RET or TCAL instruction (pseudo instruction)
  void GenReturn();
  // register new global function
  void RegisterGlobalFunction(const std::string &name,
                              std::uint8_t arg_count);

 private:
  // file header of Ionia VM's bytecode file (bad bite c -> bad byte code)
  static const std::uint32_t kFileHeader = 0xec17dbba;
  // minimum bytecode file size
  static const std::uint32_t kMinFileSize = 4 * 4;
  // size of global function table item
  static const std::uint32_t kGFTItemSize = 4 + 4 + 1;

  std::uint32_t GetSymbolIndex(const std::string &name);
  void PushInst(const VMInst &inst);
  void PushInst(VMInst::OpCode op);
  std::uint32_t GetFuncId(const std::string &label);

  // tables
  VMSymbolTable sym_table_;
  VMFuncPCTable pc_table_;
  std::map<std::uint32_t, VMGlobalFunc> global_funcs_;
  // buffer that stores instructions
  std::vector<std::uint8_t> inst_buf_;
  VMInst::OpCode last_op_;
  // map of labels
  std::map<std::string, std::uint32_t> labels_, unfilled_;
};

}  // namespace ionia::vm

#endif  // IONIA_VM_BYTECODE_H_
