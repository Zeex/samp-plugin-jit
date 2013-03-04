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

#ifndef AMXJIT_COMPILER_H
#define AMXJIT_COMPILER_H

#include <cstddef>
#include "amxptr.h"
#include "callconv.h"
#include "macros.h"

namespace amxjit {

class CaseTable;
class Instruction;

typedef int (AMXJIT_CDECL *EntryPoint)(cell index, cell *retval);

class CompileErrorHandler {
public:
  virtual ~CompileErrorHandler() {}
  virtual void execute(const Instruction &instr) = 0;
};

class CompilerOutput {
 public:
  virtual ~CompilerOutput() {}

  // Returns a pointer to the code buffer.
  virtual void *code() const = 0;

  // Returns the size of the code in bytes.
  virtual std::size_t code_size() const = 0;

  // Returns a pointer to the entry point function.
  virtual EntryPoint entry_point() const = 0;
};

class Compiler {
 public:
  virtual ~Compiler() {}

  // Compiles the specified AMX script. The optional error hander is called at
  // most only once - on first compile error.
  CompilerOutput *compile(AMXPtr amx, CompileErrorHandler *error_handler = 0);

 protected:
  // This method is called just before the compilation begins.
  // Returns false on error.
  virtual bool setup(AMXPtr amx) = 0;

  // Processes a single instruction. Returns false on error.
  virtual bool process(const Instruction &instr) = 0;

  // This method is called on a fatal error.
  virtual void abort() = 0;

  // Final compilation step. This method shuld either return a runnable
  // CompilerOutput or null which would indicate a fatal error.
  virtual CompilerOutput *finish() = 0;

  // Sets/returns current instruction.
  void set_instr(const Instruction *instr) { instr_ = instr; }
  const Instruction *get_instr() const { return instr_; }

