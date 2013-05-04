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

#include <string>
#include "compiler.h"
#include "disasm.h"

namespace amxjit {

CompilerOutput *Compiler::Compile(AMXPtr amx, CompileErrorHandler *errorHandler) {
  Instruction instr;

  compiling = true;
  currentAmx = amx;
  currentInstr = &instr;

  Setup();

  Disassembler disas(amx);
  bool error = false;

  while (!error && disas.Decode(instr, &error)) {
    if (!Process(instr)) {
      error = true;
      break;
    }

    switch (instr.GetOpcode().GetId()) {
      case OP_LOAD_PRI:
        load_pri(instr.GetOperand());
        break;
      case OP_LOAD_ALT:
        load_alt(instr.GetOperand());
        break;
      case OP_LOAD_S_PRI:
        load_s_pri(instr.GetOperand());
        break;
      case OP_LOAD_S_ALT:
        load_s_alt(instr.GetOperand());
        break;
      case OP_LREF_PRI:
        lref_pri(instr.GetOperand());
        break;
      case OP_LREF_ALT:
        lref_alt(instr.GetOperand());
        break;
      case OP_LREF_S_PRI:
        lref_s_pri(instr.GetOperand());
        break;
      case OP_LREF_S_ALT:
        lref_s_alt(instr.GetOperand());
        break;
      case OP_LOAD_I:
        load_i();
        break;
      case OP_LODB_I:
        lodb_i(instr.GetOperand());
        break;
      case OP_CONST_PRI:
        const_pri(instr.GetOperand());
        break;
      case OP_CONST_ALT:
        const_alt(instr.GetOperand());
        break;
      case OP_ADDR_PRI:
        addr_pri(instr.GetOperand());
        break;
      case OP_ADDR_ALT:
        addr_alt(instr.GetOperand());
        break;
      case OP_STOR_PRI:
        stor_pri(instr.GetOperand());
        break;
      case OP_STOR_ALT:
        stor_alt(instr.GetOperand());
        break;
      case OP_STOR_S_PRI:
        stor_s_pri(instr.GetOperand());
        break;
      case OP_STOR_S_ALT:
        stor_s_alt(instr.GetOperand());
        break;
      case OP_SREF_PRI:
        sref_pri(instr.GetOperand());
        break;
      case OP_SREF_ALT:
        sref_alt(instr.GetOperand());
        break;
      case OP_SREF_S_PRI:
        sref_s_pri(instr.GetOperand());
        break;
      case OP_SREF_S_ALT:
        sref_s_alt(instr.GetOperand());
        break;
      case OP_STOR_I:
        stor_i();
        break;
      case OP_STRB_I:
        strb_i(instr.GetOperand());
        break;
      case OP_LIDX:
        lidx();
        break;
      case OP_LIDX_B:
        lidx_b(instr.GetOperand());
        break;
      case OP_IDXADDR:
        idxaddr();
        break;
      case OP_IDXADDR_B:
        idxaddr_b(instr.GetOperand());
        break;
      case OP_ALIGN_PRI:
        align_pri(instr.GetOperand());
        break;
      case OP_ALIGN_ALT:
        align_alt(instr.GetOperand());
        break;
      case OP_LCTRL:
        lctrl(instr.GetOperand());
        break;
      case OP_SCTRL:
        sctrl(instr.GetOperand());
        break;
      case OP_MOVE_PRI:
        move_pri();
        break;
      case OP_MOVE_ALT:
        move_alt();
        break;
      case OP_XCHG:
        xchg();
        break;
      case OP_PUSH_PRI:
        push_pri();
        break;
      case OP_PUSH_ALT:
        push_alt();
        break;
      case OP_PUSH_C:
        push_c(instr.GetOperand());
        break;
      case OP_PUSH:
        push(instr.GetOperand());
        break;
      case OP_PUSH_S:
        push_s(instr.GetOperand());
        break;
      case OP_POP_PRI:
        pop_pri();
        break;
      case OP_POP_ALT:
        pop_alt();
        break;
      case OP_STACK: // value
        stack(instr.GetOperand());
        break;
      case OP_HEAP:
        heap(instr.GetOperand());
        break;
      case OP_PROC:
        proc();
        break;
      case OP_RET:
        ret();
        break;
      case OP_RETN:
        retn();
        break;
      case OP_JUMP_PRI:
        jump_pri();
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
        cell dest = instr.GetOperand() - reinterpret_cast<cell>(amx.GetCode());
        switch (instr.GetOpcode().GetId()) {
          case OP_CALL:
            call(dest);
            break;
          case OP_JUMP:
            jump(dest);
            break;
          case OP_JZER:
            jzer(dest);
            break;
          case OP_JNZ:
            jnz(dest);
            break;
          case OP_JEQ:
            jeq(dest);
            break;
          case OP_JNEQ:
            jneq(dest);
            break;
          case OP_JLESS:
            jless(dest);
            break;
          case OP_JLEQ:
            jleq(dest);
            break;
          case OP_JGRTR:
            jgrtr(dest);
            break;
          case OP_JGEQ:
            jgeq(dest);
            break;
          case OP_JSLESS:
            jsless(dest);
            break;
          case OP_JSLEQ:
            jsleq(dest);
            break;
          case OP_JSGRTR:
            jsgrtr(dest);
            break;
          case OP_JSGEQ:
            jsgeq(dest);
            break;
        }
        break;
      }
      case OP_SHL:
        shl();
        break;
      case OP_SHR:
        shr();
        break;
      case OP_SSHR:
        sshr();
        break;
      case OP_SHL_C_PRI:
        shl_c_pri(instr.GetOperand());
        break;
      case OP_SHL_C_ALT:
        shl_c_alt(instr.GetOperand());
        break;
      case OP_SHR_C_PRI:
        shr_c_pri(instr.GetOperand());
        break;
      case OP_SHR_C_ALT:
        shr_c_alt(instr.GetOperand());
        break;
      case OP_SMUL:
        smul();
        break;
      case OP_SDIV:
        sdiv();
        break;
      case OP_SDIV_ALT:
        sdiv_alt();
        break;
      case OP_UMUL:
        umul();
        break;
      case OP_UDIV:
        udiv();
        break;
      case OP_UDIV_ALT:
        udiv_alt();
        break;
      case OP_ADD:
        add();
        break;
      case OP_SUB:
        sub();
        break;
      case OP_SUB_ALT:
        sub_alt();
        break;
      case OP_AND:
        and_();
        break;
      case OP_OR:
        or_();
        break;
      case OP_XOR:
        xor_();
        break;
      case OP_NOT:
        not_();
        break;
      case OP_NEG:
        neg();
        break;
      case OP_INVERT:
        invert();
        break;
      case OP_ADD_C:
        add_c(instr.GetOperand());
        break;
      case OP_SMUL_C:
        smul_c(instr.GetOperand());
        break;
      case OP_ZERO_PRI:
        zero_pri();
        break;
      case OP_ZERO_ALT:
        zero_alt();
        break;
      case OP_ZERO:
        zero(instr.GetOperand());
        break;
      case OP_ZERO_S:
        zero_s(instr.GetOperand());
        break;
      case OP_SIGN_PRI:
        sign_pri();
        break;
      case OP_SIGN_ALT:
        sign_alt();
        break;
      case OP_EQ:
        eq();
        break;
      case OP_NEQ:
        neq();
        break;
      case OP_LESS:
        less();
        break;
      case OP_LEQ:
        leq();
        break;
      case OP_GRTR:
        grtr();
        break;
      case OP_GEQ:
        geq();
        break;
      case OP_SLESS:
        sless();
        break;
      case OP_SLEQ:
        sleq();
        break;
      case OP_SGRTR:
        sgrtr();
        break;
      case OP_SGEQ:
        sgeq();
        break;
      case OP_EQ_C_PRI:
        eq_c_pri(instr.GetOperand());
        break;
      case OP_EQ_C_ALT:
        eq_c_alt(instr.GetOperand());
        break;
      case OP_INC_PRI:
        inc_pri();
        break;
      case OP_INC_ALT:
        inc_alt();
        break;
      case OP_INC:
        inc(instr.GetOperand());
        break;
      case OP_INC_S:
        inc_s(instr.GetOperand());
        break;
      case OP_INC_I:
        inc_i();
        break;
      case OP_DEC_PRI:
        dec_pri();
        break;
      case OP_DEC_ALT:
        dec_alt();
        break;
      case OP_DEC:
        dec(instr.GetOperand());
        break;
      case OP_DEC_S:
        dec_s(instr.GetOperand());
        break;
      case OP_DEC_I:
        dec_i();
        break;
      case OP_MOVS:
        movs(instr.GetOperand());
        break;
      case OP_CMPS:
        cmps(instr.GetOperand());
        break;
      case OP_FILL:
        fill(instr.GetOperand());
        break;
      case OP_HALT:
        halt(instr.GetOperand());
        break;
      case OP_BOUNDS:
        bounds(instr.GetOperand());
        break;
      case OP_SYSREQ_PRI:
        sysreq_pri();
        break;
      case OP_SYSREQ_C: {
        const char *name = amx.GetNativeName(instr.GetOperand());
        if (name == 0) {
          error = true;
        } else {
          sysreq_c(instr.GetOperand(), name);
          }
        break;
      }
      case OP_SYSREQ_D: {
        const char *name = amx.GetNativeName(amx.FindNative(instr.GetOperand()));
        if (name == 0) {
          error = true;
        } else {
          sysreq_d(instr.GetOperand(), name);
        }
        break;
      }
      case OP_SWITCH:
        switch_(CaseTable(amx, instr.GetOperand()));
        break;
      case OP_CASETBL:
        casetbl();
        break;
      case OP_SWAP_PRI:
        swap_pri();
        break;
      case OP_SWAP_ALT:
        swap_alt();
        break;
      case OP_PUSH_ADR:
        push_adr(instr.GetOperand());
        break;
      case OP_NOP:
        nop();
        break;
      case OP_BREAK:
        break_();
        break;
    default:
      error = true;
    }
  }

  if (error) {
    errorHandler->Execute(instr);
    Abort();
    return 0;
  }

  CompilerOutput *output = Finish();

  compiling = false;
  return output;
}

} // namespace amxjit
