#ifndef IONIA_BACK_COMPILER_COMPILER_H_
#define IONIA_BACK_COMPILER_COMPILER_H_

#include <string>
#include <vector>
#include <deque>
#include <cstdint>

#include "define/ast.h"
#include "vm/codegen.h"

namespace ionia {

class Compiler {
 public:
  Compiler() { Reset(); }

  // generate bytecode buffer
  std::vector<std::uint8_t> GenerateBytecode();
  // generate bytecode file
  void GenerateBytecodeFile(const std::string &file);
  // reset compiler
  void Reset();

  // compile next AST
  void CompileNext(const ASTPtr &ast);

  // visitor functions
  void CompileId(const std::string &id);
  void CompileNum(int num);
  void CompileDefine(const std::string &id, const ASTPtr &expr);
  void CompileFunc(const IdList &args, const ASTPtr &expr);
  void CompileFunCall(const std::string &id, const ASTPtrList &args);

 private:
  struct FuncDefInfo {
    std::string label;
    IdList args;
    ASTPtr expr;
  };

  // return next label for function generation
  std::string GetNextLabel();
  // generate all of function definitions in 'func_defs_'
  void GenerateAllFuncDefs();

  VMCodeGen gen_;
  std::deque<FuncDefInfo> func_defs_;
  int label_id_;
};

}  // namespace ionia

#endif  // IONIA_BACK_COMPILER_COMPILER_H_
