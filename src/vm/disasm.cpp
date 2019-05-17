#include "vm/disasm.h"

#include <fstream>
#include <iomanip>

#include "vm/codegen.h"
#include "util/cast.h"

using namespace ionia::vm;
using namespace ionia::util;

namespace {

using OpCode = Inst::OpCode;

constexpr const char *kInstOpName[] = {
  VM_INST_ALL(VM_EXPAND_STR_ARRAY)
};

inline void PrintInstOpName(std::ostream &os, OpCode op) {
  os << std::setw(6) << std::left << kInstOpName[static_cast<int>(op)];
}

}  // namespace

void Disassembler::PrintGlobalFuncs(std::ostream &os) {
  if (global_funcs_.empty()) return;
  os << "Global Functions (GFT):" << std::endl;
  for (const auto &it : global_funcs_) {
    const auto &func = it.second;
    os << "  " << it.first << ", arg.size = ";
    os << static_cast<int>(func.arg_count) << std::endl;
  }
}

bool Disassembler::LoadBytecode(const std::string &file) {
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
  // parse tables
  auto pos = CodeGen::ParseBytecode(buffer, sym_table_, pc_table_,
                                    global_funcs_);
  if (pos < 0) return false;
  // copy bytecode segment
  rom_.reserve(buffer.size() - pos);
  for (int i = pos; i < buffer.size(); ++i) {
    rom_.push_back(buffer[i]);
  }
  // reset error counter & pc
  error_num_ = pc_ = 0;
  return true;
}

bool Disassembler::Disassemble(std::ostream &os) {
  // print GFT
  PrintGlobalFuncs(os);
  // print instructions
  using OpCode = Inst::OpCode;
  while (pc_ < rom_.size()) {
    // fetch next instruction
    auto inst = PtrCast<Inst>(rom_.data() + pc_);
    // check if there is a function at current pc
    for (int i = 0; i < pc_table_.size(); ++i) {
      if (pc_table_[i] == pc_) {
        os << std::endl << "fun" << i << ":" << std::endl;
      }
    }
    // print indent
    os << "  ";
    switch (inst->opcode) {
      case OpCode::GET: case OpCode::SET:
      case OpCode::CALL: case OpCode::TCAL: {
        PrintInstOpName(os, inst->opcode);
        if (inst->opr >= sym_table_.size()) {
          // invalid symbol id
          os << "INVALID";
          ++error_num_;
        }
        else {
          os << sym_table_[inst->opr];
        }
        pc_ += 4;
        break;
      }
      case OpCode::CNST: case OpCode::CNSH: {
        PrintInstOpName(os, inst->opcode);
        os << inst->opr;
        pc_ += 4;
        break;
      }
      case OpCode::FUN: case OpCode::RET:
      case OpCode::PUSH: case OpCode::POP: {
        PrintInstOpName(os, inst->opcode);
        // TODO: function mark
        pc_ += 1;
        break;
      }
      default: {
        os << "UNKNOWN";
        error_num_ += 1;
        pc_ += 1;
        break;
      }
    }
    // print line break
    os << std::endl;
  }
  return !error_num_;
}
