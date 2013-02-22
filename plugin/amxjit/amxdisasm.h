// Copyright (c) 2012-2013 Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef AMXJIT_AMXDISASM_H
#define AMXJIT_AMXDISASM_H

#include <cassert>
#include <string>
#include <vector>
#include <utility>
#include "amxopcode.h"
#include "amxptr.h"

namespace amxjit {

// REG_NONE is defined in WinNT.h.
#ifdef REG_NONE
  #undef REG_NONE
#endif

enum AMXRegister {
  REG_NONE = 0,
  REG_PRI = (2 << 0),
  REG_ALT = (2 << 1),
  REG_COD = (2 << 2),
  REG_DAT = (2 << 3),
  REG_HEA = (2 << 4),
  REG_STP = (2 << 5),
  REG_STK = (2 << 6),
  REG_FRM = (2 << 7),
  REG_CIP = (2 << 8)
};

class AMXInstruction {
 public:
  AMXInstruction();

 public:
  int size() const {
    return sizeof(cell) * (1 + operands_.size());
  }

  const cell address() const {
    return address_;
  }
  void set_address(cell address) {
    address_ = address;
  }

  AMXOpcode opcode() const {
    return opcode_;
  }
  void set_opcode(AMXOpcode opcode) {
    opcode_ = opcode;
  }

  cell operand(unsigned int index = 0u) const {
    assert(index < operands_.size());
    return operands_[index];
  }
  std::vector<cell> &operands() {
    return operands_;
  }
  const std::vector<cell> &operands() const {
    return operands_;
  }
  void set_operands(std::vector<cell> operands) {
    operands_ = operands;
  }
  void add_operand(cell value) {
    operands_.push_back(value);
  }
  int num_operands() const {
    return operands_.size();
  }

 public:
  const char *name() const;

  int get_src_regs() const;
  int get_dst_regs() const;

  std::string string() const;

 private:
  cell address_;
  AMXOpcode opcode_;
  std::vector<cell> operands_;

 private:
  struct StaticInfoTableEntry {
    const char *name;
    int src_regs;
    int dst_regs;
  };
  static const StaticInfoTableEntry info[NUM_AMX_OPCODES];
};

class AMXDisassembler {
 public:
  AMXDisassembler(AMXPtr amx);

 public:
  // Returns the address of the currently executing instruction
  // (instruction pointer).
  cell ip() const { return ip_; }

  // Sets the instruction pointer.
  void set_ip(cell ip) { ip_ = ip; }

 public:
  // Decodes current instruction and returns true until the end of code gets
  // reached or an error occurs. The optional error argument is set to true
  // on error.
  bool decode(AMXInstruction &instr, bool *error = 0);

private:
  AMXPtr amx_;
  cell ip_;
};

class AMXCaseTable {
 public:
  AMXCaseTable(AMXPtr amx, cell offset);

  // Returns the total number of records in the case table.
  int num_cases() const;

  // Returns the address of a 'case X:' block at the specified index.
  cell value_at(cell index) const;
  cell address_at(cell index) const;

  // Get the minimum and maximum values.
  cell find_min_value() const;
  cell find_max_value() const;

  // Returns the address of the 'default:' block.
  cell default_address() const;

 private:
  std::vector<std::pair<cell, cell> > records_;
};

} // namespace amxjit

#endif // !AMXJIT_AMXDISASM_H
