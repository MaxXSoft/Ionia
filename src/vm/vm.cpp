#include "vm/vm.h"

#include <fstream>

#include "vm/codegen.h"

void VM::Reset() {
  pc_ = 0;
  val_reg_ = {0, nullptr};
  while (!envs_.empty()) envs_.pop();
  // create root environment
  envs_.push(MakeVMEnv());
}

bool VM::LoadProgram(const std::string &file) {
  // open file
  std::ifstream ifs(file, std::ios::binary);
  if (!ifs.is_open()) return false;
  // read bytes
  std::vector<std::uint8_t> buffer;
  unsigned cur_byte = ifs.get();
  while (cur_byte != std::char_traits<unsigned char>::eof()) {
    buffer.push_back(cur_byte);
    cur_byte = ifs.get();
  }
  return LoadProgram(buffer);
}

bool VM::LoadProgram(const std::vector<std::uint8_t> &buffer) {
  auto pos = VMCodeGen::ParseBytecode(buffer, sym_table_, global_funcs_);
  if (pos < 0) return false;
  // copy bytecode segment
  rom_.reserve(buffer.size() - pos);
  for (int i = pos; i < buffer.size(); ++i) {
    rom_.push_back(buffer[i]);
  }
  return true;
}

std::uint32_t VM::RegisterFunction(const std::string &name, ExtFunc func) {
  auto id = sym_table_.size();
  sym_table_.push_back(name);
  ext_funcs_.insert({id, func});
  return id;
}

bool VM::CallFunction(const std::string &name,
                      const std::vector<VMValue> &args, VMValue &ret) {
  // TODO
}

bool Run() {
  //
}
