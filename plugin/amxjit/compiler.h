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

#include <cassert>
#include <cstddef>
#include "amxptr.h"
#include "macros.h"

namespace amxjit {

class CaseTable;
class Instruction;

typedef int (AMXJIT_CDECL *EntryPoint)(cell index, cell *retval);

class CompileErrorHandler {
public:
  virtual ~CompileErrorHandler() {}
  virtual void Execute(const Instruction &instr) = 0;
};

class CompilerOutput {
 public:
  virtual ~CompilerOutput() {}

  // Returns a pointer to the code buffer.
  virtual void *GetCode() const = 0;

  // Returns the size of the code in bytes.
  virtual std::size_t GetCodeSize() const = 0;

  // Returns a pointer to the entry point function.
  virtual EntryPoint GetEntryPoint() const = 0;
};

class Compiler {
 public:
  Compiler();
  virtual ~Compiler();

  // Compiles the specified AMX script. The optional error hander is called at
  // most only once - on first compile error.
  CompilerOutput *Compile(AMXPtr amx, CompileErrorHandler *error_handler = 0);

 protected:
  // This method is called just before the compilation begins.
  // Returns false on error.
  virtual bool Setup() = 0;

  // Processes a single instruction. Returns false on error.
  virtual bool Process(const Instruction &instr) = 0;

  // This method is called on a fatal error.
  virtual void Abort() = 0;

  // Final compilation step. This method shuld either return a runnable
  // CompilerOutput or null which would indicate a fatal error.
  virtual CompilerOutput *Finish() = 0;

  // Returnss the current AMX instance.
  AMXPtr GetCurrentAmx() const {
    assert(compiling);
    return currentAmx;
  }

  // Returns currently processed instruction.
  const Instruction &GetCurrentInstr() const {
    assert(compiling);
    return *currentInstr;
  }

  // Per-opcode methods.
  virtual void load_pri(cell address) = 0;
  virtual void load_alt(cell address) = 0;
  virtual void load_s_pri(cell offset) = 0;
  virtual void load_s_alt(cell offset) = 0;
  virtual void lref_pri(cell address) = 0;
  virtual void lref_alt(cell address) = 0;
  virtual void lref_s_pri(cell offset) = 0;
  virtual void lref_s_alt(cell offset) = 0;
  virtual void load_i() = 0;
  virtual void lodb_i(cell number) = 0;
  virtual void const_pri(cell value) = 0;
  virtual void const_alt(cell value) = 0;
  virtual void addr_pri(cell offset) = 0;
  virtual void addr_alt(cell offset) = 0;
  virtual void stor_pri(cell address) = 0;
  virtual void stor_alt(cell address) = 0;
  virtual void stor_s_pri(cell offset) = 0;
  virtual void stor_s_alt(cell offset) = 0;
  virtual void sref_pri(cell address) = 0;
  virtual void sref_alt(cell address) = 0;
  virtual void sref_s_pri(cell offset) = 0;
  virtual void sref_s_alt(cell offset) = 0;
  virtual void stor_i() = 0;
  virtual void strb_i(cell number) = 0;
  virtual void lidx() = 0;
  virtual void lidx_b(cell shift) = 0;
  virtual void idxaddr() = 0;
  virtual void idxaddr_b(cell shift) = 0;
  virtual void align_pri(cell number) = 0;
  virtual void align_alt(cell number) = 0;
  virtual void lctrl(cell index) = 0;
  virtual void sctrl(cell index) = 0;
  virtual void move_pri() = 0;
  virtual void move_alt() = 0;
  virtual void xchg() = 0;
  virtual void push_pri() = 0;
  virtual void push_alt() = 0;
  virtual void push_c(cell value) = 0;
  virtual void push(cell address)  = 0;
  virtual void push_s(cell offset) = 0;
  virtual void pop_pri() = 0;
  virtual void pop_alt() = 0;
  virtual void stack(cell value) = 0;
  virtual void heap(cell value) = 0;
  virtual void proc() = 0;
  virtual void ret() = 0;
  virtual void retn() = 0;
  virtual void call(cell address) = 0;
  virtual void jump_pri() = 0;
  virtual void jump(cell address) = 0;
  virtual void jzer(cell address) = 0;
  virtual void jnz(cell address) = 0;
  virtual void jeq(cell address) = 0;
  virtual void jneq(cell address) = 0;
  virtual void jless(cell address) = 0;
  virtual void jleq(cell address) = 0;
  virtual void jgrtr(cell address) = 0;
  virtual void jgeq(cell address) = 0;
  virtual void jsless(cell address) = 0;
  virtual void jsleq(cell address) = 0;
  virtual void jsgrtr(cell address) = 0;
  virtual void jsgeq(cell address) = 0;
  virtual void shl() = 0;
  virtual void shr() = 0;
  virtual void sshr() = 0;
  virtual void shl_c_pri(cell value) = 0;
  virtual void shl_c_alt(cell value) = 0;
  virtual void shr_c_pri(cell value) = 0;
  virtual void shr_c_alt(cell value) = 0;
  virtual void smul() = 0;
  virtual void sdiv() = 0;
  virtual void sdiv_alt() = 0;
  virtual void umul() = 0;
  virtual void udiv() = 0;
  virtual void udiv_alt() = 0;
  virtual void add() = 0;
  virtual void sub() = 0;
  virtual void sub_alt() = 0;
  virtual void and_() = 0;
  virtual void or_() = 0;
  virtual void xor_() = 0;
  virtual void not_() = 0;
  virtual void neg() = 0;
  virtual void invert() = 0;
  virtual void add_c(cell value) = 0;
  virtual void smul_c(cell value) = 0;
  virtual void zero_pri() = 0;
  virtual void zero_alt() = 0;
  virtual void zero(cell address) = 0;
  virtual void zero_s(cell offset) = 0;
  virtual void sign_pri() = 0;
  virtual void sign_alt() = 0;
  virtual void eq() = 0;
  virtual void neq() = 0;
  virtual void less() = 0;
  virtual void leq() = 0;
  virtual void grtr() = 0;
  virtual void geq() = 0;
  virtual void sless() = 0;
  virtual void sleq() = 0;
  virtual void sgrtr() = 0;
  virtual void sgeq() = 0;
  virtual void eq_c_pri(cell value) = 0;
  virtual void eq_c_alt(cell value) = 0;
  virtual void inc_pri() = 0;
  virtual void inc_alt() = 0;
  virtual void inc(cell address) = 0;
  virtual void inc_s(cell offset) = 0;
  virtual void inc_i() = 0;
  virtual void dec_pri() = 0;
  virtual void dec_alt() = 0;
  virtual void dec(cell address) = 0;
  virtual void dec_s(cell offset) = 0;
  virtual void dec_i() = 0;
  virtual void movs(cell numBytes) = 0;
  virtual void cmps(cell numBytes) = 0;
  virtual void fill(cell numBytes) = 0;
  virtual void halt(cell errorCode) = 0;
  virtual void bounds(cell value) = 0;
  virtual void sysreq_pri() = 0;
  virtual void sysreq_c(cell index, const char *name) = 0;
  virtual void sysreq_d(cell address, const char *name) = 0;
  virtual void switch_(const CaseTable &caseTable) = 0;
  virtual void casetbl() = 0;
  virtual void swap_pri() = 0;
  virtual void swap_alt() = 0;
  virtual void push_adr(cell offset) = 0;
  virtual void nop() = 0;
  virtual void break_() = 0;

 private:
   bool compiling;
   AMXPtr currentAmx;
   Instruction *currentInstr;
};

} // namespace amxjit

#endif // !AMXJIT_COMPILER_H
