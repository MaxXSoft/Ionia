#include "define/ast.h"
#include "back/interpreter/interpreter.h"

#include <iostream>

// method 'Clone'

ASTPtr IdAST::Clone() {
  return std::make_unique<IdAST>(id_);
}

ASTPtr NumAST::Clone() {
  return std::make_unique<NumAST>(num_);
}

ASTPtr DefineAST::Clone() {
  return std::make_unique<DefineAST>(id_, expr_->Clone());
}

ASTPtr FuncAST::Clone() {
  return std::make_unique<FuncAST>(args_, expr_->Clone());
}

ASTPtr PseudoFuncAST::Clone() {
  auto func = static_cast<FuncAST *>(this);
  return std::make_unique<PseudoFuncAST>(func->args(), func_);
}

ASTPtr FunCallAST::Clone() {
  ASTPtrList args;
  for (const auto &i : args_) {
    args.push_back(i->Clone());
  }
  return std::make_unique<FunCallAST>(id_, std::move(args));
}

// method 'Eval'

ValPtr IdAST::Eval(Interpreter &intp) {
  return intp.EvalId(id_);
}

ValPtr NumAST::Eval(Interpreter &intp) {
  return intp.EvalNum(num_);
}

ValPtr DefineAST::Eval(Interpreter &intp) {
  auto value = expr_->Eval(intp);
  if (!value) return nullptr;
  return intp.EvalDefine(id_, value);
}

ValPtr FuncAST::Eval(Interpreter &intp) {
  return intp.EvalFunc(Clone());
}

ValPtr FunCallAST::Eval(Interpreter &intp) {
  ValPtrList args;
  for (const auto &i : args_) {
    auto val = i->Eval(intp);
    if (!val) return nullptr;
    args.push_back(val);
  }
  return intp.EvalFunCall(id_, args);
}

// method 'Call'

ValPtr FuncAST::Call(Interpreter &intp) {
  return expr_->Eval(intp);
}

ValPtr PseudoFuncAST::Call(Interpreter &intp) {
  return intp.HandlePseudoFunCall(func_);
}

// method 'Compile'

bool IdAST::Compile(Compiler &comp) {
  // TODO
  return false;
}

bool NumAST::Compile(Compiler &comp) {
  // TODO
  return false;
}

bool DefineAST::Compile(Compiler &comp) {
  // TODO
  return false;
}

bool FuncAST::Compile(Compiler &comp) {
  // TODO
  return false;
}

bool FunCallAST::Compile(Compiler &comp) {
  // TODO
  return false;
}
