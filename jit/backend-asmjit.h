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

#ifndef JIT_BACKEND_ASMJIT_H
#define JIT_BACKEND_ASMJIT_H

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <amx/amx.h>
#include <asmjit/core.h>
#include <asmjit/x86.h>
#include "amxptr.h"
#include "backend.h"
#include "callconv.h"
#include "macros.h"

namespace jit {

class CompileErrorHandler;

class AsmjitOutput : public BackendOutput {
 public:
  AsmjitOutput(void *code, std::size_t code_size)
    : code_(code), code_size_(code_size)
  {
  }

  virtual ~AsmjitOutput() {
    AsmJit::MemoryManager::getGlobal()->free(code_);
  }

  virtual void *code() const {
    return code_;
  }
  virtual std::size_t code_size() const {
    return code_size_;
  }

 private:
  void *code_;
  std::size_t code_size_;
  
 private:
  JIT_DISALLOW_COPY_AND_ASSIGN(AsmjitOutput);
};

class AsmjitBackend : public Backend {
 public:
  // Maps AMX addresses to labels. The string part (the tag) is optional.
  typedef std::map<std::pair<cell, std::string>, AsmJit::Label> LabelMap;

  // An "intrinsic" (couldn't find a better term for this) is a portion of
  // machine code that substitutes calls to certain native functions with
  // optimized versions of those.
  typedef void (AsmjitBackend::*IntrinsicImpl)(AsmJit::X86Assembler &as);

  struct Intrinsic {
    const char *name;
    IntrinsicImpl impl;
  };

  enum RuntimeDataIndex {
    RuntimeDataExecPtr = BackendRuntimeDataExec,
    RuntimeDataAmxPtr ,    // Pointer to the corresponding AMX instance
    RuntimeDataAmxDataPtr, // Pointer to the AMX data block
    RuntimeDataEbp,
    RuntimeDataEsp,
    RuntimeDataResetEbp,
    RuntimeDataResetEsp,
    RuntimeDataInstrMapPtr
  };

  struct InstrMapEntry {
    cell  amx_addr;
    void *jit_addr;
  };

 public:
  AsmjitBackend();
  virtual ~AsmjitBackend();

  // Inherited from Backend.
  virtual BackendOutput *compile(AMXPtr amx, 
                                 CompileErrorHandler *error_handler);

 private:
  // The code begins with a little block of data containing variaous
  // runtime info like saved stack registers, pointer to the AMX, etc.
  void emit_runtime_data(AMXPtr amx, AsmJit::X86Assembler &as);

  // Reserves enough space for the instruction map- a 1 to 1 mapping
  // between addresses of AMX instructions and offsets to corresponding
  // native instructions the resulting code buffer.
  void emit_instr_map(AsmJit::X86Assembler &as, AMXPtr amx) const;

  // Emits code for:
  // int JIT_CDECL exec(cell index, cell *retval).
  //
  // The exec() function is the entry point into the JIT code and has
  // to be present in all Backends. The _helper functions below are not
  // mandatory but needed for the AsmJit backend to function properly.
  void emit_exec(AsmJit::X86Assembler &as) const;

  // Emits code for:
  // cell JIT_CDECL exec_helper(void *address).
  void emit_exec_helper(AsmJit::X86Assembler &as) const;

  // Emits code for: void JIT_CDECL halt(int error)
  void emit_halt_helper(AsmJit::X86Assembler &as) const;

  // Emits code for:
  // void JIT_CDECL jump(void *address, void *stack_base, void *stack_ptr).
  void emit_jump_helper(AsmJit::X86Assembler &as) const;

  // Emits code for:
  // cell JIT_CDECL sysreqC(int index, void *stack_base, void *stack_ptr).
  void emit_sysreq_c_helper(AsmJit::X86Assembler &as) const;

  // Emits code for:
  // cell sysreqD(void *address, void *stack_base, void *stack_ptr).
  void emit_sysreq_d_helper(AsmJit::X86Assembler &as) const;

  // Returns pointer to next instruction.
  void *get_next_instr_ptr(AsmJit::X86Assembler &as) const {
    intptr_t ip = reinterpret_cast<intptr_t>(as.getCode()) + as.getCodeSize();
    return reinterpret_cast<void*>(ip);
  }

  // Returns current pointer to the RuntimeData array (value can change as
  // more code is emitted by the assembler).
  intptr_t *get_runtime_data(AsmJit::X86Assembler &as) const {
    return reinterpret_cast<intptr_t*>(as.getCode());
  }

  // Sets the value of a RuntimeData element located at the specified index.
  void set_runtime_data(AsmJit::X86Assembler &as, int index,
                        intptr_t data) const {
    get_runtime_data(as)[index] = data;
  }

  // Collects addresses of all functions and jump destinations.
  bool collect_jump_targets(AMXPtr amx, std::set<cell> &refs) const;

  // Sets a label at the specified AMX address. The address should be relative
  // to the start of the code section.
  AsmJit::Label &amx_label(AsmJit::X86Assembler &as, cell address,
                           const std::string &name = std::string());

 private:
  // Returns address of a public function or 0 if the index is not valid.
  static cell JIT_CDECL get_public_addr(AMX *amx, int index);

  // Returns address of a native function or 0 if the index is not valid.
  static cell JIT_CDECL get_native_addr(AMX *amx, int index);

  // Pushes a cell onto the AMX stack but unlike amx_Push() it doesn't
  // change amx->paramcount.
  static void JIT_CDECL push_amx_stack(AMX *amx, cell value);

  // Returns a pointer to a machine instruction corresponding to the
  // specified AMX instruction. The address is realtive to the start of
  // the AMX code.
  static void *JIT_CDECL get_instr_ptr(void *instr_map, cell address);

 private:
  AsmJit::Label L_exec_ptr_;
  AsmJit::Label L_amx_ptr_;
  AsmJit::Label L_amx_data_ptr_;
  AsmJit::Label L_exec_;
  AsmJit::Label L_exec_helper_;
  AsmJit::Label L_halt_helper_;
  AsmJit::Label L_jump_helper_;
  AsmJit::Label L_sysreq_c_helper_;
  AsmJit::Label L_sysreq_d_helper_;
  AsmJit::Label L_ebp_;
  AsmJit::Label L_esp_;
  AsmJit::Label L_reset_ebp_;
  AsmJit::Label L_reset_esp_;
  AsmJit::Label L_instr_map_ptr_;

  void native_float(AsmJit::X86Assembler &as);
  void native_floatabs(AsmJit::X86Assembler &as);
  void native_floatadd(AsmJit::X86Assembler &as);
  void native_floatsub(AsmJit::X86Assembler &as);
  void native_floatmul(AsmJit::X86Assembler &as);
  void native_floatdiv(AsmJit::X86Assembler &as);
  void native_floatsqroot(AsmJit::X86Assembler &as);
  void native_floatlog(AsmJit::X86Assembler &as);

 private:
  LabelMap label_map_;
  static Intrinsic intrinsics_[];
  
 private:
  JIT_DISALLOW_COPY_AND_ASSIGN(AsmjitBackend);
};

} // namespace jit

#endif // !JIT_BACKEND_ASMJIT
