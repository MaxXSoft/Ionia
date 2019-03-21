#ifndef IONIA_INTERPRETER_H_
#define IONIA_INTERPRETER_H_

#include <string>
#include <stack>

#include "front/parser.h"
#include "define/symbol.h"

class Interpreter {
public:
    Interpreter(Parser &parser) : parser_(parser), error_num_(0) {
        InitEnvironment();
    }

    ValPtr EvalNext();
    ValPtr EvalId(const std::string &id);
    ValPtr EvalNum(int num);
    ValPtr EvalDefine(const std::string &id, const ValPtr &expr);
    ValPtr EvalFunc(ASTPtr func);
    ValPtr EvalFunCall(const std::string &id, const ValPtrList &args);
    ValPtr HandlePseudoFunCall(ValCallback func);

    void PrintValue(const ValPtr &value);

    unsigned int error_num() const { return error_num_; }
    const EnvPtr &root() const { return root_; }

private:
    enum class Operator {
        Equal, NotEqual, Less, LessEqual, Great, GreatEqual,
        Add, Sub, Mul, Div, Mod,
        And, Or, Not, Xor, Shl, Shr,
        LogicAnd, LogicOr, LogicNot,
    };
    
    void InitEnvironment();
    ValPtr PrintError(const char *message);
    ValPtr CallFunc(const ValPtr &func, const ValPtrList &args);

    // internal functions of ion lang
    ValPtr IonPrint(const EnvPtr &env);
    ValPtr IonInput(const EnvPtr &env);
    ValPtr IonIf(const EnvPtr &env);
    ValPtr IonIs(const EnvPtr &env);
    ValPtr IonCalcOp(const EnvPtr &env, Operator op);

    Parser &parser_;
    unsigned int error_num_;
    EnvPtr root_;
    std::stack<EnvPtr> envs_;
};

#endif // IONIA_INTERPRETER_H_
