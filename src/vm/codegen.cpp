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

void VMCodeLabel::FillLabel() {
  if (!gen_) return;
  // get unfilled instructions
  auto it = gen_->unfilled_anon_.find(this);
  assert(it != gen_->unfilled_anon_.end());
  // traversal all instructions and fill label info
  for (const auto &i : it->second) {
    auto inst = PtrCast<VMInst>(gen_->inst_buf_.data() + i);
    inst->opr = offset_;
  }
  // erase record
  gen_->unfilled_anon_.erase(it);
}

void VMCodeLabel::SetLabel() {
  if (!gen_) return;
  offset_ = gen_->pc();
}

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
    // insert to table (skip 'temp' slot)
    global_funcs.insert({sym_table[*func_id - 1], glob_func});
    // reset
    glob_func.args.clear();
  }
  pos += *global_len;
  // return position
  return static_cast<int>(pos);
}

// TODO: optimize
std::uint32_t VMCodeGen::GetSymbolIndex(const std::string &name) {
  // empty name represents 'temp' slot
  if (name.empty()) return 0;
  // search name in symbol table
  for (int i = 0; i < sym_table_.size(); ++i) {
    if (sym_table_[i] == name) return i + 1;
  }
  // symbol not found, create new
  sym_table_.push_back(name);
  return sym_table_.size();
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

void VMCodeGen::PushLabelInst(VMInst::OpCode op,
                              const std::string &label) {
  // try to find in named labels
  auto it = named_labels_.find(label);
  if (it != named_labels_.end()) {
    PushInst({op, it->second});
    return;
  }
  // if label not found
  PushInst({op, 0});
  auto offset = pc() - 4;
  // create or modify record of unfilled map
  auto unfilled_it = unfilled_named_.find(label);
  if (unfilled_it == unfilled_named_.end()) {
    unfilled_named_.insert({label, {offset}});
  }
  else {
    unfilled_it->second.push_front(offset);
  }
}

void VMCodeGen::PushLabelInst(VMInst::OpCode op,
                              const VMCodeLabel &label) {
  PushInst({op, 0});
  auto offset = pc() - 4;
  // create or modify record of unfilled map
  auto it = unfilled_anon_.find(&label);
  if (it == unfilled_anon_.end()) {
    unfilled_anon_.insert({&label, {offset}});
  }
  else {
    it->second.push_front(offset);
  }
}

void VMCodeGen::FillNamedLabels() {
  for (const auto &it : unfilled_named_) {
    // get instruction list
    const auto &insts = it.second;
    // get label offset
    auto label_it = named_labels_.find(it.first);
    assert(label_it != named_labels_.end());
    auto offset = label_it->second;
    // fill all instructions
    for (const auto &i : insts) {
      auto inst = PtrCast<VMInst>(inst_buf_.data() + i);
      inst->opr = offset;
    }
  }
  unfilled_named_.clear();
}

std::vector<std::uint8_t> VMCodeGen::GenerateBytecode() {
  std::ostringstream content;
  // fill all of named labels
  FillNamedLabels();
  // generate file header
  content.write(PtrCast<char>(&kFileHeader), sizeof(kFileHeader));
  // generate symbol table
  std::uint32_t sym_len = 0;
  auto len_pos = content.tellp();
  content.write(PtrCast<char>(&sym_len), sizeof(sym_len));
  for (int i = 0; i < sym_table_.size(); ++i) {
    const auto &sym = sym_table_[i];
    content.write(sym.c_str(), sym.size() + 1);
    sym_len += sym.size() + 1;
  }
  // update symbol table length
  content.seekp(len_pos);
  content.write(PtrCast<char>(&sym_len), sizeof(sym_len));
  content.seekp(0, std::ios::end);
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
    content.write(PtrCast<char>(&func.func_pc), sizeof(func.func_pc));
    gft_len += sizeof(func.func_pc);
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

void VMCodeGen::CNST(const std::string &label) {
  PushLabelInst(InstOp::CNST, label);
}

void VMCodeGen::RET() {
  PushInst(InstOp::RET);
}

void VMCodeGen::CENV() {
  PushInst(InstOp::CENV);
}

void VMCodeGen::CALL(const std::string &name) {
  PushInst({InstOp::CALL, GetSymbolIndex(name)});
}

void VMCodeGen::TCAL(const std::string &name) {
  PushInst({InstOp::TCAL, GetSymbolIndex(name)});
}

void VMCodeGen::WRIT() {
  PushInst(InstOp::WRIT);
}

void VMCodeGen::READ() {
  PushInst(InstOp::READ);
}

void VMCodeGen::IF(const std::string &label) {
  PushLabelInst(InstOp::IF, label);
}

void VMCodeGen::IF(const VMCodeLabel &label) {
  PushLabelInst(InstOp::IF, label);
}

void VMCodeGen::IS(const std::string &name) {
  PushInst({InstOp::IS, GetSymbolIndex(name)});
}

void VMCodeGen::EQL(const std::string &name) {
  PushInst({InstOp::EQL, GetSymbolIndex(name)});
}

void VMCodeGen::NEQ(const std::string &name) {
  PushInst({InstOp::NEQ, GetSymbolIndex(name)});
}

void VMCodeGen::LT(const std::string &name) {
  PushInst({InstOp::LT, GetSymbolIndex(name)});
}

void VMCodeGen::LEQ(const std::string &name) {
  PushInst({InstOp::LEQ, GetSymbolIndex(name)});
}

void VMCodeGen::GT(const std::string &name) {
  PushInst({InstOp::GT, GetSymbolIndex(name)});
}

void VMCodeGen::GEQ(const std::string &name) {
  PushInst({InstOp::GEQ, GetSymbolIndex(name)});
}

void VMCodeGen::ADD(const std::string &name) {
  PushInst({InstOp::ADD, GetSymbolIndex(name)});
}

void VMCodeGen::SUB(const std::string &name) {
  PushInst({InstOp::SUB, GetSymbolIndex(name)});
}

void VMCodeGen::MUL(const std::string &name) {
  PushInst({InstOp::MUL, GetSymbolIndex(name)});
}

void VMCodeGen::DIV(const std::string &name) {
  PushInst({InstOp::DIV, GetSymbolIndex(name)});
}

void VMCodeGen::MOD(const std::string &name) {
  PushInst({InstOp::MOD, GetSymbolIndex(name)});
}

void VMCodeGen::AND(const std::string &name) {
  PushInst({InstOp::AND, GetSymbolIndex(name)});
}

void VMCodeGen::OR(const std::string &name) {
  PushInst({InstOp::OR, GetSymbolIndex(name)});
}

void VMCodeGen::NOT(const std::string &name) {
  PushInst({InstOp::NOT, GetSymbolIndex(name)});
}

void VMCodeGen::XOR(const std::string &name) {
  PushInst({InstOp::XOR, GetSymbolIndex(name)});
}

void VMCodeGen::SHL(const std::string &name) {
  PushInst({InstOp::SHL, GetSymbolIndex(name)});
}

void VMCodeGen::SHR(const std::string &name) {
  PushInst({InstOp::SHR, GetSymbolIndex(name)});
}

void VMCodeGen::LAND(const std::string &name) {
  PushInst({InstOp::LAND, GetSymbolIndex(name)});
}

void VMCodeGen::LOR(const std::string &name) {
  PushInst({InstOp::LOR, GetSymbolIndex(name)});
}

void VMCodeGen::LNOT(const std::string &name) {
  PushInst({InstOp::LNOT, GetSymbolIndex(name)});
}

void VMCodeGen::LABEL(const std::string &label) {
  assert(named_labels_.find(label) == named_labels_.end());
  named_labels_[label] = pc();
}

VMCodeLabel VMCodeGen::NewLabel() {
  return VMCodeLabel(this, pc());
}

void VMCodeGen::RegisterGlobalFunction(
    const std::string &name, const std::vector<std::string> &args) {
  // get function id
  assert(!name.empty() && name[0] == '$');
  auto func_id = GetSymbolIndex(name);
  assert(global_funcs_.find(func_id) == global_funcs_.end());
  // initialize global func structure
  VMGlobalFunc func;
  func.func_pc = pc();
  for (const auto &i : args) {
    auto arg_id = GetSymbolIndex(i);
    func.args.push_back(arg_id);
  }
  // insert into table
  global_funcs_.insert({func_id, func});
}
