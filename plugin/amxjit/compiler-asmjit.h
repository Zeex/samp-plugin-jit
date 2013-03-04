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

#ifndef AMXJIT_COMPILER_ASMJIT_H
#define AMXJIT_COMPILER_ASMJIT_H

#include <set>
#include <vector>
#include <utility>
#include <amx/amx.h>
namespace AsmJit {
  #include <asmjit/core/build.h>
}
#include <asmjit/core.h>
#include <asmjit/x86.h>
#include "amxptr.h"
#include "compiler.h"
#include "macros.h"

namespace amxjit {

class CompileErrorHandler;

class CompilerAsmjit : public Compiler {
 public:
  typedef void (CompilerAsmjit::*EmitIntrinsic)();

  class AddressedLabel : public AsmJit::Label {
   public:
    AddressedLabel(cell address)
      : AsmJit::Label(), address_(address)
    {}

    AddressedLabel(cell address, const Label &label)
      : AsmJit::Label(label), address_(address)
    {}

    cell address() const { return address_; }

    bool operator<(const AddressedLabel &rhs) const {
      return address_ < rhs.address_;
    }

   private:
    cell address_;
  };

  CompilerAsmjit();
  virtual ~CompilerAsmjit();

 protected:
  virtual bool setup(AMXPtr amx);
  virtual bool process(const Instruction &instr);
  virtual void abort();
  virtual CompilerOutput *finish();

  virtual void emit_load_pri(cell address);
  virtual void emit_load_alt(cell address);
  virtual void emit_load_s_pri(cell offset);
  virtual void emit_load_s_alt(cell offset);
  virtual void emit_lref_pri(cell address);
  virtual void emit_lref_alt(cell address);
  virtual void emit_lref_s_pri(cell offset);
  virtual void emit_lref_s_alt(cell offset);
  virtual void emit_load_i();
  virtual void emit_lodb_i(cell number);
  virtual void emit_const_pri(cell value);
  virtual void emit_const_alt(cell value);
  virtual void emit_addr_pri(cell offset);
  virtual void emit_addr_alt(cell offset);
  virtual void emit_stor_pri(cell address);
  virtual void emit_stor_alt(cell address);
  virtual void emit_stor_s_pri(cell offset);
  virtual void emit_stor_s_alt(cell offset);
  virtual void emit_sref_pri(cell address);
  virtual void emit_sref_alt(cell address);
  virtual void emit_sref_s_pri(cell offset);
  virtual void emit_sref_s_alt(cell offset);
  virtual void emit_stor_i();
  virtual void emit_strb_i(cell number);
  virtual void emit_lidx();
  virtual void emit_lidx_b(cell shift);
  virtual void emit_idxaddr();
  virtual void emit_idxaddr_b(cell shift);
  virtual void emit_align_pri(cell number);
  virtual void emit_align_alt(cell number);
  virtual void emit_lctrl(cell index);
  virtual void emit_sctrl(cell index);
  virtual void emit_move_pri();
  virtual void emit_move_alt();
  virtual void emit_xchg();
  virtual void emit_push_pri();
  virtual void emit_push_alt();
  virtual void emit_push_c(cell value);
  virtual void emit_push(cell address) ;
  virtual void emit_push_s(cell offset);
  virtual void emit_pop_pri();
  virtual void emit_pop_alt();
  virtual void emit_stack(cell value);
  virtual void emit_heap(cell value);
  virtual void emit_proc();
  virtual void emit_ret();
  virtual void emit_retn();
  virtual void emit_call(cell address);
  virtual void emit_jump_pri();
  virtual void emit_jump(cell address);
  virtual void emit_jzer(cell address);
  virtual void emit_jnz(cell address);
  virtual void emit_jeq(cell address);
  virtual void emit_jneq(cell address);
  virtual void emit_jless(cell address);
  virtual void emit_jleq(cell address);
  virtual void emit_jgrtr(cell address);
  virtual void emit_jgeq(cell address);
  virtual void emit_jsless(cell address);
  virtual void emit_jsleq(cell address);
  virtual void emit_jsgrtr(cell address);
  virtual void emit_jsgeq(cell address);
  virtual void emit_shl();
  virtual void emit_shr();
  virtual void emit_sshr();
  virtual void emit_shl_c_pri(cell value);
  virtual void emit_shl_c_alt(cell value);
  virtual void emit_shr_c_pri(cell value);
  virtual void emit_shr_c_alt(cell value);
  virtual void emit_smul();
  virtual void emit_sdiv();
  virtual void emit_sdiv_alt();
  virtual void emit_umul();
  virtual void emit_udiv();
  virtual void emit_udiv_alt();
  virtual void emit_add();
  virtual void emit_sub();
  virtual void emit_sub_alt();
  virtual void emit_and();
  virtual void emit_or();
  virtual void emit_xor();
  virtual void emit_not();
  virtual void emit_neg();
  virtual void emit_invert();
  virtual void emit_add_c(cell value);
  virtual void emit_smul_c(cell value);
  virtual void emit_zero_pri();
  virtual void emit_zero_alt();
  virtual void emit_zero(cell address);
  virtual void emit_zero_s(cell offset);
  virtual void emit_sign_pri();
  virtual void emit_sign_alt();
  virtual void emit_eq();
  virtual void emit_neq();
  virtual void emit_less();
  virtual void emit_leq();
  virtual void emit_grtr();
  virtual void emit_geq();
  virtual void emit_sless();
  virtual void emit_sleq();
  virtual void emit_sgrtr();
  virtual void emit_sgeq();
  virtual void emit_eq_c_pri(cell value);
  virtual void emit_eq_c_alt(cell value);
  virtual void emit_inc_pri();
  virtual void emit_inc_alt();
  virtual void emit_inc(cell address);
  virtual void emit_inc_s(cell offset);
  virtual void emit_inc_i();
  virtual void emit_dec_pri();
  virtual void emit_dec_alt();
  virtual void emit_dec(cell address);
  virtual void emit_dec_s(cell offset);
  virtual void emit_dec_i();
  virtual void emit_movs(cell num_bytes);
  virtual void emit_cmps(cell num_bytes);
  virtual void emit_fill(cell num_bytes);
  virtual void emit_halt(cell error_code);
  virtual void emit_bounds(cell value);
  virtual void emit_sysreq_pri();
  virtual void emit_sysreq_c(cell index, const char *name);
  virtual void emit_sysreq_d(cell address, const char *name);
  virtual void emit_switch(const CaseTable &case_table);
  virtual void emit_casetbl();
  virtual void emit_swap_pri();
  virtual void emit_swap_alt();
  virtual void emit_push_adr(cell offset);
  virtual void emit_nop();
  virtual void emit_break();

