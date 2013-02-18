// Copyright (c) 2013 Zeex
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

#include <amx/amx.h>
#include "amxopcode.h"

namespace jit {

static cell *get_opcode_map() {
  #if defined __GNUC__
    cell *opcode_map;
    AMX amx = {0};
    amx.flags |= AMX_FLAG_BROWSE;
    amx_Exec(&amx, reinterpret_cast<cell*>(&opcode_map), 0);
    amx.flags &= ~AMX_FLAG_BROWSE;
    return opcode_map;
  #else
    return 0;
  #endif
}

static cell lookup_opcode(cell *opcode_map, cell opcode) {
  #if defined __GNUC__
    if (opcode_map != 0) {
      // Search for this opcode in the opcode relocation table.
      for (int i = 0; i < NUM_AMX_OPCODES; i++) {
        if (opcode_map[i] == opcode) {
          return i;
        }
      }
    }
    return opcode;
  #else
    (void)opcode_map;
    return opcode;
  #endif
}

AMXOpcodeID relocate_opcode(cell opcode) {
  #if defined __GNUC__
    static cell *opcode_map = get_opcode_map();
    opcode = lookup_opcode(opcode_map, opcode);
  #endif
	return static_cast<AMXOpcodeID>(opcode);
}

bool AMXOpcode::is_call() const {
  switch (id_) {
    case AMX_OP_CALL:
    case AMX_OP_CALL_PRI:
      return true;
  }
  return false;
}

bool AMXOpcode::is_jump() const {
  switch (id_) {
    case AMX_OP_JUMP:
    case AMX_OP_JUMP_PRI:
    case AMX_OP_JREL:
    case AMX_OP_JZER:
    case AMX_OP_JNZ:
    case AMX_OP_JEQ:
    case AMX_OP_JNEQ:
    case AMX_OP_JLESS:
    case AMX_OP_JLEQ:
    case AMX_OP_JGRTR:
    case AMX_OP_JGEQ:
    case AMX_OP_JSLESS:
    case AMX_OP_JSLEQ:
    case AMX_OP_JSGRTR:
    case AMX_OP_JSGEQ:
      return true;
  }
  return false;
}

} // namespace jit
