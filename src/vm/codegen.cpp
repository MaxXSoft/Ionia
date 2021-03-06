#include "vm/codegen.h"

#include <fstream>
#include <sstream>
#include <cstddef>
#include <cassert>

#include "version.h"
#include "util/cast.h"

using namespace ionia::vm;
using namespace ionia::util;

// definitions of static member variables
const std::uint32_t CodeGen::kFileHeader;
const std::uint32_t CodeGen::kMinFileSize;
const std::uint32_t CodeGen::kGFTItemSize;

int CodeGen::ParseBytecode(const std::vector<std::uint8_t> &buffer,
                           SymbolTable &sym_table, FuncPCTable &pc_table,
                           GlobalFuncTable &global_funcs) {
  std::size_t pos = 0;
  // check buffer size
  if (buffer.size() < kMinFileSize) return -1;
  // check file header
  auto magic_num = IntPtrCast<32>(buffer.data() + pos);
  if (*magic_num != kFileHeader) return -1;
  pos += 4;
  // check version info
  auto ver_info = IntPtrCast<32>(buffer.data() + pos);
  int major = (*ver_info >> 20) & 0xfff, minor = (*ver_info >> 12) & 0xff,
      patch = *ver_info & 0xfff;
  if (CompareVersion(major, minor, patch) > 0) return -1;
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
  GlobalFunc glob_func;
  global_funcs.clear();
  for (std::size_t i = 0; i < *global_len;) {
    // read function id
    auto func_id = IntPtrCast<32>(buffer.data() + pos + i);
    if (*func_id >= *sym_len) return -1;
    i += 4;
    // read function pc
    glob_func.pc_id = *IntPtrCast<32>(buffer.data() + pos + i);
    if (glob_func.pc_id >= *fpt_len) return -1;
    i += 4;
    // read argument count
    glob_func.arg_count = buffer[pos + i];
    i += 1;
    // insert to table
    global_funcs.insert({sym_table[*func_id], glob_func});
  }
  pos += *global_len;
  // return position
  return static_cast<int>(pos);
}

// TODO: optimize
std::uint32_t CodeGen::GetSymbolIndex(const std::string &name) {
  // search name in symbol table
  for (std::size_t i = 0; i < sym_table_.size(); ++i) {
    if (sym_table_[i] == name) return i;
  }
  // symbol not found, create new
  sym_table_.push_back(name);
  return sym_table_.size() - 1;
}

void CodeGen::PushInst(OpCode op, std::uint32_t opr) {
  Inst inst = {static_cast<std::uint32_t>(op), opr};
  auto ptr = IntPtrCast<8>(&inst);
  inst_buf_.push_back(ptr[0]);
  inst_buf_.push_back(ptr[1]);
  inst_buf_.push_back(ptr[2]);
  inst_buf_.push_back(ptr[3]);
  last_op_ = static_cast<OpCode>(inst.opcode);
}

void CodeGen::PushInst(OpCode op) {
  inst_buf_.push_back(*IntPtrCast<8>(&op));
  last_op_ = op;
}

std::uint32_t CodeGen::GetFuncId(const std::string &label) {
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

std::vector<std::uint8_t> CodeGen::GenerateBytecode() {
  std::ostringstream content;
  assert(unfilled_.empty());
  // generate file header
  content.write(PtrCast<char>(&kFileHeader), sizeof(kFileHeader));
  // generate version info
  std::uint32_t ver_info = ((APP_VERSION_MAJOR & 0xfff) << 20) |
                           ((APP_VERSION_MINOR & 0xff) << 12) |
                           (APP_VERSION_PATCH & 0xfff);
  content.write(PtrCast<char>(&ver_info), sizeof(ver_info));
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
  std::uint32_t gft_len = global_funcs_.size() * kGFTItemSize;
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
    content.write(PtrCast<char>(&func.arg_count), sizeof(func.arg_count));
    gft_len += sizeof(func.arg_count);
  }
  // write instructions
  content.write(PtrCast<char>(inst_buf_.data()), inst_buf_.size());
  // generate byte array
  auto s = content.str();
  return std::vector<std::uint8_t>(s.begin(), s.end());
}

