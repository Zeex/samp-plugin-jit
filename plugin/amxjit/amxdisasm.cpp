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

#include <iomanip>
#include <sstream>
#include "amxptr.h"
#include "amxdisasm.h"

namespace amxjit {

const AMXInstruction::StaticInfoTableEntry AMXInstruction::info[NUM_AMX_OPCODES] = {
  {"none",           REG_NONE,             REG_NONE},
  {"load.pri",       REG_NONE,             REG_PRI},
  {"load.alt",       REG_NONE,             REG_ALT},
  {"load.s.pri",     REG_FRM,              REG_PRI},
  {"load.s.alt",     REG_FRM,              REG_ALT},
  {"lref.pri",       REG_NONE,             REG_PRI},
  {"lref.alt",       REG_NONE,             REG_ALT},
  {"lref.s.pri",     REG_FRM,              REG_PRI},
  {"lref.s.alt",     REG_FRM,              REG_ALT},
  {"load.i",         REG_PRI,              REG_PRI},
  {"lodb.i",         REG_PRI,              REG_PRI},
  {"const.pri",      REG_NONE,             REG_PRI},
  {"const.alt",      REG_NONE,             REG_ALT},
  {"addr.pri",       REG_FRM,              REG_PRI},
  {"addr.alt",       REG_FRM,              REG_ALT},
  {"stor.pri",       REG_PRI,              REG_NONE},
  {"stor.alt",       REG_ALT,              REG_NONE},
  {"stor.s.pri",     REG_FRM | REG_PRI,    REG_NONE},
  {"stor.s.alt",     REG_FRM | REG_ALT,    REG_NONE},
  {"sref.pri",       REG_PRI,              REG_NONE},
  {"sref.alt",       REG_ALT,              REG_NONE},
  {"sref.s.pri",     REG_FRM | REG_PRI,    REG_NONE},
  {"sref.s.alt",     REG_FRM | REG_ALT,    REG_NONE},
  {"stor.i",         REG_PRI | REG_ALT,    REG_NONE},
  {"strb.i",         REG_PRI | REG_ALT,    REG_NONE},
  {"lidx",           REG_PRI | REG_ALT,    REG_PRI},
  {"lidx.b",         REG_PRI | REG_ALT,    REG_PRI},
  {"idxaddr",        REG_PRI | REG_ALT,    REG_PRI},
  {"idxaddr.b",      REG_PRI | REG_ALT,    REG_PRI},
  {"align.pri",      REG_NONE, REG_PRI},
  {"align.alt",      REG_NONE, REG_ALT},
  {"lctrl",          REG_PRI | REG_COD |
                     REG_DAT | REG_HEA |
                     REG_STP | REG_STK |
                     REG_FRM | REG_CIP,    REG_PRI},
  {"sctrl",          REG_PRI,              REG_HEA | REG_STK |
                                           REG_FRM | REG_CIP},
  {"move.pri",       REG_ALT,              REG_PRI},
  {"move.alt",       REG_PRI,              REG_ALT},
  {"xchg",           REG_PRI | REG_ALT,    REG_PRI | REG_ALT},
  {"push.pri",       REG_PRI | REG_STK,    REG_STK},
  {"push.alt",       REG_ALT | REG_STK,    REG_STK},
  {"push.r",         REG_NONE,             REG_NONE},
  {"push.c",         REG_STK,              REG_STK},
  {"push",           REG_STK,              REG_STK},
  {"push.s",         REG_STK | REG_FRM,    REG_STK},
  {"pop.pri",        REG_STK,              REG_PRI | REG_STK},
  {"pop.alt",        REG_STK,              REG_ALT | REG_STK},
  {"stack",          REG_STK,              REG_ALT | REG_STK},
  {"heap",           REG_HEA,              REG_ALT | REG_HEA},
  {"proc",           REG_STK | REG_FRM,    REG_STK | REG_FRM},
  {"ret",            REG_STK,              REG_STK | REG_FRM |
                                           REG_CIP},
  {"retn",           REG_STK,              REG_STK | REG_FRM |
                                           REG_CIP},
  {"call",           REG_STK | REG_CIP,    REG_PRI | REG_ALT |
                                           REG_STK | REG_CIP},
  {"call.pri",       REG_PRI | REG_STK |
                     REG_CIP,              REG_STK | REG_CIP},
  {"jump",           REG_NONE,             REG_CIP},
  {"jrel",           REG_NONE,             REG_NONE},
  {"jzer",           REG_PRI,              REG_CIP},
  {"jnz",            REG_PRI,              REG_CIP},
  {"jeq",            REG_PRI | REG_ALT,    REG_CIP},
  {"jneq",           REG_PRI | REG_ALT,    REG_CIP},
  {"jless",          REG_PRI | REG_ALT,    REG_CIP},
  {"jleq",           REG_PRI | REG_ALT,    REG_CIP},
  {"jgrtr",          REG_PRI | REG_ALT,    REG_CIP},
  {"jgeq",           REG_PRI | REG_ALT,    REG_CIP},
  {"jsless",         REG_PRI | REG_ALT,    REG_CIP},
  {"jsleq",          REG_PRI | REG_ALT,    REG_CIP},
  {"jsgrtr",         REG_PRI | REG_ALT,    REG_CIP},
  {"jsgeq",          REG_PRI | REG_ALT,    REG_CIP},
  {"shl",            REG_PRI | REG_ALT,    REG_PRI},
  {"shr",            REG_PRI | REG_ALT,    REG_PRI},
  {"sshr",           REG_PRI | REG_ALT,    REG_PRI},
  {"shl.c.pri",      REG_PRI,              REG_PRI},
  {"shl.c.alt",      REG_ALT,              REG_ALT},
  {"shr.c.pri",      REG_PRI,              REG_PRI},
  {"shr.c.alt",      REG_ALT,              REG_ALT},
  {"smul",           REG_PRI | REG_ALT,    REG_PRI},
  {"sdiv",           REG_PRI | REG_ALT,    REG_PRI},
  {"sdiv.alt",       REG_PRI | REG_ALT,    REG_PRI},
  {"umul",           REG_PRI | REG_ALT,    REG_PRI},
  {"udiv",           REG_PRI | REG_ALT,    REG_PRI},
  {"udiv.alt",       REG_PRI | REG_ALT,    REG_PRI},
  {"add",            REG_PRI | REG_ALT,    REG_PRI},
  {"sub",            REG_PRI | REG_ALT,    REG_PRI},
  {"sub.alt",        REG_PRI | REG_ALT,    REG_PRI},
  {"and",            REG_PRI | REG_ALT,    REG_PRI},
  {"or",             REG_PRI | REG_ALT,    REG_PRI},
  {"xor",            REG_PRI | REG_ALT,    REG_PRI},
  {"not",            REG_PRI,              REG_PRI},
  {"neg",            REG_PRI,              REG_PRI},
  {"invert",         REG_PRI,              REG_PRI},
  {"add.c",          REG_PRI,              REG_PRI},
  {"smul.c",         REG_PRI,              REG_PRI},
  {"zero.pri",       REG_NONE,             REG_PRI},
  {"zero.alt",       REG_NONE,             REG_PRI},
  {"zero",           REG_NONE,             REG_NONE},
  {"zero.s",         REG_FRM,              REG_NONE},
  {"sign.pri",       REG_PRI,              REG_PRI},
  {"sign.alt",       REG_ALT,              REG_ALT},
  {"eq",             REG_PRI | REG_ALT,    REG_PRI},
  {"neq",            REG_PRI | REG_ALT,    REG_PRI},
  {"less",           REG_PRI | REG_ALT,    REG_PRI},
  {"leq",            REG_PRI | REG_ALT,    REG_PRI},
  {"grtr",           REG_PRI | REG_ALT,    REG_PRI},
  {"geq",            REG_PRI | REG_ALT,    REG_PRI},
  {"sless",          REG_PRI | REG_ALT,    REG_PRI},
  {"sleq",           REG_PRI | REG_ALT,    REG_PRI},
  {"sgrtr",          REG_PRI | REG_ALT,    REG_PRI},
  {"sgeq",           REG_PRI | REG_ALT,    REG_PRI},
  {"eq.c.pri",       REG_PRI,              REG_PRI},
  {"eq.c.alt",       REG_ALT,              REG_PRI},
  {"inc.pri",        REG_PRI,              REG_PRI},
  {"inc.alt",        REG_ALT,              REG_ALT},
  {"inc",            REG_NONE,             REG_NONE},
  {"inc.s",          REG_FRM,              REG_NONE},
  {"inc.i",          REG_PRI,              REG_NONE},
  {"dec.pri",        REG_PRI,              REG_PRI},
  {"dec.alt",        REG_ALT,              REG_ALT},
  {"dec",            REG_NONE,             REG_NONE},
  {"dec.s",          REG_FRM,              REG_NONE},
  {"dec.i",          REG_PRI,              REG_NONE},
  {"movs",           REG_PRI | REG_ALT,    REG_NONE},
  {"cmps",           REG_PRI | REG_ALT,    REG_NONE},
  {"fill",           REG_PRI | REG_ALT,    REG_NONE},
  {"halt",           REG_PRI,              REG_NONE},
  {"bounds",         REG_PRI,              REG_NONE},
  {"sysreq.pri",     REG_PRI,              REG_PRI | REG_ALT |
                                           REG_COD | REG_DAT |
                                           REG_HEA | REG_STP |
                                           REG_STK | REG_FRM |
                                           REG_CIP},
  {"sysreq.c",       REG_NONE,             REG_PRI | REG_ALT |
                                           REG_COD | REG_DAT |
                                           REG_HEA | REG_STP |
                                           REG_STK | REG_FRM |
                                           REG_CIP},
  {"file",           REG_NONE,             REG_NONE},
  {"line",           REG_NONE,             REG_NONE},
  {"symbol",         REG_NONE,             REG_NONE},
  {"srange",         REG_NONE,             REG_NONE},
  {"jump.pri",       REG_PRI,              REG_CIP},
  {"switch",         REG_PRI,              REG_CIP},
  {"casetbl",        REG_NONE,             REG_NONE},
  {"swap.pri",       REG_PRI | REG_STK,    REG_PRI},
  {"swap.alt",       REG_ALT | REG_STK,    REG_ALT},
  {"push.adr",       REG_STK | REG_FRM,    REG_STK},
  {"nop",            REG_NONE,             REG_NONE},
  {"sysreq.d",       REG_NONE,             REG_PRI | REG_ALT |
                                           REG_COD | REG_DAT |
                                           REG_HEA | REG_STP |
                                           REG_STK | REG_FRM |
                                           REG_CIP},
  {"symtag",         REG_NONE,             REG_NONE},
  {"break",          REG_NONE,             REG_NONE}
};

AMXInstruction::AMXInstruction() 
  : address_(0),
    opcode_(AMX_OP_NONE)
{
}

const char *AMXInstruction::name() const {
  if (opcode_.id() >= 0 && opcode_.id() < NUM_AMX_OPCODES) {
    return info[opcode_.id()].name;
  }
  return 0;
}

int AMXInstruction::get_src_regs() const {
  if (opcode_.id() >= 0 && opcode_.id() < NUM_AMX_OPCODES) {
    return info[opcode_.id()].src_regs;
  }
  return 0;
}

int AMXInstruction::get_dst_regs() const {
  if (opcode_.id() >= 0 && opcode_.id() < NUM_AMX_OPCODES) {
    return info[opcode_.id()].dst_regs;
  }
  return 0;
}

std::string AMXInstruction::string() const {
  std::stringstream stream;
  if (name() != 0) {
    stream << name();
  } else {
    stream << std::setw(8) << std::setfill('0') << std::hex << opcode_.id();
  }
  for (std::vector<cell>::const_iterator iterator = operands_.begin();
      iterator != operands_.end(); ++iterator) {
    stream << ' ';
    cell oper = *iterator;
    if (oper < 0 || oper > 9) {
      stream << "0x" << std::hex;
    } else {
      stream << std::dec;
    }
    stream << oper;
  }
  return stream.str();
}

AMXDisassembler::AMXDisassembler(AMXPtr amx)
  : amx_(amx),
    ip_(0)
{
}

bool AMXDisassembler::decode(AMXInstruction &instr, bool *error) {
  if (error != 0) {
    *error = false;
  }

  if (ip_ < 0 || amx_.hdr()->cod + ip_ >= amx_.hdr()->dat) {
    // Went out of the code section.
    return false;
  }

  instr.operands().clear();
  instr.set_address(ip_);

  AMXOpcode opcode(*reinterpret_cast<cell*>(amx_.code() + ip_));

  ip_ += sizeof(cell);
  instr.set_opcode(opcode);

  switch (opcode.id()) {
    // Instructions with one operand.
    case AMX_OP_LOAD_PRI:   case AMX_OP_LOAD_ALT:   case AMX_OP_LOAD_S_PRI:
    case AMX_OP_LOAD_S_ALT: case AMX_OP_LREF_PRI:   case AMX_OP_LREF_ALT:
    case AMX_OP_LREF_S_PRI: case AMX_OP_LREF_S_ALT: case AMX_OP_LODB_I:
    case AMX_OP_CONST_PRI:  case AMX_OP_CONST_ALT:  case AMX_OP_ADDR_PRI:
    case AMX_OP_ADDR_ALT:   case AMX_OP_STOR_PRI:   case AMX_OP_STOR_ALT:
    case AMX_OP_STOR_S_PRI: case AMX_OP_STOR_S_ALT: case AMX_OP_SREF_PRI:
    case AMX_OP_SREF_ALT:   case AMX_OP_SREF_S_PRI: case AMX_OP_SREF_S_ALT:
    case AMX_OP_STRB_I:     case AMX_OP_LIDX_B:     case AMX_OP_IDXADDR_B:
    case AMX_OP_ALIGN_PRI:  case AMX_OP_ALIGN_ALT:  case AMX_OP_LCTRL:
    case AMX_OP_SCTRL:      case AMX_OP_PUSH_R:     case AMX_OP_PUSH_C:
    case AMX_OP_PUSH:       case AMX_OP_PUSH_S:     case AMX_OP_STACK:
    case AMX_OP_HEAP:       case AMX_OP_JREL:       case AMX_OP_JUMP:
    case AMX_OP_JZER:       case AMX_OP_JNZ:        case AMX_OP_JEQ:
    case AMX_OP_JNEQ:       case AMX_OP_JLESS:      case AMX_OP_JLEQ:
    case AMX_OP_JGRTR:      case AMX_OP_JGEQ:       case AMX_OP_JSLESS:
    case AMX_OP_JSLEQ:      case AMX_OP_JSGRTR:     case AMX_OP_JSGEQ:
    case AMX_OP_SHL_C_PRI:  case AMX_OP_SHL_C_ALT:  case AMX_OP_SHR_C_PRI:
    case AMX_OP_SHR_C_ALT:  case AMX_OP_ADD_C:      case AMX_OP_SMUL_C:
    case AMX_OP_ZERO:       case AMX_OP_ZERO_S:     case AMX_OP_EQ_C_PRI:
    case AMX_OP_EQ_C_ALT:   case AMX_OP_INC:        case AMX_OP_INC_S:
    case AMX_OP_DEC:        case AMX_OP_DEC_S:      case AMX_OP_MOVS:
    case AMX_OP_CMPS:       case AMX_OP_FILL:       case AMX_OP_HALT:
    case AMX_OP_BOUNDS:     case AMX_OP_CALL:       case AMX_OP_SYSREQ_C:
    case AMX_OP_PUSH_ADR:   case AMX_OP_SYSREQ_D:   case AMX_OP_SWITCH:
      instr.add_operand(*reinterpret_cast<cell*>(amx_.code() + ip_));
      ip_ += sizeof(cell);
      break;

    // Instructions with no operands.
    case AMX_OP_LOAD_I:     case AMX_OP_STOR_I:     case AMX_OP_LIDX:
    case AMX_OP_IDXADDR:    case AMX_OP_MOVE_PRI:   case AMX_OP_MOVE_ALT:
    case AMX_OP_XCHG:       case AMX_OP_PUSH_PRI:   case AMX_OP_PUSH_ALT:
    case AMX_OP_POP_PRI:    case AMX_OP_POP_ALT:    case AMX_OP_PROC:
    case AMX_OP_RET:        case AMX_OP_RETN:       case AMX_OP_CALL_PRI:
    case AMX_OP_SHL:        case AMX_OP_SHR:        case AMX_OP_SSHR:
    case AMX_OP_SMUL:       case AMX_OP_SDIV:       case AMX_OP_SDIV_ALT:
    case AMX_OP_UMUL:       case AMX_OP_UDIV:       case AMX_OP_UDIV_ALT:
    case AMX_OP_ADD:        case AMX_OP_SUB:        case AMX_OP_SUB_ALT:
    case AMX_OP_AND:        case AMX_OP_OR:         case AMX_OP_XOR:
    case AMX_OP_NOT:        case AMX_OP_NEG:        case AMX_OP_INVERT:
    case AMX_OP_ZERO_PRI:   case AMX_OP_ZERO_ALT:   case AMX_OP_SIGN_PRI:
    case AMX_OP_SIGN_ALT:   case AMX_OP_EQ:         case AMX_OP_NEQ:
    case AMX_OP_LESS:       case AMX_OP_LEQ:        case AMX_OP_GRTR:
    case AMX_OP_GEQ:        case AMX_OP_SLESS:      case AMX_OP_SLEQ:
    case AMX_OP_SGRTR:      case AMX_OP_SGEQ:       case AMX_OP_INC_PRI:
    case AMX_OP_INC_ALT:    case AMX_OP_INC_I:      case AMX_OP_DEC_PRI:
    case AMX_OP_DEC_ALT:    case AMX_OP_DEC_I:      case AMX_OP_SYSREQ_PRI:
    case AMX_OP_JUMP_PRI:   case AMX_OP_SWAP_PRI:   case AMX_OP_SWAP_ALT:
    case AMX_OP_NOP:        case AMX_OP_BREAK:
      break;

    // Special instructions.
    case AMX_OP_CASETBL: {
      int num = *reinterpret_cast<cell*>(amx_.code() + ip_) + 1;
      // num case records follow, each is 2 cells big.
      for (int i = 0; i < num * 2; i++) {
        instr.add_operand(*reinterpret_cast<cell*>(amx_.code() + ip_));
        ip_ += sizeof(cell);
      }
      break;
    }

    default:
      if (error != 0) {
        *error = true;
      }
      return false;
  }

  return true;
}

} // namespace amxjit