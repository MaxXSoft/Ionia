#include "vm/vm.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <utility>
#include <cassert>

#include "vm/codegen.h"
#include "util/cast.h"

using namespace ionia::vm;
using namespace ionia::util;

namespace {

// type of slot in VM environment
using SlotType = decltype(Env::slot);

}  // namespace

bool VM::PrintError(const char *message) {
  std::cerr << "[ERROR] " << message << ", pc = ";
  std::cerr << std::hex << std::setw(8) << std::setfill('0') << pc_;
  std::cerr << std::endl;
  return false;
}

bool VM::PrintError(const char *message, const char *symbol) {
  std::cerr << "[ERROR] symbol '" << symbol << "' ";
  std::cerr << message << ", pc = ";
  std::cerr << std::hex << std::setw(8) << std::setfill('0') << pc_;
  std::cerr << std::endl;
  return false;
}

void VM::InitExtFuncs() {
  // reset ext environment
  ext_ = MakeEnv();
  root_->outer = ext_;
  // try to set up all Ionia standard functions
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

bool VM::GetEnvValue(Inst *inst, Value &value) {
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
  // try to handle symbol error by calling symbol error handler
  auto str = sym_table_[inst->opr];
  if (sym_error_handler_ && sym_error_handler_(str, value)) return true;
  // value not found
  return PrintError("not found", str.c_str());
}

bool VM::DoCall(const Value &func) {
  // check if is not a function
  if (!func.env) return PrintError("calling a non-function");
  // check if is an external function
  auto it = ext_funcs_.find(func.value);
  if (it != ext_funcs_.end()) {
    // call external function
    envs_.push(MakeEnv());
    envs_.top()->ret_pc = pc_ + 1;
    if (!it->second(vals_, val_reg_)) {
      return PrintError("invalid function call");
    }
    pc_ = envs_.top()->ret_pc;
    envs_.pop();
  }
  else {
    // set up environment
    envs_.push(MakeEnv(func.env));
    envs_.top()->ret_pc = pc_ + 1;
    if (func.value >= pc_table_.size()) {
      return PrintError("invalid function pc");
    }
    pc_ = pc_table_[func.value];
  }
  return true;
}

bool VM::DoTailCall(const Value &func) {
  // check if is not a function
  if (!func.env) return PrintError("calling a non-function");
  // check if is an external function
  auto it = ext_funcs_.find(func.value);
  if (it != ext_funcs_.end()) {
    // call external function
    if (!it->second(vals_, val_reg_)) {
      return PrintError("invalid function call");
    }
    pc_ = envs_.top()->ret_pc;
    envs_.pop();
  }
  else {
    // TODO: if there is an infinite loop, system will run out of memory
    // switch environment
    auto env = MakeEnv(func.env);
    env->ret_pc = envs_.top()->ret_pc;
    envs_.top() = std::move(env);
    if (func.value >= pc_table_.size()) {
      return PrintError("invalid function pc");
    }
    pc_ = pc_table_[func.value];
  }
  return true;
}

bool VM::IonPrint(ValueStack &vals, Value &ret) {
  if (vals.size() < 1) return false;
  const auto &v = vals.top();
  if (v.env) {
    std::cout << "<function at: 0x";
    std::cout << std::hex << std::setw(8) << std::setfill('0');
    std::cout << v.value << ">" << std::endl;
  }
  else {
    std::cout << std::dec << v.value << std::endl;
  }
  ret = v;
  vals.pop();
  return true;
}

bool VM::IonInput(ValueStack &vals, Value &ret) {
  std::cin >> ret.value;
  ret.env = nullptr;
  return true;
}

bool VM::IonIf(ValueStack &vals, Value &ret) {
  if (vals.size() < 3) return false;
  // fetch false part
  auto else_then = vals.top();
  vals.pop();
  // fetch true part
  auto then = vals.top();
  vals.pop();
  // fetch condition
  if (vals.top().env) return false;
  auto cond = vals.top().value != 0;
  vals.pop();
  // tail call corresponding part
  auto result = DoTailCall(cond ? then : else_then);
  if (result) {
    auto env = MakeEnv();
    env->ret_pc = pc_;
    envs_.push(env);
  }
  return result;
}

bool VM::IonIs(ValueStack &vals, Value &ret) {
  if (vals.size() < 2) return false;
  // fetch arguments
  auto rhs = vals.top();
  vals.pop();
  auto lhs = vals.top();
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

bool VM::IonCalcOp(ValueStack &vals, Value &ret, Operator op) {
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
    std::swap(lhs, rhs);
  }
  // calculate
  switch (op) {
    case Operator::Equal: ret.value = lhs == rhs; break;
    case Operator::NotEqual: ret.value = lhs != rhs; break;
    case Operator::Less: ret.value = lhs < rhs; break;
    case Operator::LessEqual: ret.value = lhs <= rhs; break;
    case Operator::Great: ret.value = lhs > rhs; break;
    case Operator::GreatEqual: ret.value = lhs >= rhs; break;
    case Operator::Add: ret.value = lhs + rhs; break;
    case Operator::Sub: ret.value = lhs - rhs; break;
    case Operator::Mul: ret.value = lhs * rhs; break;
    case Operator::Div: ret.value = lhs / rhs; break;
    case Operator::Mod: ret.value = lhs % rhs; break;
    case Operator::And: ret.value = lhs & rhs; break;
    case Operator::Or: ret.value = lhs | rhs; break;
    case Operator::Not: ret.value = ~lhs; break;
    case Operator::Xor: ret.value = lhs ^ rhs; break;
    case Operator::Shl: ret.value = lhs << rhs; break;
    case Operator::Shr: ret.value = lhs >> rhs; break;
    case Operator::LogicAnd: ret.value = lhs && rhs; break;
    case Operator::LogicOr: ret.value = lhs || rhs; break;
    case Operator::LogicNot: ret.value = !lhs; break;
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
  auto pos = CodeGen::ParseBytecode(buffer, sym_table_, pc_table_,
                                    global_funcs_);
  if (pos < 0) return false;
  // copy bytecode segment
  rom_.reserve(buffer.size() - pos);
  for (int i = pos; i < buffer.size(); ++i) {
    rom_.push_back(buffer[i]);
  }
  // set up external functions (Ionia standard functions)
  InitExtFuncs();
  return true;
}

bool VM::RegisterFunction(const std::string &name, ExtFunc func) {
  Value ret;
  return RegisterFunction(name, func, ret);
}

bool VM::RegisterFunction(const std::string &name, ExtFunc func,
                          Value &ret) {
  for (std::size_t i = 0; i < sym_table_.size(); ++i) {
    if (sym_table_[i] == name) {
      // get new function pc id
      auto pc_id = pc_table_.size() + ext_funcs_.size();
      // add func to external function table
      ext_funcs_.insert({pc_id, func});
      // add func to ext environment
      ret = MakeValue(pc_id, ext_);
      ext_->slot.insert({i, ret});
      return true;
    }
  }
  return false;
}

bool VM::CallFunction(const std::string &name,
                      const std::vector<Value> &args, Value &ret) {
  // find function name in global function table
  auto it = global_funcs_.find("$" + name);
  if (it == global_funcs_.end()) return false;
  const auto &func = it->second;
  // set up new environment
  auto env = MakeEnv(root_);
  // set up arguments
  if (args.size() != func.arg_count) return false;
  for (const auto &i : args) vals_.push(i);
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
  root_ = MakeEnv(ext_);
  envs_.push(root_);
}

bool VM::Run() {
#define VM_NEXT(len)                                    \
  do {                                                  \
    pc_ += len;                                         \
    inst = PtrCast<Inst>(rom_.data() + pc_);            \
    goto *inst_labels[static_cast<int>(inst->opcode)];  \
  } while (0)

  Inst *inst;
  Value opr;
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

  // push value register into value stack
  VM_LABEL(PUSH) {
    vals_.push(val_reg_);
    VM_NEXT(1);
  }

  // pop value in value stack to value register
  VM_LABEL(POP) {
    if (vals_.empty()) {
      return PrintError("pop from empty stack");
    }
    else {
      val_reg_ = vals_.top();
      vals_.pop();
      VM_NEXT(1);
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
    if (!DoCall(val_reg_)) return false;
    VM_NEXT(0);
  }

  // tail call function and modify outer environment
  VM_LABEL(TCAL) {
    if (!DoTailCall(val_reg_)) return false;
    // return from root environment, exit from VM
    if (envs_.empty()) return true;
    VM_NEXT(0);
  }

#undef VM_NEXT
}
