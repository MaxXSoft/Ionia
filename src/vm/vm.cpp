#include "vm/vm.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>

#include "vm/codegen.h"
#include "util/cast.h"

namespace {

// type of slot in VM environment
using SlotType = decltype(VMEnv::slot);

bool PrintError(const char *message) {
  std::cerr << "[ERROR] " << message << std::endl;
  return false;
}

bool PrintError(const char *message, const char *symbol) {
  std::cerr << "[ERROR] symbol '" << symbol << "' ";
  std::cerr << message << std::endl;
  return false;
}

}  // namespace

const char *VM::GetEnvValue(VMInst *inst, VMValue &value) {
  auto cur_env = envs_.top();
  // recursively find value in environments
  while (cur_env) {
    auto it = cur_env->slot.find(inst->opr);
    if (it != cur_env->slot.end()) {
      value = it->second;
      return nullptr;
    }
    else {
      cur_env = cur_env->outer;
    }
  }
  // value not found
  assert(inst->opr != 0);
  auto str = sym_table_[inst->opr - 1].c_str();
  return str;
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

bool VM::RegisterFunction(const std::string &name, ExtFunc func) {
  // check if exists
  for (const auto &i : sym_table_) {
    if (i == name) {
      ext_funcs_.insert({i.c_str(), func});
      return true;
    }
  }
  return false;
}

bool VM::CallFunction(const std::string &name,
                      const std::vector<VMValue> &args, VMValue &ret) {
  // find function name in global function table
  auto it = global_funcs_.find("$" + name);
  if (it == global_funcs_.end()) return false;
  const auto &func = it->second;
  // set up new environment
  auto env = MakeVMEnv(root_);
  // set up arguments
  if (args.size() != func.args.size()) return false;
  for (int i = 0; i < args.size(); ++i) {
    env->slot.insert({func.args[i], args[i]});
  }
  // backup environment stack and reset
  // since VM will automatically stop when executing RET instruction
  // and there is only one environment in environment stack
  auto last_envs = envs_;
  while (!envs_.empty()) envs_.pop();
  // call function
  envs_.push(env);
  pc_ = func.func_pc;
  auto result = Run();
  if (result) ret = val_reg_;
  // restore stack
  envs_ = last_envs;
  return result;
}

const VMValue *VM::GetValueFromEnv(const VMEnvPtr &env,
                                   const std::string &name) {
  // find name in symbol table
  for (std::uint32_t i = 0; i < sym_table_.size(); ++i) {
    if (sym_table_[i] == name) {
      auto it = env->slot.find(i + 1);
      return it == env->slot.end() ? nullptr : &it->second;
    }
  }
  return nullptr;
}

void VM::Reset() {
  pc_ = 0;
  val_reg_ = {0, nullptr};
  while (!envs_.empty()) envs_.pop();
  // create root environment if not exists
  if (!root_) {
    root_ = MakeVMEnv();
  }
  else {
    // reset root environment
    root_->outer = nullptr;
    root_->ret_pc = 0;
    root_->slot.clear();
  }
  envs_.push(root_);
}

bool VM::Run() {
#define VM_NEXT(len)                                    \
  do {                                                  \
    pc_ += len;                                         \
    inst = PtrCast<VMInst>(rom_.data() + pc_);          \
    goto *inst_labels[static_cast<int>(inst->opcode)];  \
  } while (0)

  VMInst *inst;
  VMValue opr;
  SlotType slot;
  const void *inst_labels[] = { VM_INST_ALL(VM_EXPAND_LABEL_LIST) };
  // fetch first instruction
  VM_NEXT(0);

  // get value of identifier from environment
  VM_LABEL(GET) {
    if (auto str = GetEnvValue(inst, val_reg_)) {
      return PrintError("not found", str);
    }
    else {
      VM_NEXT(4);
    }
  }

  // set value of identifier in current environment
  VM_LABEL(SET) {
    envs_.top()->slot[inst->opr] = val_reg_;
    VM_NEXT(4);
  }

  // set value register as a function
  VM_LABEL(FUN) {
    val_reg_.env = envs_.top();
    VM_NEXT(1);
  }

  // put constant number to value register
  VM_LABEL(CNST) {
    val_reg_.value = inst->opr;
    val_reg_.env = nullptr;
    VM_NEXT(4);
  }

  // return from function
  VM_LABEL(RET) {
    if (envs_.size() > 1) {
      pc_ = envs_.top()->ret_pc;
      envs_.pop();
    }
    else {
      // return from root environment, exit from VM
      return true;
    }
    VM_NEXT(0);
  }

  // create new environment and switch
  VM_LABEL(CENV) {
    envs_.push(MakeVMEnv(envs_.top()));
    VM_NEXT(1);
  }

  // call function and modify outer environment
  VM_LABEL(CALL) {
    // get function object
    if (auto str = GetEnvValue(inst, opr)) {
      auto it = ext_funcs_.find(str);
      if (it != ext_funcs_.end()) {
        // calling an external function
        envs_.top()->outer = nullptr;
        val_reg_ = it->second(envs_.top());
        envs_.pop();
        VM_NEXT(4);
      }
      else {
        // raise error
        return PrintError("not found", str);
      }
    }
    // check if is not a function
    if (!opr.env) return PrintError("calling a non-function");
    // set up environment
    envs_.top()->outer = opr.env;
    envs_.top()->ret_pc = pc_ + 4;
    pc_ = opr.value;
    VM_NEXT(0);
  }

  // tail call function and modify outer environment
  VM_LABEL(TCAL) {
    // get function object
    if (auto str = GetEnvValue(inst, opr)) {
      auto it = ext_funcs_.find(str);
      if (it != ext_funcs_.end()) {
        // calling an external function
        slot = envs_.top()->slot;
        envs_.pop();
        envs_.top()->outer = nullptr;
        envs_.top()->slot = slot;
        val_reg_ = it->second(envs_.top());
        // return from tail call
        pc_ = envs_.top()->ret_pc;
        envs_.pop();
        VM_NEXT(0);
      }
      else {
        // raise error
        return PrintError("not found", str);
      }
    }
    // check if is not a function
    if (!opr.env) return PrintError("calling a non-function");
    // set up environment
    slot = envs_.top()->slot;
    envs_.pop();
    envs_.top()->outer = opr.env;
    envs_.top()->slot = slot;
    pc_ = opr.value;
    VM_NEXT(0);
  }

  // write to console
  VM_LABEL(WRIT) {
    if (val_reg_.env) {
      std::cout << "<function at: ";
      std::cout << std::hex << val_reg_.value << ">" << std::endl;
    }
    else {
      std::cout << std::dec << val_reg_.value << std::endl;
    }
    VM_NEXT(1);
  }

  // read from console
  VM_LABEL(READ) {
    std::cin >> val_reg_.value;
    val_reg_.env = nullptr;
    VM_NEXT(1);
  }

  // if value is true then jump
  VM_LABEL(IF) {
    if (val_reg_.value) {
      pc_ = inst->opr;
      VM_NEXT(0);
    }
    else {
      VM_NEXT(4);
    }
  }

  VM_LABEL(IS) {
    // read operand from environment
    if (auto str = GetEnvValue(inst, opr)) {
      return PrintError("not found", str);
    }
    // check value
    if ((val_reg_.env && opr.env) || (!val_reg_.env && !opr.env)) {
      val_reg_.value = val_reg_.value == opr.value;
    }
    else {
      val_reg_.value = 0;
    }
    val_reg_.env = nullptr;
    VM_NEXT(4);
  }

  VM_LABEL(EQL) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value == opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(NEQ) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value != opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(LT) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value < opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(LEQ) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value <= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(GT) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value > opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(GEQ) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value >= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(ADD) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value += opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(SUB) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value -= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(MUL) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value *= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(DIV) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value /= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(MOD) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value %= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(AND) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value &= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(OR) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value |= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(NOT) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = ~opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(XOR) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value ^= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(SHL) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value <<= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(SHR) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value >>= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(LAND) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value && opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(LOR) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value || opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(LNOT) {
    // get operand
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = !opr.value;
    VM_NEXT(4);
  }

#undef VM_NEXT
}
