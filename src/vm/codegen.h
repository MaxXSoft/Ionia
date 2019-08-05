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
                           SymbolTable &sym_table, FuncPCTable &pc_table,
                           GlobalFuncTable &global_funcs);

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
  void RET();
  void CALL();
  void TCAL();

  // create a new label
  void LABEL(const std::string &label);

  // get function value (pseudo instruction)
  void GetFuncValue(const std::string &name);
  // define function (pseudo instruction)
  void DefineFunction(const std::string &name);
  // set constant value (pseudo instruction)
  void SetConst(std::int32_t num);
  // automatic generate RET or TCAL instruction (pseudo instruction)
  void GenReturn();
  // generate GET only when it's necessary (pseudo instruction)
  void SmartGet(const std::string &name);
  // register new global function
  void RegisterGlobalFunction(const std::string &name,
                              const std::string &label,
                              std::uint8_t arg_count);

 private:
  // file header of Ionia VM's bytecode file (bad bite c -> bad byte code)
  static const std::uint32_t kFileHeader = 0xec17dbba;
  // minimum bytecode file size (magic, version, ST len, FPT len, GFT len)
  static const std::uint32_t kMinFileSize = 5 * 4;
  // size of global function table item
  static const std::uint32_t kGFTItemSize = 4 + 4 + 1;

  std::uint32_t GetSymbolIndex(const std::string &name);
  void PushInst(OpCode op, std::uint32_t opr);
  void PushInst(OpCode op);
  std::uint32_t GetFuncId(const std::string &label);

  // tables
  SymbolTable sym_table_;
  FuncPCTable pc_table_;
  std::map<std::uint32_t, GlobalFunc> global_funcs_;
  // buffer that stores instructions
  std::vector<std::uint8_t> inst_buf_;
  OpCode last_op_;
  // map of labels
  std::map<std::string, std::uint32_t> labels_, unfilled_;
};

}  // namespace ionia::vm

#endif  // IONIA_VM_BYTECODE_H_
