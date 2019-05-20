#include "back/compiler/compiler.h"

using namespace ionia;

std::string Compiler::GetNextLabel() {
  return ":func-" + std::to_string(label_id_++);
}

void Compiler::GenerateAllFuncDefs() {
  while (!func_defs_.empty()) {
    const auto &func = func_defs_.front();
    // generate label
    gen_.LABEL(func.label);
    // generate prologue
    for (auto it = func.args.rbegin(); it != func.args.rend(); ++it) {
      gen_.POP();
      gen_.SET(*it);
    }
    // generate body
    func.expr->Compile(*this);
    // generate return
    gen_.GenReturn();
    // check if is global function
    if (func.name[0] == '$') {
      gen_.RegisterGlobalFunction(func.name, func.args.size());
    }
    func_defs_.pop_front();
  }
}

std::vector<std::uint8_t> Compiler::GenerateBytecode() {
  // generate RET instruction
  gen_.RET();
  // generate all function definitions
  GenerateAllFuncDefs();
  // generate bytecode
  return gen_.GenerateBytecode();
}

void Compiler::GenerateBytecodeFile(const std::string &file) {
  // generate RET instruction
  gen_.RET();
  // generate all function definitions
  GenerateAllFuncDefs();
  // generate bytecode
  gen_.GenerateBytecodeFile(file);
}

void Compiler::Reset() {
  gen_.Reset();
  func_defs_.clear();
  label_id_ = 0;
}

void Compiler::CompileNext(const ASTPtr &ast) {
  ast->Compile(*this);
}

void Compiler::CompileId(const std::string &id) {
  gen_.SmartGet(id);
}

void Compiler::CompileNum(int num) {
  gen_.SetConst(num);
}

void Compiler::CompileDefine(const std::string &id, const ASTPtr &expr) {
  auto last_func_def_len = func_defs_.size();
  // generate definition
  expr->Compile(*this);
  // check if length of func def changed
  if (func_defs_.size() != last_func_def_len) {
    // record function name
    func_defs_.back().name = id;
  }
  // generate SET instruction
  gen_.SET(id);
}

void Compiler::CompileFunc(const IdList &args, const ASTPtr &expr) {
  auto label = GetNextLabel();
  // record function definition
  func_defs_.emplace_back(FuncDefInfo({label, "", args, expr->Clone()}));
  // generate function value
  gen_.GetFuncValue(label);
}

void Compiler::CompileFunCall(const std::string &id,
                              const ASTPtrList &args) {
  // push all arguments
  for (const auto &i : args) {
    i->Compile(*this);
    gen_.PUSH();
  }
  // call function id
  gen_.CALL(id);
}
