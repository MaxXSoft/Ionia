#ifndef IONIA_DEFINE_SYMBOL_H_
#define IONIA_DEFINE_SYMBOL_H_

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <functional>

#include "define/type.h"

class Environment;
using EnvPtr = std::shared_ptr<Environment>;

class Value {
 public:
  Value(const EnvPtr &env, int value) : env_(env), num_val_(value) {}
  Value(const EnvPtr &env, ASTPtr value)
      : env_(env), func_val_(std::move(value)) {}

  const EnvPtr &env() const { return env_; }
  bool is_func() const { return func_val_ != nullptr; }
  int num_val() const { return num_val_; }
  const ASTPtr &func_val() const { return func_val_; }

 private:
  EnvPtr env_;
  int num_val_;
  ASTPtr func_val_;
};

using ValPtr = std::shared_ptr<Value>;
using ValPtrList = std::vector<ValPtr>;
using ValCallback = std::function<ValPtr(const EnvPtr &env)>;

class Environment {
 public:
  Environment() {}
  Environment(const EnvPtr &outer) : outer_(outer) {}

  void AddSymbol(const std::string &id, const ValPtr &value) {
    symbols_[id] = value;
  }

  ValPtr GetValue(const std::string &id);

  const EnvPtr &outer() const { return outer_; }

 private:
  EnvPtr outer_;
  std::map<std::string, ValPtr> symbols_;
};

#endif  // IONIA_DEFINE_SYMBOL_H_
