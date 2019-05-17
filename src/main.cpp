#include <iostream>
#include <string>
#include <fstream>
#include <functional>
#include <cassert>

#include "version.h"
#include "front/lexer.h"
#include "front/parser.h"
#include "back/interpreter/interpreter.h"
#include "back/compiler/compiler.h"
#include "vm/vm.h"
#include "util/argparse.h"

using namespace std;

namespace {

// default output file name
constexpr const char *kDefaultOutputFile = "out.ibc";

// display version info
void PrintVersion() {
  cout << APP_NAME << " version " << APP_VERSION << endl;
  cout << "A simple, lightweight functional programming language," << endl;
  cout << "with implementation of interpreter and VM runtime." << endl;
  cout << endl;
  cout << "Copyright (C) 2010-2019 MaxXing, MaxXSoft. License GPLv3.";
  cout << endl;
}

// handle all the works of front end, return error count
int HandleFrondEnd(const std::string &input,
                   std::function<int(const ASTPtr &)> func) {
  // open input file
  ifstream ifs(input);
  // initialize lexer and parser
  Lexer lexer(ifs);
  Parser parser(lexer);
  // parse and check
  int err;
  while (auto ast = parser.ParseNext()) {
    err = func(ast);
    if (err) break;
  }
  // get error count
  err += lexer.error_num() + parser.error_num();
  return err;
}

// run bytecode file with VM
int RunBytecode(const std::string &input) {
  VM vm;
  if (!vm.LoadProgram(input)) {
    cerr << "invalid bytecode file" << endl;
    return 1;
  }
  return vm.Run() ? 0 : 1;
}

// compile input source file to output
int Compile(const std::string &input, const std::string &output) {
  Compiler comp;
  // parse and compile
  auto err = HandleFrondEnd(input, [&comp](const ASTPtr &ast) {
    comp.CompileNext(ast);
    return 0;
  });
  // write to output
  if (!err) {
    auto out = output.empty() ? kDefaultOutputFile : output;
    comp.GenerateBytecodeFile(out);
  }
  return err;
}

// compile input file to memory and run with VM
int CompileAndRun(const std::string &input) {
  Compiler comp;
  // parse and compile
  auto err = HandleFrondEnd(input, [&comp](const ASTPtr &ast) {
    comp.CompileNext(ast);
    return 0;
  });
  // check if error
  if (err) return err;
  // generate and run
  VM vm;
  auto ret = vm.LoadProgram(comp.GenerateBytecode());
  assert(ret);
  return vm.Run() ? 0 : 1;
}

// disassemble input bytecode file to output
int Disassemble(const std::string &input, const std::string &output) {
  // TODO
  cerr << "disassembler is not implemented" << endl;
  return 1;
}

// interpret input source file by using interpreter
int Interpret(const std::string &input) {
  Interpreter intp;
  // parse and interpret
  auto err = HandleFrondEnd(input, [&intp](const ASTPtr &ast) {
    return intp.EvalNext(ast) ? 0 : intp.error_num();
  });
  return err;
}

}  // namespace

int main(int argc, const char *argv[]) {
  // set up argument parser
  ArgParser argp;
  argp.AddArgument<string>("input", "input source/bytecode file");
  argp.AddOption<bool>("help", "h", "show this message", false);
  argp.AddOption<bool>("version", "v", "show version info", false);
  argp.AddOption<bool>("interpret", "i",
                       "run source file with interpreter (default)", true);
  argp.AddOption<bool>("compile", "c", "compile source file to bytecode",
                       false);
  argp.AddOption<string>("output", "o", "set output file name", "");
  argp.AddOption<bool>("run-vm", "r", "run bytecode file with VM", false);
  argp.AddOption<bool>("compile-run", "cr",
                       "compile & run source file with VM", false);
  argp.AddOption<bool>("disassemble", "d", "disassemble bytecode file",
                       false);
  // parse argument
  auto ret = argp.Parse(argc, argv);

  // check if need to exit program
  if (argp.GetValue<bool>("help")) {
    argp.PrintHelp();
    return 0;
  }
  else if (argp.GetValue<bool>("version")) {
    PrintVersion();
    return 0;
  }
  else if (!ret) {
    cerr << "invalid input, run '";
    cerr << argp.program_name() << " -h' for help" << endl;
    return 1;
  }

  // dispatch
  int result;
  auto input = argp.GetValue<string>("input");
  if (argp.GetValue<bool>("run-vm")) {
    result = RunBytecode(input);
  }
  else if (argp.GetValue<bool>("compile")) {
    result = Compile(input, argp.GetValue<string>("output"));
  }
  else if (argp.GetValue<bool>("compile-run")) {
    result = CompileAndRun(input);
  }
  else if (argp.GetValue<bool>("disassemble")) {
    result = Disassemble(input, argp.GetValue<string>("output"));
  }
  else if (argp.GetValue<bool>("interpret")) {
    result = Interpret(input);
  }
  else {
    cerr << "invalid command line argument" << endl;
    result = 1;
  }

  return result;
}
