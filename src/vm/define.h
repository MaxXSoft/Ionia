#ifndef IONIA_VM_DEFINE_H_
#define IONIA_VM_DEFINE_H_

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <cstdint>

struct VMInst {
  // 6-bit opcode
  enum class OpCode : std::uint16_t {
    Get, Set, Getl, Setl, Fun, Cnst, Cnsl,
    Ret, Cenv, Call, Tcal,
    Writ, Read, If, Ifl, Is,
    Eql, Neq, Lt, Leq, Gt, Geq,
    Add, Sub, Mul, Div, Mod,
    And, Or, Not, Xor, Shl, Shr,
    Land, Lor, Lnot,
  } opcode : 6;
  // 10-bit operand
  std::uint16_t opr : 10;
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
  std::unordered_map<const char *, VMValue> slot;
  VMEnvPtr outer;
};

struct VMGlobalFunc {
  std::uint32_t func_pc;
  std::vector<std::uint32_t> args;
};

// definition of tables
using VMSymbolTable = std::vector<std::string>;
using VMGlobalFuncTable = std::unordered_map<std::uint32_t, VMGlobalFunc>;

// make new VM environment
inline VMEnvPtr MakeVMEnv() {
  return std::make_shared<VMEnv>(VMEnv({{}, nullptr}));
}

// make new VM environment with specific outer environment
inline VMEnvPtr MakeVMEnv(const VMEnvPtr &outer) {
  return std::make_shared<VMEnv>(VMEnv({{}, outer}));
}

#endif  // IONIA_VM_DEFINE_H_