  // Per-opcode methods.
  virtual void emit_load_pri(cell address) = 0;
  virtual void emit_load_alt(cell address) = 0;
  virtual void emit_load_s_pri(cell offset) = 0;
  virtual void emit_load_s_alt(cell offset) = 0;
  virtual void emit_lref_pri(cell address) = 0;
  virtual void emit_lref_alt(cell address) = 0;
  virtual void emit_lref_s_pri(cell offset) = 0;
  virtual void emit_lref_s_alt(cell offset) = 0;
  virtual void emit_load_i() = 0;
  virtual void emit_lodb_i(cell number) = 0;
  virtual void emit_const_pri(cell value) = 0;
  virtual void emit_const_alt(cell value) = 0;
  virtual void emit_addr_pri(cell offset) = 0;
  virtual void emit_addr_alt(cell offset) = 0;
  virtual void emit_stor_pri(cell address) = 0;
  virtual void emit_stor_alt(cell address) = 0;
  virtual void emit_stor_s_pri(cell offset) = 0;
  virtual void emit_stor_s_alt(cell offset) = 0;
  virtual void emit_sref_pri(cell address) = 0;
  virtual void emit_sref_alt(cell address) = 0;
  virtual void emit_sref_s_pri(cell offset) = 0;
  virtual void emit_sref_s_alt(cell offset) = 0;
  virtual void emit_stor_i() = 0;
  virtual void emit_strb_i(cell number) = 0;
  virtual void emit_lidx() = 0;
  virtual void emit_lidx_b(cell shift) = 0;
  virtual void emit_idxaddr() = 0;
  virtual void emit_idxaddr_b(cell shift) = 0;
  virtual void emit_align_pri(cell number) = 0;
  virtual void emit_align_alt(cell number) = 0;
  virtual void emit_lctrl(cell index) = 0;
  virtual void emit_sctrl(cell index) = 0;
  virtual void emit_move_pri() = 0;
  virtual void emit_move_alt() = 0;
  virtual void emit_xchg() = 0;
  virtual void emit_push_pri() = 0;
  virtual void emit_push_alt() = 0;
  virtual void emit_push_c(cell value) = 0;
  virtual void emit_push(cell address)  = 0;
  virtual void emit_push_s(cell offset) = 0;
  virtual void emit_pop_pri() = 0;
  virtual void emit_pop_alt() = 0;
  virtual void emit_stack(cell value) = 0;
  virtual void emit_heap(cell value) = 0;
  virtual void emit_proc() = 0;
  virtual void emit_ret() = 0;
  virtual void emit_retn() = 0;
  virtual void emit_call(cell address) = 0;
  virtual void emit_jump_pri() = 0;
  virtual void emit_jump(cell address) = 0;
  virtual void emit_jzer(cell address) = 0;
  virtual void emit_jnz(cell address) = 0;
  virtual void emit_jeq(cell address) = 0;
  virtual void emit_jneq(cell address) = 0;
  virtual void emit_jless(cell address) = 0;
  virtual void emit_jleq(cell address) = 0;
  virtual void emit_jgrtr(cell address) = 0;
  virtual void emit_jgeq(cell address) = 0;
  virtual void emit_jsless(cell address) = 0;
  virtual void emit_jsleq(cell address) = 0;
  virtual void emit_jsgrtr(cell address) = 0;
  virtual void emit_jsgeq(cell address) = 0;
  virtual void emit_shl() = 0;
  virtual void emit_shr() = 0;
  virtual void emit_sshr() = 0;
  virtual void emit_shl_c_pri(cell value) = 0;
  virtual void emit_shl_c_alt(cell value) = 0;
  virtual void emit_shr_c_pri(cell value) = 0;
  virtual void emit_shr_c_alt(cell value) = 0;
  virtual void emit_smul() = 0;
  virtual void emit_sdiv() = 0;
  virtual void emit_sdiv_alt() = 0;
  virtual void emit_umul() = 0;
  virtual void emit_udiv() = 0;
  virtual void emit_udiv_alt() = 0;
  virtual void emit_add() = 0;
  virtual void emit_sub() = 0;
  virtual void emit_sub_alt() = 0;
  virtual void emit_and() = 0;
  virtual void emit_or() = 0;
  virtual void emit_xor() = 0;
  virtual void emit_not() = 0;
  virtual void emit_neg() = 0;
  virtual void emit_invert() = 0;
  virtual void emit_add_c(cell value) = 0;
  virtual void emit_smul_c(cell value) = 0;
  virtual void emit_zero_pri() = 0;
  virtual void emit_zero_alt() = 0;
  virtual void emit_zero(cell address) = 0;
  virtual void emit_zero_s(cell offset) = 0;
  virtual void emit_sign_pri() = 0;
  virtual void emit_sign_alt() = 0;
  virtual void emit_eq() = 0;
  virtual void emit_neq() = 0;
  virtual void emit_less() = 0;
  virtual void emit_leq() = 0;
  virtual void emit_grtr() = 0;
  virtual void emit_geq() = 0;
  virtual void emit_sless() = 0;
  virtual void emit_sleq() = 0;
  virtual void emit_sgrtr() = 0;
  virtual void emit_sgeq() = 0;
  virtual void emit_eq_c_pri(cell value) = 0;
  virtual void emit_eq_c_alt(cell value) = 0;
  virtual void emit_inc_pri() = 0;
  virtual void emit_inc_alt() = 0;
  virtual void emit_inc(cell address) = 0;
  virtual void emit_inc_s(cell offset) = 0;
  virtual void emit_inc_i() = 0;
  virtual void emit_dec_pri() = 0;
  virtual void emit_dec_alt() = 0;
  virtual void emit_dec(cell address) = 0;
  virtual void emit_dec_s(cell offset) = 0;
  virtual void emit_dec_i() = 0;
  virtual void emit_movs(cell num_bytes) = 0;
  virtual void emit_cmps(cell num_bytes) = 0;
  virtual void emit_fill(cell num_bytes) = 0;
  virtual void emit_halt(cell error_code) = 0;
  virtual void emit_bounds(cell value) = 0;
  virtual void emit_sysreq_pri() = 0;
  virtual void emit_sysreq_c(cell index, const char *name) = 0;
  virtual void emit_sysreq_d(cell address, const char *name) = 0;
  virtual void emit_switch(const CaseTable &case_table) = 0;
  virtual void emit_casetbl() = 0;
  virtual void emit_swap_pri() = 0;
  virtual void emit_swap_alt() = 0;
  virtual void emit_push_adr(cell offset) = 0;
  virtual void emit_nop() = 0;
  virtual void emit_break() = 0;

 private:
   const Instruction *instr_;
};

} // namespace amxjit

#endif // !AMXJIT_COMPILER_H
