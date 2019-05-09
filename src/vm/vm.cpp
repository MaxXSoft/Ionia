#include "vm/vm.h"

#include <iostream>
#include <iomanip>
#include <fstream>

#include "vm/codegen.h"
#include "util/cast.h"

namespace {

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
  auto str = inst->opr ? sym_table_[inst->opr - 1].c_str() : nullptr;
  auto cur_env = envs_.top();
  // recursively find value in environments
  while (cur_env) {
    auto it = cur_env->slot.find(str);
    if (it != cur_env->slot.end()) {
      value = it->second;
      return nullptr;
    }
    else {
      cur_env = cur_env->outer;
    }
  }
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
  // check if conflict
  for (const auto &i : sym_table_) {
    if (i == name) return false;
  }
  sym_table_.push_back(name);
  ext_funcs_.insert({name, func});
  return true;
}

bool VM::RegisterFunction(const std::string &name, ExtFunc func,
                          std::uint32_t &id) {
  // check if conflict
  for (const auto &i : sym_table_) {
    if (i == name) return false;
  }
  id = sym_table_.size();
  sym_table_.push_back(name);
  ext_funcs_.insert({name, func});
  return true;
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
    auto sym_ptr = sym_table_[func.args[i]].c_str();
    env->slot.insert({sym_ptr, args[i]});
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

bool VM::TailCallFunction(const std::string &name,
                          const std::vector<VMValue> &args, VMValue &ret) {
  // find function name in global function table
  auto it = global_funcs_.find("$" + name);
  if (it == global_funcs_.end()) return false;
  const auto &func = it->second;
  // check argument size
  if (args.size() != func.args.size()) return false;
  // create new environment and set up return PC
  auto env = MakeVMEnv(root_);
  env->ret_pc = envs_.top()->ret_pc;
  // backup environment stack and reset
  // since VM will automatically stop when executing RET instruction
  // and there is only one environment in environment stack
  envs_.pop();
  auto last_envs = envs_;
  while (!envs_.empty()) envs_.pop();
  // set up arguments
  for (int i = 0; i < args.size(); ++i) {
    auto sym_ptr = sym_table_[func.args[i]].c_str();
    env->slot.insert({sym_ptr, args[i]});
  }
  // tail call function
  envs_.push(env);
  pc_ = func.func_pc;
  auto result = Run();
  if (result) ret = val_reg_;
  // restore stack
  envs_ = last_envs;
  return result;
}

void VM::Reset() {
  pc_ = 0;
  val_reg_ = {0, nullptr};
  while (!envs_.empty()) envs_.pop();
  // create root environment
  root_ = MakeVMEnv();
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
    auto str = inst->opr ? sym_table_[inst->opr - 1].c_str() : nullptr;
    envs_.top()->slot[str] = val_reg_;
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
    VMValue func;
    if (auto str = GetEnvValue(inst, func)) {
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
    if (!func.env) return PrintError("calling a non-function");
    // set up environment
    envs_.top()->outer = func.env;
    envs_.top()->ret_pc = pc_ + 4;
    pc_ = func.value;
    VM_NEXT(0);
  }

  // tail call function and modify outer environment
  VM_LABEL(TCAL) {
    // get function object
    VMValue func;
    if (auto str = GetEnvValue(inst, func)) {
      auto it = ext_funcs_.find(str);
      if (it != ext_funcs_.end()) {
        // calling an external function
        auto slot = envs_.top()->slot;
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
    if (!func.env) return PrintError("calling a non-function");
    // set up environment
    auto slot = envs_.top()->slot;
    envs_.pop();
    envs_.top()->outer = func.env;
    envs_.top()->slot = slot;
    pc_ = func.value;
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

  // if value is true then branch
  VM_LABEL(IF) {
    if (val_reg_.value) {
      pc_ += static_cast<std::int32_t>(inst->opr);
      VM_NEXT(0);
    }
    else {
      VM_NEXT(4);
    }
  }

  VM_LABEL(IS) {
    // read operand from environment
    VMValue opr;
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
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value == opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(NEQ) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value != opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(LT) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value < opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(LEQ) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value <= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(GT) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value > opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(GEQ) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value >= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(ADD) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value += opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(SUB) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value -= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(MUL) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value *= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(DIV) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value /= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(MOD) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value %= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(AND) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value &= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(OR) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value |= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(NOT) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = ~opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(XOR) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value ^= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(SHL) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value <<= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(SHR) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value >>= opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(LAND) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value && opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(LOR) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = val_reg_.value || opr.value;
    VM_NEXT(4);
  }

  VM_LABEL(LNOT) {
    // get operand
    VMValue opr;
    if (val_reg_.env || GetEnvValue(inst, opr) || opr.env) {
      return PrintError("invalid argument");
    }
    // get result
    val_reg_.value = !opr.value;
    VM_NEXT(4);
  }

#undef VM_NEXT
}
