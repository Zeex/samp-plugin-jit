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
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <utility>
#include <amx/amx.h>
#include "amxptr.h"
#include "backend.h"
#include "callconv.h"
#include "macros.h"

namespace AsmJit {
  struct GpReg;
  struct Label;
  struct X86Assembler;
}

namespace jit {

class AsmjitBackendImpl;
class CompileErrorHandler;

class AsmjitBackend : public Backend {
 public:
  // Maps AMX addresses to labels. The string part (the tag) is optional.
  typedef std::map<std::pair<cell, std::string>, AsmJit::Label*> LabelMap;

  enum RuntimeDataIndex {
    RuntimeDataExecPtr = BackendRuntimeDataExec,
    RuntimeDataAmxPtr,
    RuntimeDataEbp,
    RuntimeDataEsp,
    RuntimeDataResetEbp,
    RuntimeDataResetEsp,
    RuntimeDataInstrMapSize,
    RuntimeDataInstrMapPtr
  };

  // This struct represents an entry of the instruction map.
  struct InstrMapEntry {
    cell  amx_addr;
    void *jit_addr;
  };

  class CompareInstrMapEntries
    : std::binary_function<const InstrMapEntry&, const InstrMapEntry&, bool> {
   public:
     bool operator()(const InstrMapEntry &lhs, const InstrMapEntry &rhs) const {
      return lhs.amx_addr < rhs.amx_addr;
     }
  };

  // An "intrinsic" (couldn't find a better term for this) is a portion of
  // machine code that substitutes calls to certain native functions with
  // optimized versions of those.
  class Intrinsic {
   public:
    Intrinsic(const char *name) : name_(name) {}
    virtual ~Intrinsic() {}

    std::string name() const { return name_; }
    virtual void emit(AsmJit::X86Assembler &as) = 0;

   private:
    std::string name_;
  };

 public:
  AsmjitBackend();
  virtual ~AsmjitBackend();

  // Inherited from Backend.
  virtual BackendOutput *compile(AMXPtr amx, 
                                 CompileErrorHandler *error_handler);

 private:
  // The following emit_* functions emit code for common operations and
  // helper functions. Those that are not functions should not have side
  // effects. Functions may change the value of the eax, ecx and edx 
  // registers (i.e. cdecl calling convention).
  // ---------------------------------------------------------------------

  // The code begins with a little block of data containing variaous
  // runtime info like saved stack registers, pointer to the AMX, etc.
  void emit_runtime_data(AMXPtr amx, AsmJit::X86Assembler &as);

  // Reserves enough space for the instruction map- a 1 to 1 mapping
  // between addresses of AMX instructions and offsets to corresponding
  // native instructions the resulting code buffer.
  void emit_instr_map(AsmJit::X86Assembler &as, AMXPtr amx) const;

  // Emits code for:
  // int JIT_CDECL exec(cell index, cell *retval)
  //
  // The exec() function is the entry point into the JIT code and has
  // to be present in all Backends. The _helper functions below are not
  // mandatory but needed for the AsmJit backend to function properly.
  void emit_exec(AsmJit::X86Assembler &as) const;

  // Emits code for:
  // cell JIT_CDECL exec_helper(void *address)
  void emit_exec_helper(AsmJit::X86Assembler &as) const;

  // Emits code for:
  // void JIT_CDECL halt_helper(int error)
  void emit_halt_helper(AsmJit::X86Assembler &as) const;

  // Emits code for:
  // void JIT_CDECL jump_helper(void *address, void *stack_base, void *stack_ptr)
  void emit_jump_helper(AsmJit::X86Assembler &as) const;

  // Emits code for:
  // cell JIT_CDECL sysreq_c_helper(int index, void *stack_base, void *stack_ptr)
  void emit_sysreq_c_helper(AsmJit::X86Assembler &as) const;

  // Emits code for:
  // cell sysreq_d_helper(void *address, void *stack_base, void *stack_ptr)
  void emit_sysreq_d_helper(AsmJit::X86Assembler &as) const;

  // Emits code for copying the AMX pointer to reg.
  void emit_get_amx_ptr(AsmJit::X86Assembler &as,
                        const AsmJit::GpReg &reg) const;

  // Emits code for copying the AMX data pointer to reg.
  void emit_get_amx_data_ptr(AsmJit::X86Assembler &as,
                             const AsmJit::GpReg &reg) const;

 private:
  // Returns pointer to next instruction.
  void *get_next_instr_ptr(AsmJit::X86Assembler &as) const;

  // Returns current pointer to the RuntimeData array (value can change as
  // more code is emitted by the assembler).
  intptr_t *get_runtime_data(AsmJit::X86Assembler &as) const;

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

  // Returns a pointer to the machine instruction corresponding to the
  // specified AMX instruction. The address is realtive to the start of
  // the AMX code.
  static void *JIT_CDECL get_instr_ptr(cell address, void *instr_map,
                                       std::size_t instr_map_size);

 private:
  struct Labels;
  Labels *labels_;

 private:
  LabelMap label_map_;
  std::vector<Intrinsic*> intrinsics_;
  
 private:
  JIT_DISALLOW_COPY_AND_ASSIGN(AsmjitBackend);
};

class AsmjitBackendOutput : public BackendOutput {
 public:
  AsmjitBackendOutput(void *code, std::size_t code_size);
  virtual ~AsmjitBackendOutput();

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
  JIT_DISALLOW_COPY_AND_ASSIGN(AsmjitBackendOutput);
};

} // namespace jit

#endif // !JIT_BACKEND_ASMJIT
