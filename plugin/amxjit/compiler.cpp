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

#include "amxdisasm.h"
#include "compiler.h"

namespace amxjit {

CompilerOutput *Compiler::compile(AMXPtr amx, CompileErrorHandler *error_handler) {
  setup(amx);

  AMXDisassembler disas(amx);
  AMXInstruction instr;
  bool error = false;

  set_instr(&instr);

  while (!error && disas.decode(instr, &error)) {
    if (!process(instr)) {
      error = true;
      break;
    }

    switch (instr.opcode().id()) {
      case AMX_OP_LOAD_PRI:
        emit_load_pri(instr.operand());
        break;
      case AMX_OP_LOAD_ALT:
        emit_load_alt(instr.operand());
        break;
      case AMX_OP_LOAD_S_PRI:
        emit_load_s_pri(instr.operand());
        break;
      case AMX_OP_LOAD_S_ALT:
        emit_load_s_alt(instr.operand());
        break;
      case AMX_OP_LREF_PRI:
        emit_lref_pri(instr.operand());
        break;
      case AMX_OP_LREF_ALT:
        emit_lref_alt(instr.operand());
        break;
      case AMX_OP_LREF_S_PRI:
        emit_lref_s_pri(instr.operand());
        break;
      case AMX_OP_LREF_S_ALT:
        emit_lref_s_alt(instr.operand());
        break;
      case AMX_OP_LOAD_I:
        emit_load_i();
        break;
      case AMX_OP_LODB_I:
        emit_lodb_i(instr.operand());
        break;
      case AMX_OP_CONST_PRI:
        emit_const_pri(instr.operand());
        break;
      case AMX_OP_CONST_ALT:
        emit_const_alt(instr.operand());
        break;
      case AMX_OP_ADDR_PRI:
        emit_addr_pri(instr.operand());
        break;
      case AMX_OP_ADDR_ALT:
        emit_addr_alt(instr.operand());
        break;
      case AMX_OP_STOR_PRI:
        emit_stor_pri(instr.operand());
        break;
      case AMX_OP_STOR_ALT:
        emit_stor_alt(instr.operand());
        break;
      case AMX_OP_STOR_S_PRI:
        emit_stor_s_pri(instr.operand());
        break;
      case AMX_OP_STOR_S_ALT:
        emit_stor_s_alt(instr.operand());
        break;
      case AMX_OP_SREF_PRI:
        emit_sref_pri(instr.operand());
        break;
      case AMX_OP_SREF_ALT:
        emit_sref_alt(instr.operand());
        break;
      case AMX_OP_SREF_S_PRI:
        emit_sref_s_pri(instr.operand());
        break;
      case AMX_OP_SREF_S_ALT:
        emit_sref_s_alt(instr.operand());
        break;
      case AMX_OP_STOR_I:
        emit_stor_i();
        break;
      case AMX_OP_STRB_I:
        emit_strb_i(instr.operand());
        break;
      case AMX_OP_LIDX:
        emit_lidx();
        break;
      case AMX_OP_LIDX_B:
        emit_lidx_b(instr.operand());
        break;
      case AMX_OP_IDXADDR:
        emit_idxaddr();
        break;
      case AMX_OP_IDXADDR_B:
        emit_idxaddr_b(instr.operand());
        break;
      case AMX_OP_ALIGN_PRI:
        emit_align_pri(instr.operand());
        break;
      case AMX_OP_ALIGN_ALT:
        emit_align_alt(instr.operand());
        break;
      case AMX_OP_LCTRL:
        emit_lctrl(instr.operand());
        break;
      case AMX_OP_SCTRL:
        emit_sctrl(instr.operand());
        break;
      case AMX_OP_MOVE_PRI:
        emit_move_pri();
        break;
      case AMX_OP_MOVE_ALT:
        emit_move_alt();
        break;
      case AMX_OP_XCHG:
        emit_xchg();
        break;
      case AMX_OP_PUSH_PRI:
        emit_push_pri();
        break;
      case AMX_OP_PUSH_ALT:
        emit_push_alt();
        break;
      case AMX_OP_PUSH_C:
        emit_push_c(instr.operand());
        break;
      case AMX_OP_PUSH:
        emit_push(instr.operand());
        break;
      case AMX_OP_PUSH_S:
        emit_push_s(instr.operand());
        break;
      case AMX_OP_POP_PRI:
        emit_pop_pri();
        break;
      case AMX_OP_POP_ALT:
        emit_pop_alt();
        break;
      case AMX_OP_STACK: // value
        emit_stack(instr.operand());
        break;
      case AMX_OP_HEAP:
        emit_heap(instr.operand());
        break;
      case AMX_OP_PROC:
        emit_proc();
        break;
      case AMX_OP_RET:
        emit_ret();
        break;
      case AMX_OP_RETN:
        emit_retn();
        break;
      case AMX_OP_JUMP_PRI:
        emit_jump_pri();
        break;
      case AMX_OP_CALL:
      case AMX_OP_JUMP:
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
      case AMX_OP_JSGEQ: {
        cell dest = instr.operand() - reinterpret_cast<cell>(amx.code());
        switch (instr.opcode().id()) {
          case AMX_OP_CALL:
            emit_call(dest);
            break;
          case AMX_OP_JUMP:
            emit_jump(dest);
            break;
          case AMX_OP_JZER:
            emit_jzer(dest);
            break;
          case AMX_OP_JNZ:
            emit_jnz(dest);
            break;
          case AMX_OP_JEQ:
            emit_jeq(dest);
            break;
          case AMX_OP_JNEQ:
            emit_jneq(dest);
            break;
          case AMX_OP_JLESS:
            emit_jless(dest);
            break;
          case AMX_OP_JLEQ:
            emit_jleq(dest);
            break;
          case AMX_OP_JGRTR:
            emit_jgrtr(dest);
            break;
          case AMX_OP_JGEQ:
            emit_jgeq(dest);
            break;
          case AMX_OP_JSLESS:
            emit_jsless(dest);
            break;
          case AMX_OP_JSLEQ:
            emit_jsleq(dest);
            break;
          case AMX_OP_JSGRTR:
            emit_jsgrtr(dest);
            break;
          case AMX_OP_JSGEQ:
            emit_jsgeq(dest);
            break;
        }
        break;
      }
      case AMX_OP_SHL:
        emit_shl();
        break;
      case AMX_OP_SHR:
        emit_shr();
        break;
      case AMX_OP_SSHR:
        emit_sshr();
        break;
      case AMX_OP_SHL_C_PRI:
        emit_shl_c_pri(instr.operand());
        break;
      case AMX_OP_SHL_C_ALT:
        emit_shl_c_alt(instr.operand());
        break;
      case AMX_OP_SHR_C_PRI:
        emit_shr_c_pri(instr.operand());
        break;
      case AMX_OP_SHR_C_ALT:
        emit_shr_c_alt(instr.operand());
        break;
      case AMX_OP_SMUL:
        emit_smul();
        break;
      case AMX_OP_SDIV:
        emit_sdiv();
        break;
      case AMX_OP_SDIV_ALT:
        emit_sdiv_alt();
        break;
      case AMX_OP_UMUL:
        emit_umul();
        break;
      case AMX_OP_UDIV:
        emit_udiv();
        break;
      case AMX_OP_UDIV_ALT:
        emit_udiv_alt();
        break;
      case AMX_OP_ADD:
        emit_add();
        break;
      case AMX_OP_SUB:
        emit_sub();
        break;
      case AMX_OP_SUB_ALT:
        emit_sub_alt();
        break;
      case AMX_OP_AND:
        emit_and();
        break;
      case AMX_OP_OR:
        emit_or();
        break;
      case AMX_OP_XOR:
        emit_xor();
        break;
      case AMX_OP_NOT:
        emit_not();
        break;
      case AMX_OP_NEG:
        emit_neg();
        break;
      case AMX_OP_INVERT:
        emit_invert();
        break;
      case AMX_OP_ADD_C:
        emit_add_c(instr.operand());
        break;
      case AMX_OP_SMUL_C:
        emit_smul_c(instr.operand());
        break;
      case AMX_OP_ZERO_PRI:
        emit_zero_pri();
        break;
      case AMX_OP_ZERO_ALT:
        emit_zero_alt();
        break;
      case AMX_OP_ZERO:
        emit_zero(instr.operand());
        break;
      case AMX_OP_ZERO_S:
        emit_zero_s(instr.operand());
        break;
      case AMX_OP_SIGN_PRI:
        emit_sign_pri();
        break;
      case AMX_OP_SIGN_ALT:
        emit_sign_alt();
        break;
      case AMX_OP_EQ:
        emit_eq();
        break;
      case AMX_OP_NEQ:
        emit_neq();
        break;
      case AMX_OP_LESS:
        emit_less();
        break;
      case AMX_OP_LEQ:
        emit_leq();
        break;
      case AMX_OP_GRTR:
        emit_grtr();
        break;
      case AMX_OP_GEQ:
        emit_geq();
        break;
      case AMX_OP_SLESS:
        emit_sless();
        break;
      case AMX_OP_SLEQ:
        emit_sleq();
        break;
      case AMX_OP_SGRTR:
        emit_sgrtr();
        break;
      case AMX_OP_SGEQ:
        emit_sgeq();
        break;
      case AMX_OP_EQ_C_PRI:
        emit_eq_c_pri(instr.operand());
        break;
      case AMX_OP_EQ_C_ALT:
        emit_eq_c_alt(instr.operand());
        break;
      case AMX_OP_INC_PRI:
        emit_inc_pri();
        break;
      case AMX_OP_INC_ALT:
        emit_inc_alt();
        break;
      case AMX_OP_INC:
        emit_inc(instr.operand());
        break;
      case AMX_OP_INC_S:
        emit_inc_s(instr.operand());
        break;
      case AMX_OP_INC_I:
        emit_inc_i();
        break;
      case AMX_OP_DEC_PRI:
        emit_dec_pri();
        break;
      case AMX_OP_DEC_ALT:
        emit_dec_alt();
        break;
      case AMX_OP_DEC:
        emit_dec(instr.operand());
        break;
      case AMX_OP_DEC_S:
        emit_dec_s(instr.operand());
        break;
      case AMX_OP_DEC_I:
        emit_dec_i();
        break;
      case AMX_OP_MOVS:
        emit_movs(instr.operand());
        break;
      case AMX_OP_CMPS:
        emit_cmps(instr.operand());
        break;
      case AMX_OP_FILL:
        emit_fill(instr.operand());
        break;
      case AMX_OP_HALT:
        emit_halt(instr.operand());
        break;
      case AMX_OP_BOUNDS:
        emit_bounds(instr.operand());
        break;
      case AMX_OP_SYSREQ_PRI:
        emit_sysreq_pri();
        break;
      case AMX_OP_SYSREQ_C: {
        const char *name = amx.get_native_name(instr.operand());
        if (name == 0) {
          error = true;
        } else {
          emit_sysreq_c(instr.operand(), name);
          }
        break;
      }
      case AMX_OP_SYSREQ_D: {
        const char *name = amx.get_native_name(amx.find_native(instr.operand()));
        if (name == 0) {
          error = true;
        } else {
          emit_sysreq_d(instr.operand(), name);
        }
        break;
      }
      case AMX_OP_SWITCH:
        emit_switch(AMXCaseTable(amx, instr.operand()));
        break;
      case AMX_OP_CASETBL:
        emit_casetbl();
        break;
      case AMX_OP_SWAP_PRI:
        emit_swap_pri();
        break;
      case AMX_OP_SWAP_ALT:
        emit_swap_alt();
        break;
      case AMX_OP_PUSH_ADR:
        emit_push_adr(instr.operand());
        break;
      case AMX_OP_NOP:
        emit_nop();
        break;
      case AMX_OP_BREAK:
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
