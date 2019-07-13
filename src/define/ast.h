#ifndef IONIA_DEFINE_AST_H_
#define IONIA_DEFINE_AST_H_

#include <utility>
#include <string>

#include "define/type.h"
#include "define/symbol.h"

namespace ionia {

// forward declaration of interpreter & compiler
class Interpreter;
class Compiler;

class BaseAST {
 public:
  virtual ~BaseAST() = default;

  virtual ASTPtr Clone() = 0;
  virtual ValPtr Eval(Interpreter &intp) = 0;
  virtual void Compile(Compiler &comp) = 0;
};

class IdAST : public BaseAST {
 public:
  IdAST(const std::string &id) : id_(id) {}

  ASTPtr Clone() override;
  ValPtr Eval(Interpreter &intp) override;
  void Compile(Compiler &comp) override;

 private:
  std::string id_;
};

class NumAST : public BaseAST {
 public:
  NumAST(int num) : num_(num) {}

  ASTPtr Clone() override;
  ValPtr Eval(Interpreter &intp) override;
  void Compile(Compiler &comp) override;

 private:
  int num_;
};

class DefineAST : public BaseAST {
 public:
  DefineAST(const std::string &id, ASTPtr expr)
      : id_(id), expr_(std::move(expr)) {}

  ASTPtr Clone() override;
  ValPtr Eval(Interpreter &intp) override;
  void Compile(Compiler &comp) override;

 private:
  std::string id_;
  ASTPtr expr_;
};

class FuncAST : public BaseAST {
 public:
  FuncAST(IdList args, ASTPtr expr)
      : args_(std::move(args)), expr_(std::move(expr)) {}

  virtual ASTPtr Clone() override;
  ValPtr Eval(Interpreter &intp) override;
  void Compile(Compiler &comp) override;

  virtual ValPtr Call(Interpreter &intp);

  const IdList &args() const { return args_; }

 protected:
  FuncAST(IdList args) : args_(std::move(args)) {}

 private:
  IdList args_;
  ASTPtr expr_;
};

class PseudoFuncAST : public FuncAST {
 public:
  PseudoFuncAST(IdList args, ValCallback func)
      : FuncAST(std::move(args)), func_(func) {}

  ASTPtr Clone() override;
  ValPtr Call(Interpreter &intp) override;

 private:
  ValCallback func_;
};

class FunCallAST : public BaseAST {
 public:
  FunCallAST(ASTPtr callee, ASTPtrList args)
      : callee_(std::move(callee)), args_(std::move(args)) {}

  ASTPtr Clone() override;
  ValPtr Eval(Interpreter &intp) override;
  void Compile(Compiler &comp) override;

 private:
  ASTPtr callee_;
  ASTPtrList args_;
};

}  // ionia

#endif  // IONIA_DEFINE_AST_H_
