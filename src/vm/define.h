#ifndef IONIA_VM_DEFINE_H_
#define IONIA_VM_DEFINE_H_

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <cstdint>
#include <cassert>

// all supported instructions of Ionia VM
#define VM_INST_ALL(f)                  \
  f(GET) f(SET) f(FUN) f(CNST) f(CNSH)  \
  f(PUSH) f(POP) f(RET) f(CALL) f(TCAL)
// expand macro to comma-separated list
#define VM_EXPAND_LIST(i)         i,
// expand macro to comma-separated string array
#define VM_EXPAND_STR_ARRAY(i)    #i,
// expand macro to label list
#define VM_EXPAND_LABEL_LIST(i)   &&VML_##i,
// define a label of VM threading
#define VM_LABEL(l)               VML_##l:
// width of opcode field in Inst
#define VM_INST_OPCODE_WIDTH      4
// width of oprand field in Inst
#define VM_INST_OPR_WIDTH         (32 - VM_INST_OPCODE_WIDTH)
// immediate number mask of Inst
#define VM_INST_IMM_MASK          ((1 << VM_INST_OPR_WIDTH) - 1)

namespace ionia::vm {

// enumeration of opcode
enum class OpCode : std::uint32_t {
  VM_INST_ALL(VM_EXPAND_LIST)
};

// structure of instruction
struct Inst {
  // opcode
  std::uint32_t opcode : VM_INST_OPCODE_WIDTH;
  // operand
  std::uint32_t opr : VM_INST_OPR_WIDTH;
};

// forward declaration of Env
struct Env;
using EnvPtr = std::shared_ptr<Env>;

struct Value {
  std::int32_t value;
  // when env is nullptr, the value is integer
  EnvPtr env;
};

struct Env {
  std::unordered_map<std::uint32_t, Value> slot;
  EnvPtr outer;
  std::uint32_t ret_pc;
};

struct GlobalFunc {
  std::uint32_t pc_id;
  std::uint8_t arg_count;
};

// definition of tables
using SymbolTable = std::vector<std::string>;
using FuncPCTable = std::vector<std::uint32_t>;
using GlobalFuncTable = std::unordered_map<std::string, GlobalFunc>;

// make new VM environment
inline EnvPtr MakeEnv() {
  return std::make_shared<Env>(Env({{}, nullptr, 0}));
}

// make new VM environment with specific outer environment
inline EnvPtr MakeEnv(const EnvPtr &outer) {
  return std::make_shared<Env>(Env({{}, outer, 0}));
}

// make new VM integer value
inline Value MakeValue(std::int32_t value) {
  return {value, nullptr};
}

// make new VM function value
inline Value MakeValue(std::int32_t pc_id, const EnvPtr &env) {
  assert(env != nullptr);
  return {pc_id, env};
}

}  // namespace ionia::vm

#endif  // IONIA_VM_DEFINE_H_