void CodeGen::GenerateBytecodeFile(const std::string &file) {
  auto content = GenerateBytecode();
  std::ofstream ofs(file, std::ios::binary);
  ofs.write(reinterpret_cast<char *>(content.data()), content.size());
}

void CodeGen::Reset() {
  sym_table_.clear();
  pc_table_.clear();
  global_funcs_.clear();
  inst_buf_.clear();
  labels_.clear();
  unfilled_.clear();
  last_op_ = static_cast<OpCode>(0);
}

void CodeGen::GET(const std::string &name) {
  PushInst(OpCode::GET, GetSymbolIndex(name));
}

void CodeGen::SET(const std::string &name) {
  PushInst(OpCode::SET, GetSymbolIndex(name));
}

void CodeGen::FUN() {
  PushInst(OpCode::FUN);
}

void CodeGen::CNST(std::uint32_t num) {
  PushInst(OpCode::CNST, num);
}

void CodeGen::CNSH(std::uint32_t num) {
  PushInst(OpCode::CNSH, num);
}

void CodeGen::PUSH() {
  PushInst(OpCode::PUSH);
}

void CodeGen::POP() {
  PushInst(OpCode::POP);
}

void CodeGen::RET() {
  PushInst(OpCode::RET);
}

void CodeGen::CALL() {
  PushInst(OpCode::CALL);
}

void CodeGen::TCAL() {
  PushInst(OpCode::TCAL);
}

void CodeGen::LABEL(const std::string &label) {
  assert(labels_.find(label) == labels_.end());
  // check if label is unfilled
  auto it = unfilled_.find(label);
  if (it != unfilled_.end()) {
    // fill function pc
    pc_table_[it->second] = inst_buf_.size();
    labels_[label] = it->second;
    unfilled_.erase(it);
  }
  else {
    // create new function pc
    pc_table_.push_back(inst_buf_.size());
    labels_[label] = pc_table_.size() - 1;
  }
}

void CodeGen::GetFuncValue(const std::string &name) {
  SetConst(GetFuncId(name));
  FUN();
}

void CodeGen::DefineFunction(const std::string &name) {
  GetFuncValue(name);
  SET(name);
}

void CodeGen::SetConst(std::int32_t num) {
  CNST(num & VM_INST_IMM_MASK);
  auto hi = num & ~VM_INST_IMM_MASK;
  // check if needs to set high part of constant
  auto is_hi_zero = !hi && !(num & (1 << (VM_INST_OPR_WIDTH - 1)));
  auto is_hi_neg = hi == ~VM_INST_IMM_MASK;
  if (!is_hi_zero && !is_hi_neg) {
    CNSH((num & ~VM_INST_IMM_MASK) >> VM_INST_OPCODE_WIDTH);
  }
}

void CodeGen::GenReturn() {
  if (last_op_ == OpCode::CALL) {
    // modify opcode to TCAL
    auto inst = PtrCast<Inst>(inst_buf_.data() + inst_buf_.size() - 1);
    last_op_ = OpCode::TCAL;
    inst->opcode = static_cast<std::uint32_t>(last_op_);
  }
  else {
    // just generate RET
    RET();
  }
}

void CodeGen::SmartGet(const std::string &name) {
  if (last_op_ == OpCode::SET) {
    auto index = GetSymbolIndex(name);
    // check if last instruction has same index
    auto inst = PtrCast<Inst>(inst_buf_.data() + inst_buf_.size() - 4);
    if (inst->opr == index) return;
  }
  // just generate GET
  GET(name);
}

void CodeGen::RegisterGlobalFunction(const std::string &name,
                                     const std::string &label,
                                     std::uint8_t arg_count) {
  // get function id
  assert(!name.empty() && name[0] == '$');
  auto func_id = GetSymbolIndex(name);
  assert(global_funcs_.find(func_id) == global_funcs_.end());
  // initialize global func structure
  GlobalFunc func;
  // get function pc id
  func.pc_id = GetFuncId(label);
  // get argument count
  func.arg_count = arg_count;
  // insert into table
  global_funcs_.insert({func_id, func});
}
