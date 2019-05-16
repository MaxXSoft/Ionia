#include "vm/codegen.h"

#include <fstream>
#include <sstream>
#include <cstddef>
#include <cassert>

#include "util/cast.h"

namespace {

using InstOp = VMInst::OpCode;

}  // namespace

// definitions of static member variables
const std::uint32_t VMCodeGen::kFileHeader;
const std::uint32_t VMCodeGen::kMinFileSize;

int VMCodeGen::ParseBytecode(const std::vector<std::uint8_t> &buffer,
                             VMSymbolTable &sym_table,
                             VMFuncPCTable &pc_table,
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
  // read function pc table length
  auto fpt_len = IntPtrCast<32>(buffer.data() + pos);
  pos += 4;
  // read function pc table
  pc_table.clear();
  for (std::size_t i = 0; i < *fpt_len; i += 4) {
    auto func_pc = IntPtrCast<32>(buffer.data() + pos + i);
    pc_table.push_back(*func_pc);
  }
  pos += *fpt_len;
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
    glob_func.pc_id = *IntPtrCast<32>(buffer.data() + pos + i);
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

// TODO: optimize
std::uint32_t VMCodeGen::GetSymbolIndex(const std::string &name) {
  // search name in symbol table
  for (int i = 0; i < sym_table_.size(); ++i) {
    if (sym_table_[i] == name) return i;
  }
  // symbol not found, create new
  sym_table_.push_back(name);
  return sym_table_.size() - 1;
}

void VMCodeGen::PushInst(const VMInst &inst) {
  auto ptr = IntPtrCast<8>(&inst);
  inst_buf_.push_back(ptr[0]);
  inst_buf_.push_back(ptr[1]);
  inst_buf_.push_back(ptr[2]);
  inst_buf_.push_back(ptr[3]);
}

void VMCodeGen::PushInst(VMInst::OpCode op) {
  inst_buf_.push_back(*IntPtrCast<8>(&op));
}

std::uint32_t VMCodeGen::GetFuncId(const std::string &label) {
  // try to find in named labels
  auto it = labels_.find(label);
  if (it != labels_.end()) return it->second;
  // check unfilled map
  it = unfilled_.find(label);
  if (it == unfilled_.end()) {
    // add label to unfilled map
    pc_table_.push_back(0);
    auto pc_id = pc_table_.size() - 1;
    unfilled_.insert({label, pc_id});
    return pc_id;
  }
  else {
    return it->second;
  }
}

std::vector<std::uint8_t> VMCodeGen::GenerateBytecode() {
  std::ostringstream content;
  assert(unfilled_.empty());
  // generate file header
  content.write(PtrCast<char>(&kFileHeader), sizeof(kFileHeader));
  // generate symbol table
  std::uint32_t sym_len = 0;
  auto len_pos = content.tellp();
  content.write(PtrCast<char>(&sym_len), sizeof(sym_len));
  for (const auto &i : sym_table_) {
    content.write(i.c_str(), i.size() + 1);
    sym_len += i.size() + 1;
  }
  // update symbol table length
  content.seekp(len_pos);
  content.write(PtrCast<char>(&sym_len), sizeof(sym_len));
  content.seekp(0, std::ios::end);
  // generate function pc table
  std::uint32_t fpt_len = pc_table_.size() * sizeof(std::uint32_t);
  content.write(PtrCast<char>(&fpt_len), sizeof(fpt_len));
  for (const auto &i : pc_table_) {
    content.write(PtrCast<char>(&i), sizeof(i));
  }
  // generate global function table
  std::uint32_t gft_len = 0;
  len_pos = content.tellp();
  content.write(PtrCast<char>(&gft_len), sizeof(gft_len));
  for (const auto &it : global_funcs_) {
    const auto &func = it.second;
    // write function id
    content.write(PtrCast<char>(&it.first), sizeof(it.first));
    gft_len += sizeof(it.first);
    // write function pc
    content.write(PtrCast<char>(&func.pc_id), sizeof(func.pc_id));
    gft_len += sizeof(func.pc_id);
    // write argument count
    std::uint8_t argc = func.args.size();
    content.write(PtrCast<char>(&argc), sizeof(argc));
    gft_len += sizeof(argc);
    // write argument id
    for (const auto &i : func.args) {
      content.write(PtrCast<char>(&i), sizeof(i));
      gft_len += sizeof(i);
    }
  }
  // update global function table length
  content.seekp(len_pos);
  content.write(PtrCast<char>(&gft_len), sizeof(gft_len));
  content.seekp(0, std::ios::end);
  // write instructions
  content.write(PtrCast<char>(inst_buf_.data()), inst_buf_.size());
  // generate byte array
  auto s = content.str();
  return std::vector<std::uint8_t>(s.begin(), s.end());
}

void VMCodeGen::GenerateBytecodeFile(const std::string &file) {
  auto content = GenerateBytecode();
  std::ofstream ofs(file, std::ios::binary);
  ofs.write(reinterpret_cast<char *>(content.data()), content.size());
}

void VMCodeGen::GET(const std::string &name) {
  PushInst({InstOp::GET, GetSymbolIndex(name)});
}

void VMCodeGen::SET(const std::string &name) {
  PushInst({InstOp::SET, GetSymbolIndex(name)});
}

void VMCodeGen::FUN() {
  PushInst(InstOp::FUN);
}

void VMCodeGen::CNST(std::uint32_t num) {
  PushInst({InstOp::CNST, num});
}

void VMCodeGen::CNSH(std::uint32_t num) {
  PushInst({InstOp::CNSH, num});
}

void VMCodeGen::PUSH() {
  PushInst(InstOp::PUSH);
}

void VMCodeGen::POP() {
  PushInst(InstOp::POP);
}

void VMCodeGen::SWAP() {
  PushInst(InstOp::SWAP);
}

void VMCodeGen::RET() {
  PushInst(InstOp::RET);
}

void VMCodeGen::CALL(const std::string &name) {
  PushInst({InstOp::CALL, GetSymbolIndex(name)});
}

void VMCodeGen::TCAL(const std::string &name) {
  PushInst({InstOp::TCAL, GetSymbolIndex(name)});
}

void VMCodeGen::LABEL(const std::string &label) {
  assert(labels_.find(label) == labels_.end());
  // check if label is unfilled
  auto it = unfilled_.find(label);
  if (it != unfilled_.end()) {
    // fill function pc
    pc_table_[it->second] = pc();
    labels_[label] = it->second;
    unfilled_.erase(it);
  }
  else {
    // create new function pc
    pc_table_.push_back(pc());
    labels_[label] = pc_table_.size() - 1;
  }
}

void VMCodeGen::GetFuncValue(const std::string &name) {
  SetConst(GetFuncId(name));
  FUN();
}

void VMCodeGen::DefineFunction(const std::string &name) {
  GetFuncValue(name);
  SET(name);
}

void VMCodeGen::SetConst(std::int32_t num) {
  CNST(num & VM_INST_IMM_MASK);
  auto hi = num & ~VM_INST_IMM_MASK;
  if (hi && hi != ~VM_INST_IMM_MASK) {
    CNSH((num & ~VM_INST_IMM_MASK) >> VM_INST_OPCODE_WIDTH);
  }
}

void VMCodeGen::RegisterGlobalFunction(
    const std::string &name, const std::vector<std::string> &args) {
  // get function id
  assert(!name.empty() && name[0] == '$');
  auto func_id = GetSymbolIndex(name);
  assert(global_funcs_.find(func_id) == global_funcs_.end());
  // initialize global func structure
  VMGlobalFunc func;
  // get function pc id
  func.pc_id = GetFuncId(name);
  // get argument id list
  for (const auto &i : args) {
    auto arg_id = GetSymbolIndex(i);
    func.args.push_back(arg_id);
  }
  // insert into table
  global_funcs_.insert({func_id, func});
}
