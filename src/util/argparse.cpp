#include "util/argparse.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <cassert>

// all supported types
#define ARGPARSE_SUPPORTED_TYPES(f)                               \
  f(short) f(unsigned short) f(int) f(unsigned int) f(long)       \
  f(unsigned long) f(long long) f(unsigned long long) f(bool)     \
  f(float) f(double) f(long double) f(std::string)
// expand to type check
#define ARGPARSE_EXPAND_EQL(t)                                    \
  type == typeid(t) ||
// expand to value read
#define ARGPARSE_EXPAND_READ(t)                                   \
  if (value.type() == typeid(t)) return ReadValue<t>(arg, value);

namespace {

template <typename T>
inline bool ReadValue(const char *str, std::any &value) {
  std::istringstream iss(str);
  T v;
  iss >> v;
  value = v;
  return !iss.fail();
}

}  // namespace

bool ArgParser::LogError(const char *message) {
  std::cerr << "[" << program_name_ << ".ArgParser] ERROR: ";
  std::cerr << message << std::endl;
  return false;
}

bool ArgParser::LogError(const char *message, const std::string &name) {
  std::cerr << "[" << program_name_ << ".ArgParser] ERROR: ";
  std::cerr << message << " '" << name << "'" << std::endl;
  return false;
}

void ArgParser::CheckArgName(const std::string &name) {
  // regular expression: ([A-Za-z0-9]|_|-)+
  assert(!name.empty());
  for (const auto &i : name) {
    assert(std::isalnum(i) || i == '_' || i == '-');
  }
}

void ArgParser::CheckArgType(const std::type_info &type) {
  assert(ARGPARSE_SUPPORTED_TYPES(ARGPARSE_EXPAND_EQL) 0);
}

bool ArgParser::ReadArgValue(const char *arg, std::any &value) {
  ARGPARSE_SUPPORTED_TYPES(ARGPARSE_EXPAND_READ);
  assert(false);
  return false;
}

bool ArgParser::Parse(int argc, const char *argv[]) {
  // update program name
  set_program_name(argv[0]);
  // check if need to print help info
  if (argc == 1 && (!args_.empty() || !opts_.empty())) {
    PrintHelp();
    return false;
  }
  int i;
  // insufficient argument count
  if (args_.size() > argc - 1) {
    return LogError("arguments required");
  }
  // parse argument list
  for (i = 0; i < args_.size(); ++i) {
    if (argv[i + 1][0] == '-' ||
        !ReadArgValue(argv[i + 1], vals_[args_[i].name])) {
      return LogError("invalid argument", args_[i].name);
    }
  }
  // parse option list
  for (++i; i < argc; ++i) {
    // find option info
    auto it = opt_map_.find(argv[i]);
    if (it == opt_map_.end()) return LogError("invalid option");
    const auto &info = opts_[it->second];
    auto &val = vals_[info.name];
    // fill value
    if (val.type() == typeid(bool)) {
      val = true;
    }
    else {
      // read argument of option
      ++i;
      if (argv[i][0] == '-' || !ReadArgValue(argv[i], val)) {
        return LogError("invalid option", argv[i - 1]);
      }
    }
  }
  return true;
}

void ArgParser::PrintHelp() {
  using namespace std;
  // print usage
  cout << "usage: " << program_name_ << " ";
  for (const auto &i : args_) cout << "<" << i.name << "> ";
  if (!opts_.empty()) cout << "[options...]" << endl;
  // print arguments
  if (!args_.empty()) {
    cout << endl;
    cout << "arguments:" << endl;
    for (const auto &i : args_) {
      cout << left << setw(padding_) << ("  " + i.name);
      cout << i.help_msg << endl;
    }
  }
  // print options
  if (!opts_.empty()) {
    cout << endl;
    cout << "options:" << endl;
    for (const auto &i : opts_) {
      // generate option title
      auto title = "  -" + i.short_name + ", --" + i.name;
      if (vals_[i.name].type() != typeid(bool)) title += " <arg>";
      // print info
      cout << left << setw(padding_) << title;
      cout << i.help_msg << endl;
    }
  }
}
