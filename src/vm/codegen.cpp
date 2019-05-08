#include "vm/codegen.h"

#include <cstddef>

#include "util/cast.h"

int VMCodeGen::ParseBytecode(const std::vector<std::uint8_t> &buffer,
                             VMSymbolTable &sym_table,
                             VMGlobalFuncTable &global_funcs) {
  std::size_t pos = 0;
  // check buffer size
  if (buffer.size() < kMinFileSize) return -1;
  // check file header
  auto magic_num = IntPtrCast<32>(buffer.data() + pos);
  if (*magic_num != kFileHeader) return -1;
  pos += 4;
  // read symbol table length
  auto sym_len = IntPtrCast<32>(buffer.data() + pos);
  pos += 4;
  // read symbol table
  std::string symbol;
  sym_table.clear();
  for (std::size_t i = 0; i < *sym_len; ++i) {
    if (buffer[pos + i] == '\0') {
      sym_table.push_back(symbol);
      symbol.clear();
    }
    else {
      symbol.push_back(buffer[pos + i]);
    }
  }
  pos += *sym_len;
  // read global function table length
  auto global_len = IntPtrCast<32>(buffer.data() + pos);
  pos += 4;
  // read global function table
  VMGlobalFunc glob_func;
  global_funcs.clear();
  for (std::size_t i = 0; i < *global_len;) {
    // read function id
    auto func_id = IntPtrCast<32>(buffer.data() + pos + i);
    i += 4;
    // read function pc
    glob_func.func_pc = *IntPtrCast<32>(buffer.data() + pos + i);
    i += 4;
    // read argument count
    auto arg_count = buffer[pos + i];
    i += 1;
    // read argument id
    for (std::size_t j = 0; j < arg_count; ++j) {
      glob_func.args.push_back(*IntPtrCast<32>(buffer.data() + pos + i));
      i += 4;
    }
    // insert to table
    global_funcs.insert({sym_table[*func_id], glob_func});
    // reset
    glob_func.args.clear();
  }
  pos += *global_len;
  // return position
  return static_cast<int>(pos);
}
