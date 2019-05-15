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

void VM::InitExtFuncs() {
  // set up external functions
  BindExtFunc("<<<", &VM::IonPrint);
  BindExtFunc(">>>", &VM::IonInput);
  BindExtFunc("?", &VM::IonIf);
  BindExtFunc("is", &VM::IonIs);
  BindExtFunc("eq", &VM::IonCalcOp, Operator::Equal);
  BindExtFunc("neq", &VM::IonCalcOp, Operator::NotEqual);
  BindExtFunc("lt", &VM::IonCalcOp, Operator::Less);
  BindExtFunc("le", &VM::IonCalcOp, Operator::LessEqual);
  BindExtFunc("gt", &VM::IonCalcOp, Operator::Great);
  BindExtFunc("ge", &VM::IonCalcOp, Operator::GreatEqual);
  BindExtFunc("+", &VM::IonCalcOp, Operator::Add);
  BindExtFunc("-", &VM::IonCalcOp, Operator::Sub);
  BindExtFunc("*", &VM::IonCalcOp, Operator::Mul);
  BindExtFunc("/", &VM::IonCalcOp, Operator::Div);
  BindExtFunc("%", &VM::IonCalcOp, Operator::Mod);
  BindExtFunc("&", &VM::IonCalcOp, Operator::And);
  BindExtFunc("|", &VM::IonCalcOp, Operator::Or);
  BindExtFunc("~", &VM::IonCalcOp, Operator::Not);
  BindExtFunc("^", &VM::IonCalcOp, Operator::Xor);
  BindExtFunc("<<", &VM::IonCalcOp, Operator::Shl);
  BindExtFunc(">>", &VM::IonCalcOp, Operator::Shr);
  BindExtFunc("&&", &VM::IonCalcOp, Operator::LogicAnd);
  BindExtFunc("||", &VM::IonCalcOp, Operator::LogicOr);
  BindExtFunc("!", &VM::IonCalcOp, Operator::LogicNot);
}

bool VM::GetEnvValue(VMInst *inst, VMValue &value) {
  auto cur_env = envs_.top();
  // recursively find value in environments
  while (cur_env) {
    auto it = cur_env->slot.find(inst->opr);
    if (it != cur_env->slot.end()) {
      value = it->second;
      return true;
    }
    else {
      cur_env = cur_env->outer;
    }
  }
  // value not found
  auto str = sym_table_[inst->opr].c_str();
  return PrintError("not found", str);
}

bool VM::DoCall(const VMValue &func) {
  // check if is not a function
  if (!func.env) return PrintError("calling a non-function");
  // check if is an external function
  auto it = ext_funcs_.find(func.value);
  if (it != ext_funcs_.end()) {
    // call external function
    envs_.push(MakeVMEnv());
    envs_.top()->ret_pc = pc_ + 4;
    if (!it->second(vals_, val_reg_)) {
      return PrintError("invalid function call");
    }
    pc_ = envs_.top()->ret_pc;
    envs_.pop();
  }
  else {
    // set up environment
    envs_.push(MakeVMEnv(func.env));
    envs_.top()->ret_pc = pc_ + 4;
    if (func.value >= pc_table_.size()) {
      return PrintError("invalid function pc");
    }
    pc_ = pc_table_[func.value];
  }
  return true;
}

bool VM::DoTailCall(const VMValue &func) {
  // check if is not a function
  if (!func.env) return PrintError("calling a non-function");
  // check if is an external function
  auto it = ext_funcs_.find(func.value);
  if (it != ext_funcs_.end()) {
    // call external function
    envs_.top()->outer = nullptr;
    if (!it->second(vals_, val_reg_)) {
      return PrintError("invalid function call");
    }
    pc_ = envs_.top()->ret_pc;
    envs_.pop();
  }
  else {
    // switch environment
    envs_.top()->outer = func.env;
    if (func.value >= pc_table_.size()) {
      return PrintError("invalid function pc");
    }
    pc_ = pc_table_[func.value];
  }
  return true;
}

bool VM::IonPrint(ValueStack &vals, VMValue &ret) {
  if (vals.size() < 1) return false;
  const auto &v = vals.top();
  if (v.env) {
    std::cout << "<function at: ";
    std::cout << std::hex << v.value << ">" << std::endl;
  }
  else {
    std::cout << std::dec << v.value << std::endl;
  }
  ret = v;
  vals.pop();
  return true;
}

bool VM::IonInput(ValueStack &vals, VMValue &ret) {
  std::cin >> ret.value;
  ret.env = nullptr;
  return true;
}

bool VM::IonIf(ValueStack &vals, VMValue &ret) {
  if (vals.size() < 3) return false;
  // fetch condition
  if (vals.top().env) return false;
  auto cond = vals.top().value != 0;
  vals.pop();
  // fetch true part
  auto then = vals.top();
  vals.pop();
  // fetch false part
  auto else_then = vals.top();
  vals.pop();
  // tail call corresponding part
  return DoTailCall(cond ? then : else_then);
}

bool VM::IonIs(ValueStack &vals, VMValue &ret) {
  if (vals.size() < 2) return false;
  // fetch arguments
  auto lhs = vals.top();
  vals.pop();
  auto rhs = vals.top();
  vals.pop();
  // check if lhs and rhs are same
  if ((lhs.env && rhs.env) || (!lhs.env && !rhs.env)) {
    ret.value = lhs.value == rhs.value;
  }
  else {
    ret.value = 0;
  }
  ret.env = nullptr;
  return true;
}

