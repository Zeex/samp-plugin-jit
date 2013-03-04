// Copyright (c) 2012 Zeex
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

#include "compiler.h"
#include "disasm.h"

namespace amxjit {

CompilerOutput *Compiler::compile(AMXPtr amx, CompileErrorHandler *error_handler) {
  setup(amx);

  Disassembler disas(amx);
  Instruction instr;
  bool error = false;

  set_instr(&instr);

  while (!error && disas.decode(instr, &error)) {
    if (!process(instr)) {
      error = true;
      break;
    }

    switch (instr.opcode().id()) {
      case OP_LOAD_PRI:
        emit_load_pri(instr.operand());
        break;
      case OP_LOAD_ALT:
        emit_load_alt(instr.operand());
        break;
      case OP_LOAD_S_PRI:
        emit_load_s_pri(instr.operand());
        break;
      case OP_LOAD_S_ALT:
        emit_load_s_alt(instr.operand());
        break;
      case OP_LREF_PRI:
        emit_lref_pri(instr.operand());
        break;
      case OP_LREF_ALT:
        emit_lref_alt(instr.operand());
        break;
      case OP_LREF_S_PRI:
        emit_lref_s_pri(instr.operand());
        break;
      case OP_LREF_S_ALT:
        emit_lref_s_alt(instr.operand());
        break;
      case OP_LOAD_I:
        emit_load_i();
        break;
      case OP_LODB_I:
        emit_lodb_i(instr.operand());
        break;
      case OP_CONST_PRI:
        emit_const_pri(instr.operand());
        break;
      case OP_CONST_ALT:
        emit_const_alt(instr.operand());
        break;
      case OP_ADDR_PRI:
        emit_addr_pri(instr.operand());
        break;
      case OP_ADDR_ALT:
        emit_addr_alt(instr.operand());
        break;
      case OP_STOR_PRI:
        emit_stor_pri(instr.operand());
        break;
      case OP_STOR_ALT:
        emit_stor_alt(instr.operand());
        break;
      case OP_STOR_S_PRI:
        emit_stor_s_pri(instr.operand());
        break;
      case OP_STOR_S_ALT:
        emit_stor_s_alt(instr.operand());
        break;
      case OP_SREF_PRI:
        emit_sref_pri(instr.operand());
        break;
      case OP_SREF_ALT:
        emit_sref_alt(instr.operand());
        break;
      case OP_SREF_S_PRI:
        emit_sref_s_pri(instr.operand());
        break;
      case OP_SREF_S_ALT:
        emit_sref_s_alt(instr.operand());
        break;
      case OP_STOR_I:
        emit_stor_i();
        break;
      case OP_STRB_I:
        emit_strb_i(instr.operand());
        break;
      case OP_LIDX:
        emit_lidx();
        break;
      case OP_LIDX_B:
        emit_lidx_b(instr.operand());
        break;
      case OP_IDXADDR:
        emit_idxaddr();
        break;
      case OP_IDXADDR_B:
        emit_idxaddr_b(instr.operand());
        break;
      case OP_ALIGN_PRI:
        emit_align_pri(instr.operand());
        break;
      case OP_ALIGN_ALT:
        emit_align_alt(instr.operand());
        break;
      case OP_LCTRL:
        emit_lctrl(instr.operand());
        break;
      case OP_SCTRL:
        emit_sctrl(instr.operand());
        break;
      case OP_MOVE_PRI:
        emit_move_pri();
        break;
      case OP_MOVE_ALT:
        emit_move_alt();
        break;
      case OP_XCHG:
        emit_xchg();
        break;
      case OP_PUSH_PRI:
        emit_push_pri();
        break;
      case OP_PUSH_ALT:
        emit_push_alt();
        break;
      case OP_PUSH_C:
        emit_push_c(instr.operand());
        break;
      case OP_PUSH:
        emit_push(instr.operand());
        break;
      case OP_PUSH_S:
        emit_push_s(instr.operand());
        break;
      case OP_POP_PRI:
        emit_pop_pri();
        break;
      case OP_POP_ALT:
        emit_pop_alt();
        break;
      case OP_STACK: // value
        emit_stack(instr.operand());
        break;
      case OP_HEAP:
        emit_heap(instr.operand());
        break;
      case OP_PROC:
        emit_proc();
        break;
      case OP_RET:
        emit_ret();
        break;
      case OP_RETN:
        emit_retn();
        break;
      case OP_JUMP_PRI:
        emit_jump_pri();
        break;
      case OP_CALL:
      case OP_JUMP:
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
      case OP_JSGEQ: {
        cell dest = instr.operand() - reinterpret_cast<cell>(amx.code());
        switch (instr.opcode().id()) {
          case OP_CALL:
            emit_call(dest);
            break;
          case OP_JUMP:
            emit_jump(dest);
            break;
          case OP_JZER:
            emit_jzer(dest);
            break;
          case OP_JNZ:
            emit_jnz(dest);
            break;
          case OP_JEQ:
            emit_jeq(dest);
            break;
          case OP_JNEQ:
            emit_jneq(dest);
            break;
          case OP_JLESS:
            emit_jless(dest);
            break;
          case OP_JLEQ:
            emit_jleq(dest);
            break;
          case OP_JGRTR:
            emit_jgrtr(dest);
            break;
          case OP_JGEQ:
            emit_jgeq(dest);
            break;
          case OP_JSLESS:
            emit_jsless(dest);
            break;
          case OP_JSLEQ:
            emit_jsleq(dest);
            break;
          case OP_JSGRTR:
            emit_jsgrtr(dest);
            break;
          case OP_JSGEQ:
            emit_jsgeq(dest);
            break;
        }
        break;
      }
      case OP_SHL:
        emit_shl();
        break;
      case OP_SHR:
        emit_shr();
        break;
      case OP_SSHR:
        emit_sshr();
        break;
      case OP_SHL_C_PRI:
        emit_shl_c_pri(instr.operand());
        break;
      case OP_SHL_C_ALT:
        emit_shl_c_alt(instr.operand());
        break;
      case OP_SHR_C_PRI:
        emit_shr_c_pri(instr.operand());
        break;
      case OP_SHR_C_ALT:
        emit_shr_c_alt(instr.operand());
        break;
      case OP_SMUL:
        emit_smul();
        break;
      case OP_SDIV:
        emit_sdiv();
        break;
      case OP_SDIV_ALT:
        emit_sdiv_alt();
        break;
      case OP_UMUL:
        emit_umul();
        break;
      case OP_UDIV:
        emit_udiv();
        break;
      case OP_UDIV_ALT:
        emit_udiv_alt();
        break;
      case OP_ADD:
        emit_add();
        break;
      case OP_SUB:
        emit_sub();
        break;
      case OP_SUB_ALT:
        emit_sub_alt();
        break;
      case OP_AND:
        emit_and();
        break;
      case OP_OR:
        emit_or();
        break;
      case OP_XOR:
        emit_xor();
        break;
      case OP_NOT:
        emit_not();
        break;
      case OP_NEG:
        emit_neg();
        break;
      case OP_INVERT:
        emit_invert();
        break;
      case OP_ADD_C:
        emit_add_c(instr.operand());
        break;
      case OP_SMUL_C:
        emit_smul_c(instr.operand());
        break;
      case OP_ZERO_PRI:
        emit_zero_pri();
        break;
      case OP_ZERO_ALT:
        emit_zero_alt();
        break;
      case OP_ZERO:
        emit_zero(instr.operand());
        break;
      case OP_ZERO_S:
        emit_zero_s(instr.operand());
        break;
      case OP_SIGN_PRI:
        emit_sign_pri();
        break;
      case OP_SIGN_ALT:
        emit_sign_alt();
        break;
      case OP_EQ:
        emit_eq();
        break;
      case OP_NEQ:
        emit_neq();
        break;
      case OP_LESS:
        emit_less();
        break;
      case OP_LEQ:
        emit_leq();
        break;
      case OP_GRTR:
        emit_grtr();
        break;
      case OP_GEQ:
        emit_geq();
        break;
      case OP_SLESS:
        emit_sless();
        break;
      case OP_SLEQ:
        emit_sleq();
        break;
      case OP_SGRTR:
        emit_sgrtr();
        break;
      case OP_SGEQ:
        emit_sgeq();
        break;
      case OP_EQ_C_PRI:
        emit_eq_c_pri(instr.operand());
        break;
      case OP_EQ_C_ALT:
        emit_eq_c_alt(instr.operand());
        break;
      case OP_INC_PRI:
        emit_inc_pri();
        break;
      case OP_INC_ALT:
        emit_inc_alt();
        break;
      case OP_INC:
        emit_inc(instr.operand());
        break;
      case OP_INC_S:
        emit_inc_s(instr.operand());
        break;
      case OP_INC_I:
        emit_inc_i();
        break;
      case OP_DEC_PRI:
        emit_dec_pri();
        break;
      case OP_DEC_ALT:
        emit_dec_alt();
        break;
      case OP_DEC:
        emit_dec(instr.operand());
        break;
      case OP_DEC_S:
        emit_dec_s(instr.operand());
        break;
      case OP_DEC_I:
        emit_dec_i();
        break;
      case OP_MOVS:
        emit_movs(instr.operand());
        break;
      case OP_CMPS:
        emit_cmps(instr.operand());
        break;
      case OP_FILL:
        emit_fill(instr.operand());
        break;
      case OP_HALT:
        emit_halt(instr.operand());
        break;
      case OP_BOUNDS:
        emit_bounds(instr.operand());
        break;
      case OP_SYSREQ_PRI:
        emit_sysreq_pri();
        break;
      case OP_SYSREQ_C: {
        const char *name = amx.get_native_name(instr.operand());
        if (name == 0) {
          error = true;
        } else {
          emit_sysreq_c(instr.operand(), name);
          }
        break;
      }
      case OP_SYSREQ_D: {
        const char *name = amx.get_native_name(amx.find_native(instr.operand()));
        if (name == 0) {
          error = true;
        } else {
          emit_sysreq_d(instr.operand(), name);
        }
        break;
      }
      case OP_SWITCH:
        emit_switch(CaseTable(amx, instr.operand()));
        break;
      case OP_CASETBL:
        emit_casetbl();
        break;
      case OP_SWAP_PRI:
        emit_swap_pri();
        break;
      case OP_SWAP_ALT:
        emit_swap_alt();
        break;
      case OP_PUSH_ADR:
        emit_push_adr(instr.operand());
        break;
      case OP_NOP:
        emit_nop();
        break;
      case OP_BREAK:
        emit_break();
        break;
    default:
      error = true;
    }
  }

  if (error) {
    error_handler->execute(instr);
    abort();
    return 0;
  }

  return finish();
}

} // namespace amxjit
