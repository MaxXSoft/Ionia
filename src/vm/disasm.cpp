#include "vm/disasm.h"

#include <fstream>
#include <iomanip>

#include "vm/codegen.h"
#include "util/cast.h"

using namespace ionia::vm;
using namespace ionia::util;

namespace {

// splitter of each column
constexpr const char *kSplit = "    ";

constexpr const char *kInstOpName[] = {
  VM_INST_ALL(VM_EXPAND_STR_ARRAY)
};

inline void PrintInstOpName(std::ostream &os, OpCode op) {
  os << std::setw(6) << std::left << std::setfill(' ');
  os << kInstOpName[static_cast<int>(op)];
}

inline void PrintPC(std::ostream &os, unsigned int pc, bool print_split) {
  os << std::hex << std::setw(8) << std::setfill('0') << std::right << pc;
  if (print_split) os << kSplit;
}

inline void PrintRawBytecode(std::ostream &os, const Inst *inst,
                             bool is_short) {
  auto byte = PtrCast<std::uint8_t>(inst);
  if (is_short) {
    os << std::hex << std::setw(2) << std::setfill('0') << std::right;
    os << static_cast<int>(byte[0]) << "      ";
  }
  else {
    os << std::hex << std::setw(8) << std::setfill('0') << std::right;
    os << ((byte[0] << 24) | (byte[1] << 16) | (byte[2] << 8) | byte[3]);
  }
  os << kSplit;
}

}  // namespace

void Disassembler::PrintGlobalFuncs(std::ostream &os) {
  if (global_funcs_.empty()) return;
  os << "Global Functions (GFT):" << std::endl;
  for (const auto &it : global_funcs_) {
    const auto &func = it.second;
    os << "  " << it.first << ", arg.count = ";
    os << static_cast<int>(func.arg_count) << std::endl;
  }
  os << std::endl;
}

void Disassembler::PrintLabel(std::ostream &os) {
  // check if there is a function at current pc
  for (std::size_t i = 0; i < pc_table_.size(); ++i) {
    if (pc_table_[i] == pc_) {
      os << std::endl << GetLabelName(i) << ":" << std::endl;
    }
  }
}

std::string Disassembler::GetLabelName(std::size_t index) {
  for (const auto &it : global_funcs_) {
    if (it.second.pc_id == index) return it.first;
  }
  return "fun" + std::to_string(index);
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
  for (std::size_t i = pos; i < buffer.size(); ++i) {
    rom_.push_back(buffer[i]);
  }
  // reset error counter & pc
  error_num_ = pc_ = 0;
  last_const_ = -1;
  return true;
}

bool Disassembler::Disassemble(std::ostream &os) {
  // print GFT
  PrintGlobalFuncs(os);
  // print instructions
  while (pc_ < rom_.size()) {
    // fetch next instruction
    auto inst = PtrCast<Inst>(rom_.data() + pc_);
    PrintLabel(os);
    // print current pc
    PrintPC(os, pc_, true);
    // print instruction
    auto opcode = static_cast<OpCode>(inst->opcode);
    switch (opcode) {
      case OpCode::GET: case OpCode::SET: {
        PrintRawBytecode(os, inst, false);
        PrintInstOpName(os, opcode);
        if (inst->opr >= sym_table_.size()) {
          // invalid symbol id
          os << "INVALID";
          ++error_num_;
        }
        else {
          os << sym_table_[inst->opr];
        }
        last_const_ = -1;
        pc_ += 4;
        break;
      }
      case OpCode::CNST: case OpCode::CNSH: {
        PrintRawBytecode(os, inst, false);
        PrintInstOpName(os, opcode);
        os << std::dec << inst->opr;
        // record last constant
        if (opcode == OpCode::CNST) {
          last_const_ = inst->opr;
          if (inst->opr & (1 << (VM_INST_OPR_WIDTH - 1))) {
            last_const_ |= ~VM_INST_IMM_MASK;
          }
        }
        else {
          last_const_ |= inst->opr << VM_INST_OPCODE_WIDTH;
        }
        last_const_ &= 0xffffffff;
        pc_ += 4;
        break;
      }
      case OpCode::FUN: case OpCode::RET:
      case OpCode::PUSH: case OpCode::POP:
      case OpCode::CALL: case OpCode::TCAL: {
        PrintRawBytecode(os, inst, true);
        PrintInstOpName(os, opcode);
        // print function mark
        if (opcode == OpCode::FUN && last_const_ != -1) {
          if (static_cast<std::size_t>(last_const_) >= pc_table_.size()) {
            os << "(INVALID)";
            ++error_num_;
          }
          else {
            os << "(" << GetLabelName(last_const_) << " at ";
            PrintPC(os, pc_table_[last_const_], false);
            os << ")";
          }
        }
        last_const_ = -1;
        pc_ += 1;
        break;
      }
      default: {
        PrintRawBytecode(os, inst, true);
        os << "UNKNOWN";
        error_num_ += 1;
        last_const_ = -1;
        pc_ += 1;
        break;
      }
    }
    // print line break
    os << std::endl;
  }
  return !error_num_;
}
