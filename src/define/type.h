#ifndef IONIA_DEFINE_TYPE_H_
#define IONIA_DEFINE_TYPE_H_

#include <memory>
#include <string>
#include <vector>

// AST type
class BaseAST;
using ASTPtr = std::unique_ptr<BaseAST>;
using ASTPtrList = std::vector<ASTPtr>;
using IdList = std::vector<std::string>;

#endif  // IONIA_DEFINE_TYPE_H_
