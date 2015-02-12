// Copyright (c) 2012-2015 Zeex
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

#ifndef AMXJIT_DISASM_H
#define AMXJIT_DISASM_H

#include <cstddef>
#include <string>
#include <vector>
#include <utility>
#include "amxref.h"
#include "opcode.h"

namespace amxjit {

// Win32 defines REG_NONE in WinNT.h.
#undef REG_NONE

enum Register {
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

class Instruction {
 public:
  Instruction();

  const char *name() const;
  std::size_t size() const;

  const cell address() const { return address_; }
  void set_address(cell address) { address_ = address; }

  Opcode opcode() const { return opcode_; }
  void set_opcode(Opcode opcode) { opcode_ = opcode; }

  cell operand(std::size_t index = 0) const;

  const std::vector<cell> &operands() const {
    return operands_;
  }
  void set_operands(std::vector<cell> operands) {
    operands_ = operands;
  }

  void AppendOperand(cell value) { operands_.push_back(value); }
  void RemoveOperands() { operands_.clear(); }

  std::string ToString() const;

 private:
  cell address_;
  Opcode opcode_;
  std::vector<cell> operands_;

 private:
  struct StaticInstrInfo {
    const char *name;
    int src_regs;
    int dst_regs;
  };
  static const StaticInstrInfo info[NUM_OPCODES];
};

class CaseTable {
 public:
  CaseTable(AMXRef amx, cell offset);

  // Returns the total number of records_ in the case table.
  int num_cases() const;

  // Returns the address of a "case X:" block.
  cell GetCaseValue(cell index) const;
  cell GetCaseAddress(cell index) const;

  // Finds the minimum and maximum values in the table.
  cell FindMinValue() const;
  cell FindMaxValue() const;

  // Returns the address of the "default:" block.
  cell GetDefaultAddress() const;

 private:
  std::vector<std::pair<cell, cell> > records_;
};

// Decodes a single AMX instruction at the specified address and
// stored the result in instr. Returns false on error.
bool DecodeInstruction(AMXRef amx, cell address);
bool DecodeInstruction(AMXRef amx, cell address, Instruction &instr);

// Disassembler is merely a convenience wrapper around
// DecodeInstruction. It's well suited for whlie loops
// like the following:
//
//   Instruction instr;
//   Disassembler disasm(amx);
//
//   while (disasm.Decode(instr)) {
//     ... do something with instr ...
//   }
//
// The error argument is set to true if DecodeInstruction returns
// an error (usually means an invalid instruction).
class Disassembler {
 public:
  Disassembler(AMXRef amx): amx_(amx), cur_address_() {}
  bool Decode(Instruction &instr);
  bool Decode(Instruction &instr, bool &error);
 private:
  AMXRef amx_;
  cell cur_address_;
};

} // namespace amxjit

#endif // !AMXJIT_DISASM_H
