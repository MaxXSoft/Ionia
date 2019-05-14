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
  f(RET) f(CENV) f(CALL) f(TCAL)            \
  f(WRIT) f(READ) f(IF) f(IS)               \
  f(EQL) f(NEQ) f(LT) f(LEQ) f(GT) f(GEQ)   \
  f(ADD) f(SUB) f(MUL) f(DIV) f(MOD)        \
  f(AND) f(OR) f(NOT) f(XOR) f(SHL) f(SHR)  \
  f(LAND) f(LOR) f(LNOT)
// expand macro to comma-separated list
#define VM_EXPAND_LIST(i)         i,
// expand macro to label list
#define VM_EXPAND_LABEL_LIST(i)   &&VML_##i,
// define a label of VM threading
#define VM_LABEL(l)               VML_##l:

struct VMInst {
  // 6-bit opcode
  enum class OpCode : std::uint32_t {
    VM_INST_ALL(VM_EXPAND_LIST)
  } opcode : 6;
  // 26-bit operand
  std::uint32_t opr : 26;
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

#endif  // IONIA_VM_DEFINE_H_
