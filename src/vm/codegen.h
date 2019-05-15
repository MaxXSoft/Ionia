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

  void SetLabel();

private:
  friend class VMCodeGen;

  VMCodeLabel(VMCodeGen *gen, std::uint32_t offset)
      : gen_(gen), offset_(offset) {}

  void FillLabel();

  VMCodeGen *gen_;
  std::uint32_t offset_;
};

// code generator of Ionia VM
class VMCodeGen {
 public:
  virtual ~VMCodeGen() = default;

  // Function returns start position of bytecode segment.
  // If error, returns -1.
  static int ParseBytecode(const std::vector<std::uint8_t> &buffer,
                           VMSymbolTable &sym_table,
                           VMFuncPCTable &pc_table,
                           VMGlobalFuncTable &global_funcs);

  // generate bytecode vector
  std::vector<std::uint8_t> GenerateBytecode();
  // generate bytecode to file
  void GenerateBytecodeFile(const std::string &file);

  // generate instructions
  void GET(const std::string &name);
  void SET(const std::string &name);
  void FUN();
  void CNST(std::uint32_t num);
  void CNST(const std::string &label);
  void CNSH(std::uint32_t num);
  void PUSH(const std::string &name);
  void POP(const std::string &name);
  void RET();
  void CALL(const std::string &name);
  void TCAL(const std::string &name);

  // create a new named label
  void LABEL(const std::string &label);
  // create a new anonymous label
  VMCodeLabel NewLabel();

  // define function (pseudo instruction)
  void DefineFunction(const std::string &name);
  // set constant value (pseudo instruction)
  void SetConst(std::int32_t num);
  // register new global function
  void RegisterGlobalFunction(const std::string &name,
                              const std::vector<std::string> &args);

  // get current pc
  std::uint32_t pc() const { return inst_buf_.size(); }

 private:
  // file header of Ionia VM's bytecode file (bad bite c -> bad byte code)
  static const std::uint32_t kFileHeader = 0xec17dbba;
  // minimum bytecode file size
  static const std::uint32_t kMinFileSize = 4 * 4;

  friend class VMCodeLabel;

  using OffsetList = std::forward_list<std::uint32_t>;

  std::uint32_t GetSymbolIndex(const std::string &name);
  void PushInst(const VMInst &inst);
  void PushInst(VMInst::OpCode op);
  void PushLabelInst(VMInst::OpCode op, const std::string &label);
  void PushLabelInst(VMInst::OpCode op, const VMCodeLabel &label);
  void FillNamedLabels();

  // tables
  VMSymbolTable sym_table_;
  VMFuncPCTable pc_table_;
  std::map<std::uint32_t, VMGlobalFunc> global_funcs_;
  // buffer that stores instructions
  std::vector<std::uint8_t> inst_buf_;
  // map of named labels
  std::map<std::string, std::uint32_t> named_labels_;
  // map of unfilled insturctions
  std::map<const VMCodeLabel *, OffsetList> unfilled_anon_;
  std::map<std::string, OffsetList> unfilled_named_;
};

#endif  // IONIA_VM_BYTECODE_H_
