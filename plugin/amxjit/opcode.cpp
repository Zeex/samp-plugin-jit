// Copyright (c) 2013-2014 Zeex
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

#include "opcode.h"

namespace amxjit {
namespace {

cell *GetOpcodeTable() {
  #ifdef AMXJIT_RELOCATE_OPCODES
    cell *opcode_table;
    AMX amx = {0};
    amx.flags |= AMX_FLAG_BROWSE;
    amx_Exec(&amx, reinterpret_cast<cell*>(&opcode_table), 0);
    amx.flags &= ~AMX_FLAG_BROWSE;
    return opcode_table;
  #else
    return 0;
  #endif
}

cell FindOpcode(cell *opcode_table, cell opcode) {
  #ifdef AMXJIT_RELOCATE_OPCODES
    if (opcode_table != 0) {
      for (int i = 0; i < NUM_OPCODES; i++) {
        if (opcode_table[i] == opcode) {
          return i;
        }
      }
    }
    return opcode;
  #else
    (void)opcode_table;
    return opcode;
  #endif
}

} // anonymous namespace

OpcodeID RelocateOpcode(cell opcode) {
  #ifdef AMXJIT_RELOCATE_OPCODES
    static cell *opcode_table = GetOpcodeTable();
    opcode = FindOpcode(opcode_table, opcode);
  #endif
	return static_cast<OpcodeID>(opcode);
}

bool Opcode::IsCall() const {
  switch (id) {
    case OP_CALL:
    case OP_CALL_PRI:
      return true;
  }
  return false;
}

bool Opcode::IsJump() const {
  switch (id) {
    case OP_JUMP:
    case OP_JUMP_PRI:
    case OP_JREL:
    case OP_JZER:
    case OP_JNZ:
    case OP_JEQ:
    case OP_JNEQ:
    case OP_JLESS:
    case OP_JLEQ:
    case OP_JGRTR:
    case OP_JGEQ:
    case OP_JSLESS:
    case OP_JSLEQ:
    case OP_JSGRTR:
    case OP_JSGEQ:
      return true;
  }
  return false;
}

} // namespace amxjit
