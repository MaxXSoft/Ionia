#ifndef IONIA_VM_BYTECODE_H_
#define IONIA_VM_BYTECODE_H_

#include <vector>
#include <map>
#include <string>
#include <forward_list>
#include <cstdint>

#include "vm/define.h"

// forward declaration of VMCodeGen
class VMCodeGen;

// helper class of VMCodeGen
class VMCodeLabel {
public:
  VMCodeLabel() = delete;
  VMCodeLabel(const VMCodeLabel &) = delete;
  VMCodeLabel(VMCodeLabel &&label) noexcept
      : gen_(label.gen_), offset_(label.offset_) {
    label.gen_ = nullptr;
  }
  ~VMCodeLabel() { FillLabel(); }

  VMCodeLabel &operator=(const VMCodeLabel &) = delete;
  VMCodeLabel &operator=(VMCodeLabel &&label) noexcept {
    if (&label != this) {
      FillLabel();
      gen_ = label.gen_;
      offset_ = label.offset_;
      label.gen_ = nullptr;
    }
    return label;
  }

  inline void SetLabel();

private:
  friend class VMCodeGen;

  VMCodeLabel(VMCodeGen *gen, std::uint32_t offset)
      : gen_(gen), offset_(offset) {}

  inline void FillLabel();

  VMCodeGen *gen_;
  std::uint32_t offset_;
};

// code generator of Ionia VM
class VMCodeGen {
 public:
  VMCodeGen() { sym_table_.push_back("temp"); }

  // Function returns start position of bytecode segment.
  // If error, returns -1.
  static int ParseBytecode(const std::vector<std::uint8_t> &buffer,
                           VMSymbolTable &sym_table,
                           VMGlobalFuncTable &global_funcs);

  // generate bytecode vector
  std::vector<std::uint8_t> GenerateBytecode();
  // generate bytecode to file
  void GenerateBytecodeFile(const std::string &file);

  // generate instructions
  inline void GET(const std::string &name);
  inline void SET(const std::string &name);
  inline void FUN();
  inline void CNST(std::uint32_t num);
  inline void RET();
  inline void CENV();
  inline void CALL(const std::string &name);
  inline void TCAL(const std::string &name);
  inline void WRIT();
  inline void READ();
  inline void IF(const std::string &label);
  inline void IF(const VMCodeLabel &label);
  inline void IS(const std::string &name);
  inline void EQL(const std::string &name);
  inline void NEQ(const std::string &name);
  inline void LT(const std::string &name);
  inline void LEQ(const std::string &name);
  inline void GT(const std::string &name);
  inline void GEQ(const std::string &name);
  inline void ADD(const std::string &name);
  inline void SUB(const std::string &name);
  inline void MUL(const std::string &name);
  inline void DIV(const std::string &name);
  inline void MOD(const std::string &name);
  inline void AND(const std::string &name);
  inline void OR(const std::string &name);
  inline void NOT(const std::string &name);
  inline void XOR(const std::string &name);
  inline void SHL(const std::string &name);
  inline void SHR(const std::string &name);
  inline void LAND(const std::string &name);
  inline void LOR(const std::string &name);
  inline void LNOT(const std::string &name);

  // create a new named label
  inline void LABEL(const std::string &label);
  // create a new anonymous label
  inline VMCodeLabel NewLabel();

  // register new global function
  void RegisterGlobalFunction(const std::string &name,
                              const std::vector<std::string> &args);

  // get current pc
  std::uint32_t pc() const { return inst_buf_.size(); }

 private:
  // file header of Ionia VM's bytecode file (bad bite c -> bad byte code)
  static const std::uint32_t kFileHeader = 0xbadb17ec;
  // minimum bytecode file size
  static const std::uint32_t kMinFileSize = 3 * 4;

  friend class VMCodeLabel;

  using InstList = std::forward_list<VMInst *>;

  std::uint32_t GetSymbolIndex(const std::string &name);
  inline void PushInst(const VMInst &inst);
  inline void PushInst(VMInst::OpCode op);
  void FillNamedLabels();

  // tables
  VMSymbolTable sym_table_;
  std::map<std::uint32_t, VMGlobalFunc> global_funcs_;
  // buffer that stores instructions
  std::vector<std::uint8_t> inst_buf_;
  // map of named labels
  std::map<std::string, std::uint32_t> named_labels_;
  // map of unfilled insturctions
  std::map<const VMCodeLabel *, InstList> unfilled_anon_;
  std::map<std::string, InstList> unfilled_named_;
};

#endif  // IONIA_VM_BYTECODE_H_
