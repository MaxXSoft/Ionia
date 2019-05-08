#ifndef IONIA_VM_BYTECODE_H_
#define IONIA_VM_BYTECODE_H_

#include <vector>
#include <map>
#include <string>
#include <cstdint>

#include "vm/define.h"

class VMCodeGen {
 public:
  VMCodeGen() {}

  // Function returns start position of bytecode segment.
  // If error, returns -1.
  static int ParseBytecode(const std::vector<std::uint8_t> &buffer,
                           VMSymbolTable &sym_table,
                           VMGlobalFuncTable &global_funcs);
  std::vector<std::uint8_t> GenerateBytecode();
  void GenerateBytecodeFile(const std::string &file);

 private:
  // file header of Ionia VM's bytecode file (bad bite c -> bad byte code)
  static const std::uint32_t kFileHeader = 0xbadb17ec;
  // minimum bytecode file size
  static const std::uint32_t kMinFileSize = 3 * 4;

  // tables
  VMSymbolTable sym_table_;
};

#endif  // IONIA_VM_BYTECODE_H_
