#ifndef IONIA_DEFINE_TYPE_H_
#define IONIA_DEFINE_TYPE_H_

#include <memory>
#include <string>
#include <vector>

namespace ionia {

// AST type
class BaseAST;
using ASTPtr = std::unique_ptr<BaseAST>;
using ASTPtrList = std::vector<ASTPtr>;
using IdList = std::vector<std::string>;

}  // namespace ionia

#endif  // IONIA_DEFINE_TYPE_H_