bool VM::IonCalcOp(ValueStack &vals, VMValue &ret, Operator op) {
  std::int32_t lhs, rhs;
  // fetch lhs
  if (vals.top().env) return false;
  lhs = vals.top().value;
  vals.pop();
  // fetch rhs
  if (op != Operator::Not && op != Operator::LogicNot) {
    if (vals.top().env) return false;
    rhs = vals.top().value;
    vals.pop();
  }
  // calculate
  switch (op) {
    case Operator::Equal: ret.value = lhs == rhs;
    case Operator::NotEqual: ret.value = lhs != rhs;
    case Operator::Less: ret.value = lhs < rhs;
    case Operator::LessEqual: ret.value = lhs <= rhs;
    case Operator::Great: ret.value = lhs > rhs;
    case Operator::GreatEqual: ret.value = lhs >= rhs;
    case Operator::Add: ret.value = lhs + rhs;
    case Operator::Sub: ret.value = lhs - rhs;
    case Operator::Mul: ret.value = lhs * rhs;
    case Operator::Div: ret.value = lhs / rhs;
    case Operator::Mod: ret.value = lhs % rhs;
    case Operator::And: ret.value = lhs & rhs;
    case Operator::Or: ret.value = lhs | rhs;
    case Operator::Not: ret.value = ~lhs;
    case Operator::Xor: ret.value = lhs ^ rhs;
    case Operator::Shl: ret.value = lhs << rhs;
    case Operator::Shr: ret.value = lhs >> rhs;
    case Operator::LogicAnd: ret.value = lhs && rhs;
    case Operator::LogicOr: ret.value = lhs || rhs;
    case Operator::LogicNot: ret.value = !lhs;
    default: assert(false);
  }
  // return to VM
  ret.env = nullptr;
  return true;
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
  auto pos = VMCodeGen::ParseBytecode(buffer, sym_table_, pc_table_,
                                      global_funcs_);
  if (pos < 0) return false;
  // reset ext environment
  ext_ = MakeVMEnv();
  // copy bytecode segment
  rom_.reserve(buffer.size() - pos);
  for (int i = pos; i < buffer.size(); ++i) {
    rom_.push_back(buffer[i]);
  }
  return true;
}

bool VM::RegisterFunction(const std::string &name, ExtFunc func) {
  VMValue ret;
  return RegisterFunction(name, func, ret);
}

bool VM::RegisterFunction(const std::string &name, ExtFunc func,
                          VMValue &ret) {
  for (std::size_t i = 0; i < sym_table_.size(); ++i) {
    if (sym_table_[i] == name) {
      // get new function pc id
      auto pc_id = pc_table_.size() + ext_funcs_.size();
      // add func to external function table
      ext_funcs_.insert({pc_id, func});
      // add func to ext environment
      ret = MakeVMValue(pc_id, ext_);
      ext_->slot.insert({i, ret});
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
  pc_ = pc_table_[func.pc_id];
  auto result = Run();
  if (result) ret = val_reg_;
  // restore stack
  envs_ = last_envs;
  return result;
}

void VM::Reset() {
  pc_ = 0;
  val_reg_ = {0, nullptr};
  // clear stacks
  while (!vals_.empty()) vals_.pop();
  while (!envs_.empty()) envs_.pop();
  // create root environment
  root_ = MakeVMEnv(ext_);
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
    if (!GetEnvValue(inst, val_reg_)) return false;
    VM_NEXT(4);
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
    // sign extend
    if (inst->opr & (1 << (VM_INST_OPR_WIDTH - 1))) {
      val_reg_.value |= ~VM_INST_IMM_MASK;
    }
    val_reg_.env = nullptr;
    VM_NEXT(4);
  }

  // set constant number as higher part of value register
  VM_LABEL(CNSH) {
    val_reg_.value |= inst->opr << VM_INST_OPCODE_WIDTH;
    val_reg_.env = nullptr;
    VM_NEXT(4);
  }

  // push slot value into value stack
  VM_LABEL(PUSH) {
    if (!GetEnvValue(inst, opr)) return false;
    vals_.push(opr);
    VM_NEXT(4);
  }

  // pop value in value stack to slot
  VM_LABEL(POP) {
    if (vals_.empty()) {
      return PrintError("pop from empty stack");
    }
    else {
      envs_.top()->slot[inst->opr] = vals_.top();
      vals_.pop();
      VM_NEXT(4);
    }
  }

  // return from function
  VM_LABEL(RET) {
    if (envs_.size() > 1) {
      pc_ = envs_.top()->ret_pc;
      envs_.pop();
      VM_NEXT(0);
    }
    else {
      // return from root environment, exit from VM
      return true;
    }
  }

  // call function and create new environment
  VM_LABEL(CALL) {
    // get function object
    if (!GetEnvValue(inst, opr)) return false;
    if (!DoCall(opr)) return false;
    VM_NEXT(0);
  }

  // tail call function and modify outer environment
  VM_LABEL(TCAL) {
    // get function object
    if (!GetEnvValue(inst, opr)) return false;
    if (!DoTailCall(opr)) return false;
    VM_NEXT(0);
  }

#undef VM_NEXT
}
