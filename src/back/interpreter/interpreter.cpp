#include "back/interpreter/interpreter.h"

#include <iostream>
#include <utility>
#include <functional>
#include <cassert>
#include <cstddef>

#include "define/ast.h"

using namespace ionia;

namespace {

inline FuncAST *FuncCast(const ValPtr &val) {
  return dynamic_cast<FuncAST *>(val->func_val().get());
}

template <typename Obj, typename Func, typename... Args>
inline void AddFuncToEnv(const EnvPtr &env, const char *id,
                         const IdList &args, Obj obj, Func func,
                         Args... other) {
  using namespace std::placeholders;
  auto obj_func = std::bind(func, obj, _1, other...);
  auto fun = std::make_unique<PseudoFuncAST>(args, obj_func);
  env->AddSymbol(id, std::make_shared<Value>(env, std::move(fun)));
}

}  // namespace

void Interpreter::InitEnvironment() {
  auto env = std::make_shared<Environment>();
  // create pseudo functions
  AddFuncToEnv(env, "<<<", {"v"}, this, &Interpreter::IonPrint);
  AddFuncToEnv(env, ">>>", {}, this, &Interpreter::IonInput);
  AddFuncToEnv(env, "?", {"c", "t", "e"}, this, &Interpreter::IonIf);
  AddFuncToEnv(env, "is", {"l", "r"}, this, &Interpreter::IonIs);
  // create pseudo functions (operators)
  auto add_op = [this, &env](const char *id, Operator op, bool unary) {
    if (unary) {
      AddFuncToEnv(env, id, {"l"}, this, &Interpreter::IonCalcOp, op);
    }
    else {
      AddFuncToEnv(env, id, {"l", "r"}, this, &Interpreter::IonCalcOp, op);
    }
  };
  add_op("eq", Operator::Equal, false);
  add_op("neq", Operator::NotEqual, false);
  add_op("lt", Operator::Less, false);
  add_op("le", Operator::LessEqual, false);
  add_op("gt", Operator::Great, false);
  add_op("ge", Operator::GreatEqual, false);
  add_op("+", Operator::Add, false);
  add_op("-", Operator::Sub, false);
  add_op("*", Operator::Mul, false);
  add_op("/", Operator::Div, false);
  add_op("%", Operator::Mod, false);
  add_op("&", Operator::And, false);
  add_op("|", Operator::Or, false);
  add_op("~", Operator::Not, true);
  add_op("^", Operator::Xor, false);
  add_op("<<", Operator::Shl, false);
  add_op(">>", Operator::Shr, false);
  add_op("&&", Operator::LogicAnd, false);
  add_op("||", Operator::LogicOr, false);
  add_op("!", Operator::LogicNot, true);
  // do initialize
  root_ = std::move(env);
  envs_.push(std::make_shared<Environment>(root_));
}

ValPtr Interpreter::PrintError(const char *message) {
  std::cerr << "error(interpreter): " << message << std::endl;
  ++error_num_;
  return nullptr;
}

ValPtr Interpreter::CallFunc(const ValPtr &func, const ValPtrList &args) {
  // convert to pointer of FuncAST
  auto func_ptr = FuncCast(func);
  if (!func_ptr) return PrintError("calling a non-function");
  // check argument
  if (args.size() != func_ptr->args().size()) {
    return PrintError("argument count mismatch");
  }
  // create new nested environment
  auto args_env = std::make_shared<Environment>(func->env());
  for (std::size_t i = 0; i < args.size(); ++i) {
    args_env->AddSymbol(func_ptr->args()[i], args[i]);
  }
  // call function
  envs_.push(args_env);
  auto ret = func_ptr->Call(*this);
  envs_.pop();
  return ret;
}

ValPtr Interpreter::IonPrint(const EnvPtr &env) {
  auto val = env->GetValue("v");
  if (!val) return PrintError("invalid argument");
  PrintValue(val);
  return val;
}

ValPtr Interpreter::IonInput(const EnvPtr &env) {
  int num;
  std::cin >> num;
  return std::make_shared<Value>(env, num);
}

