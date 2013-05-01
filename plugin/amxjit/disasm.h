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

#ifndef AMXJIT_DISASM_H
#define AMXJIT_DISASM_H

#include <cassert>
#include <vector>
#include <utility>
#include "amxptr.h"
#include "opcode.h"

namespace amxjit {

// REG_NONE is defined in WinNT.h.
#ifdef REG_NONE
  #undef REG_NONE
#endif

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

 public:
  int GetSize() const {
    return sizeof(cell) * (1 + operands.size());
  }

  const cell GetAddress() const {
    return address;
  }

  void SetAddress(cell address) {
    this->address = address;
  }

  Opcode GetOpcode() const {
    return opcode;
  }

  void SetOpcode(Opcode opcode) {
    this->opcode = opcode;
  }

  cell GetOperand(unsigned int index = 0u) const {
    assert(index < operands.size());
    return operands[index];
  }

  std::vector<cell> &GetOperands() {
    return operands;
  }

  const std::vector<cell> &GetOperands() const {
    return operands;
  }

  void SetOperands(std::vector<cell> operands) {
    operands = operands;
  }

  void AddOperand(cell value) {
    operands.push_back(value);
  }

  int GetNumOperands() const {
    return operands.size();
  }

 public:
  const char *GetName() const;

  int GetSrcRegs() const;
  int GetDstRegs() const;

  std::string AsString() const;

 private:
  cell address;
  Opcode opcode;
  std::vector<cell> operands;

 private:
  struct StaticInfoTableEntry {
    const char *name;
    int srcRegs;
    int dstRegs;
  };

  static const StaticInfoTableEntry info[NUM_OPCODES];
};

class Disassembler {
 public:
  Disassembler(AMXPtr amx);

 public:
  // Gets/sets the instruction pointer.
  cell GetInstrPtr() const { return ip; }
  void SetInstrPtr(cell ip) { this->ip = ip; }

  // Decodes current instruction and returns true until the end of code gets
  // reached or an error occurs. The optional error argument is set to true
  // on error.
  bool Decode(Instruction &instr, bool *error = 0);

 private:
  AMXPtr amx;
  cell ip;
};

class CaseTable {
 public:
  CaseTable(AMXPtr amx, cell offset);

  // Returns the total number of records in the case table.
  int GetNumCases() const;

  // Returns the address of a 'case X:' block at the specified index.
  cell GetValue(cell index) const;
  cell GetAddress(cell index) const;

  // Finds the minimum and maximum values in the table.
  cell FindMinValue() const;
  cell FindMaxValue() const;

  // Returns the address of the 'default:' block.
  cell GetDefaultAddress() const;

 private:
  std::vector<std::pair<cell, cell> > records;
};

} // namespace amxjit

#endif // !AMXJIT_DISASM_H
