#ifndef IONIA_VM_DEFINE_H_
#define IONIA_VM_DEFINE_H_

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <cstdint>

// all supported instructions of Ionia VM
#define VM_INST_ALL(f)                      \
  f(GET) f(SET) f(FUN) f(CNST)              \
  f(RET) f(CENV) f(CALL) f(TCAL)
// expand macro to comma-separated list
#define VM_EXPAND_LIST(i)         i,
// expand macro to label list
#define VM_EXPAND_LABEL_LIST(i)   &&VML_##i,
// define a label of VM threading
#define VM_LABEL(l)               VML_##l:
// width of opcode field in VMInst
#define VM_INST_OPCODE_WIDTH      5
// width of oprand field in VMInst
#define VM_INST_OPR_WIDTH         (32 - VM_INST_OPCODE_WIDTH)
// immediate number mask of VMInst
#define VM_INST_IMM_MASK          ((1 << VM_INST_OPR_WIDTH) - 1)

struct VMInst {
  // opcode
  enum class OpCode : std::uint32_t {
    VM_INST_ALL(VM_EXPAND_LIST)
  } opcode : VM_INST_OPCODE_WIDTH;
  // operand
  std::uint32_t opr : VM_INST_OPR_WIDTH;
};

// forward declaration of VMEnv
struct VMEnv;
using VMEnvPtr = std::shared_ptr<VMEnv>;

struct VMValue {
  std::int32_t value;
  // when env is nullptr, the value is integer
  VMEnvPtr env;
};

struct VMEnv {
  std::unordered_map<std::uint32_t, VMValue> slot;
  VMEnvPtr outer;
  std::uint32_t ret_pc;
};

struct VMGlobalFunc {
  std::uint32_t func_pc;
  std::vector<std::uint32_t> args;
};

// definition of tables
using VMSymbolTable = std::vector<std::string>;
using VMGlobalFuncTable = std::unordered_map<std::string, VMGlobalFunc>;

// make new VM environment
inline VMEnvPtr MakeVMEnv() {
  auto env = std::make_shared<VMEnv>(VMEnv({{}, nullptr, 0}));
  // insert temp register into slot
  env->slot.insert({0, {0, nullptr}});
  return env;
}

// make new VM environment with specific outer environment
inline VMEnvPtr MakeVMEnv(const VMEnvPtr &outer) {
  auto env = std::make_shared<VMEnv>(VMEnv({{}, outer, 0}));
  // insert temp register into slot
  env->slot.insert({0, {0, nullptr}});
  return env;
}

// make new VM integer value
inline VMValue MakeVMValue(std::int32_t value) {
  return {value, nullptr};
}

#endif  // IONIA_VM_DEFINE_H_