 private:
  intptr_t *get_runtime_data();
  void set_runtime_data(int index, intptr_t data);

  void emit_get_amx_ptr(const AsmJit::GpReg &reg);
  void emit_get_amx_data_ptr(const AsmJit::GpReg &reg);

  void emit_runtime_data(AMXPtr amx);
  void emit_instr_map(AMXPtr amx);
  void emit_exec();
  void emit_exec_helper();
  void emit_halt_helper();
  void emit_jump_helper();
  void emit_sysreq_c_helper();
  void emit_sysreq_d_helper();
  void emit_break_helper();

  bool emit_intrinsic(const char *name);
  void emit_float();
  void emit_floatabs();
  void emit_floatadd();
  void emit_floatsub();
  void emit_floatmul();
  void emit_floatdiv();
  void emit_floatsqroot();
  void emit_floatlog();

  const AddressedLabel &amx_label(cell address) {
    std::set<AddressedLabel>::const_iterator it = amx_labels_.find(address);
    if (it != amx_labels_.end()) {
      return *it;
    } else {
      std::pair<std::set<AddressedLabel>::iterator, bool> result =
        amx_labels_.insert(AddressedLabel(address, as_.newLabel()));
      return *result.first;
    }
  }

  AsmJit::X86Assembler as_;

  AsmJit::Label L_exec_ptr_;
  AsmJit::Label L_amx_ptr_;
  AsmJit::Label L_ebp_ptr_;
  AsmJit::Label L_esp_ptr_;
  AsmJit::Label L_reset_ebp_ptr_;
  AsmJit::Label L_reset_esp_ptr_;
  AsmJit::Label L_instr_map_size_;
  AsmJit::Label L_instr_map_ptr_;
  AsmJit::Label L_exec_;
  AsmJit::Label L_exec_helper_;
  AsmJit::Label L_halt_helper_;
  AsmJit::Label L_jump_helper_;
  AsmJit::Label L_sysreq_c_helper_;
  AsmJit::Label L_sysreq_d_helper_;
  AsmJit::Label L_break_helper_;

  // Labels corresponding to AMX instructions.
  std::set<AddressedLabel> amx_labels_;

  // Maps AMX instructions to JIT code offsets.
  std::vector<std::pair<cell, std::ptrdiff_t> > instr_map_;

 private:
  AMXJIT_DISALLOW_COPY_AND_ASSIGN(CompilerAsmjit);
};

class CompilerOutputAsmjit : public CompilerOutput {
 public:
  CompilerOutputAsmjit(void *code, std::size_t code_size);
  virtual ~CompilerOutputAsmjit();

  virtual void *code() const {
    return code_;
  }

  virtual std::size_t code_size() const {
    return code_size_;
  }

  virtual EntryPoint entry_point() const {
    assert(code_ != 0);
    return (EntryPoint)*reinterpret_cast<void**>(code());
  }

 private:
  void *code_;
  std::size_t code_size_;
  
 private:
  AMXJIT_DISALLOW_COPY_AND_ASSIGN(CompilerOutputAsmjit);
};

} // namespace amxjit

#endif // !AMXJIT_COMPILER_ASMJIT_H