ValPtr Interpreter::IonIf(const EnvPtr &env) {
  // get arguments
  auto cond = env->GetValue("c");
  auto then = env->GetValue("t");
  auto else_then = env->GetValue("e");
  if (!cond || cond->is_func() || !then || !then->is_func()
      || !else_then || !else_then->is_func()) {
    return PrintError("invalid argument");
  }
  // execute 'if' operation
  return CallFunc(cond->num_val() ? then : else_then, {});
}

ValPtr Interpreter::IonIs(const EnvPtr &env) {
  auto lhs = env->GetValue("l"), rhs = env->GetValue("r");
  if (!lhs || !rhs) return PrintError("invalid argument");
  return std::make_shared<Value>(env, lhs == rhs ? 1 : 0);
}

ValPtr Interpreter::IonCalcOp(const EnvPtr &env, Operator op) {
  // get arguments
  bool arg_err = false;
  ValPtr lhs = env->GetValue("l"), rhs;
  arg_err = !lhs || lhs->is_func();
  if (op != Operator::Not && op != Operator::LogicNot) {
    rhs = env->GetValue("r");
    if (!arg_err) arg_err = !rhs || rhs->is_func();
  }
  if (arg_err) return PrintError("invalid argument");
  // convert to integer
  auto l = lhs->num_val(), r = rhs ? rhs->num_val() : 0;
  auto new_val = [&env](int v) { return std::make_shared<Value>(env, v); };
  // do calculation
  switch (op) {
    case Operator::Equal: return new_val(l == r);
    case Operator::NotEqual: return new_val(l != r);
    case Operator::Less: return new_val(l < r);
    case Operator::LessEqual: return new_val(l <= r);
    case Operator::Great: return new_val(l > r);
    case Operator::GreatEqual: return new_val(l >= r);
    case Operator::Add: return new_val(l + r);
    case Operator::Sub: return new_val(l - r);
    case Operator::Mul: return new_val(l * r);
    case Operator::Div: return new_val(l / r);
    case Operator::Mod: return new_val(l % r);
    case Operator::And: return new_val(l & r);
    case Operator::Or: return new_val(l | r);
    case Operator::Not: return new_val(~l);
    case Operator::Xor: return new_val(l ^ r);
    case Operator::Shl: return new_val(l << r);
    case Operator::Shr: return new_val(l >> r);
    case Operator::LogicAnd: return new_val(l && r);
    case Operator::LogicOr: return new_val(l || r);
    case Operator::LogicNot: return new_val(!l);
    default: return nullptr;
  }
}

ValPtr Interpreter::EvalNext(const ASTPtr &ast) {
  assert(ast);
  return ast->Eval(*this);
}

ValPtr Interpreter::EvalId(const std::string &id) {
  auto value = envs_.top()->GetValue(id);
  if (!value) return PrintError("identifier not found");
  return value;
}

ValPtr Interpreter::EvalNum(int num) {
  return std::make_shared<Value>(envs_.top(), num);
}

ValPtr Interpreter::EvalDefine(const std::string &id, const ValPtr &expr) {
  envs_.top()->AddSymbol(id, expr);
  return expr;
}

ValPtr Interpreter::EvalFunc(ASTPtr func) {
  return std::make_shared<Value>(envs_.top(), std::move(func));
}

ValPtr Interpreter::EvalFunCall(const ValPtr &callee,
                                const ValPtrList &args) {
  // get & check function
  if (!callee || !callee->is_func()) return PrintError("invalid function");
  return CallFunc(callee, args);
}

ValPtr Interpreter::HandlePseudoFunCall(ValCallback func) {
  return func(envs_.top());
}

void Interpreter::PrintValue(const ValPtr &value) {
  if (value->is_func()) {
    std::cout << "<function at: ";
    std::cout << value->func_val().get();
    std::cout << ">" << std::endl;
  }
  else {
    std::cout << value->num_val() << std::endl;
  }
}
