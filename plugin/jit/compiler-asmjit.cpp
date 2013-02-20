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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <amx/amx.h>
namespace AsmJit {
  #include <asmjit/core/build.h>
}
#include <asmjit/core.h>
#include <asmjit/x86.h>
#include "amxptr.h"
#include "amxdisasm.h"
#include "callconv.h"
#include "compiler.h"
#include "compiler-asmjit.h"

namespace {

// AsmJit core
using AsmJit::Label;
using AsmJit::asmjit_cast;

// X86-specific
using AsmJit::X86Assembler;
using AsmJit::byte_ptr;
using AsmJit::word_ptr;
using AsmJit::dword_ptr;
using AsmJit::GpReg;
using AsmJit::al;
using AsmJit::ax;
using AsmJit::eax;
using AsmJit::cl;
using AsmJit::cx;
using AsmJit::ebx;
using AsmJit::ecx;
using AsmJit::edx;
using AsmJit::esi;
using AsmJit::edi;
using AsmJit::ebp;
using AsmJit::esp;
using AsmJit::st;

// Indices of runtime data block elements. Runtime data is the first thing
// that is written to the code buffer and is used throughout generated JIT
// code to keep track of certain data at runtime.
enum RuntimeDataIndex {
  RuntimeDataExecPtr,
  RuntimeDataAmxPtr,
  RuntimeDataEbp,
  RuntimeDataEsp,
  RuntimeDataResetEbp,
  RuntimeDataResetEsp,
  RuntimeDataInstrMapSize,
  RuntimeDataInstrMapPtr,
  RuntimeDataLast_
};

// Indices of "fixed" labels (see Assembler below).
// The order here is not as important as in RuntimeDataIndex.
enum LabelIndex {
  // Runtime data labels.
  LabelExecPtr,
  LabelAmxPtr,
  LabelEbp,
  LabelEsp,
  LabelResetEbp,
  LabelResetEsp,
  LabelInstrMapSize,
  LabelInstrMapPtr,

  // Entry point and other auxilary functions.
  LabelExec,
  LabelExecHelper,
  LabelHaltHelper,
  LabelJumpHelper,
  LabelSysreqCHelper,
  LabelSysreqDHelper,
  LabelBreakHelper,

  // Terminator (not a real label).
  LabelLast_
};

class Assembler;

typedef void (*EmitIntrinsic)(Assembler &as);
typedef void (*EmitInstruction)(Assembler &as,
                                const jit::AMXInstruction &instr, bool *error);

struct Intrinsic {
  const char    *name;
  EmitIntrinsic  emit;
};

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

class AddressedLabel : public Label {
 public:
  AddressedLabel(cell address)
    : Label(), address_(address)
  {}

  AddressedLabel(cell address, const Label &label)
    : Label(label), address_(address)
  {}

  cell address() const { return address_; }

  bool operator<(const AddressedLabel &rhs) const {
    return address_ < rhs.address_;
  }

 private:
  cell address_;
};

// It's a good idea to follow AsmJit naming convention when extending this class.
class Assembler : public X86Assembler {
 public:
  Assembler(jit::AMXPtr amx) : amx_(amx) {}
  virtual ~Assembler() {}

  jit::AMXPtr &getAmx() { return amx_; }
  const jit::AMXPtr &getAmx() const { return amx_; }

  void setFixedLabels(const std::vector<Label> &labels) {
    fixedLabels_ = labels;
  }

  const Label &getFixedLabel(int index) const {
    assert(index >= 0 && index < static_cast<int>(fixedLabels_.size())
           && "Bad label index");
    return fixedLabels_[index];
  }

  const AddressedLabel &getAmxLabel(cell address) {
    std::set<AddressedLabel>::const_iterator it = amxLabels_.find(address);
    if (it != amxLabels_.end()) {
      return *it;
    } else {
      std::pair<std::set<AddressedLabel>::iterator, bool> result =
        amxLabels_.insert(AddressedLabel(address, newLabel()));
      return *result.first;
    }
  }

  void bindFixed(int index) {
    bind(getFixedLabel(index));
  }

 private:
  jit::AMXPtr amx_;
  std::vector<Label> fixedLabels_;
  std::set<AddressedLabel> amxLabels_;
};

void emit_float(Assembler &as) {
  as.fild(dword_ptr(esp, 4));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void emit_floatabs(Assembler &as) {
  as.fld(dword_ptr(esp, 4));
  as.fabs();
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void emit_floatadd(Assembler &as) {
  as.fld(dword_ptr(esp, 4));
  as.fadd(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void emit_floatsub(Assembler &as) {
  as.fld(dword_ptr(esp, 4));
  as.fsub(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void emit_floatmul(Assembler &as) {
  as.fld(dword_ptr(esp, 4));
  as.fmul(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void emit_floatdiv(Assembler &as) {
  as.fld(dword_ptr(esp, 4));
  as.fdiv(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void emit_floatsqroot(Assembler &as) {
  as.fld(dword_ptr(esp, 4));
  as.fsqrt();
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void emit_floatlog(Assembler &as) {
  as.fld1();
  as.fld(dword_ptr(esp, 8));
  as.fyl2x();
  as.fld1();
  as.fdivrp(st(1));
  as.fld(dword_ptr(esp, 4));
  as.fyl2x();
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

bool emit_intrinsic(Assembler &as, const char *name) {
  Intrinsic intrinsics[] = {
    {"float",       &emit_float},
    {"floatabs",    &emit_floatabs},
    {"floatadd",    &emit_floatadd},
    {"floatsub",    &emit_floatsub},
    {"floatmul",    &emit_floatmul},
    {"floatdiv",    &emit_floatdiv},
    {"floatsqroot", &emit_floatsqroot},
    {"floatlog",    &emit_floatlog}
  };

  for (std::size_t i = 0; i < sizeof(intrinsics) / sizeof(*intrinsics); i++) {
    if (std::strcmp(intrinsics[i].name, name) == 0) {
      intrinsics[i].emit(as);
      return true;
    }
  }

  return false;
}

cell JIT_CDECL get_public_addr(AMX *amx, int index) {
  return jit::AMXPtr(amx).get_public_addr(index);
}

cell JIT_CDECL get_native_addr(AMX *amx, int index) {
  return jit::AMXPtr(amx).get_native_addr(index);
}

void *JIT_CDECL get_instr_ptr(cell address, void *instr_map,
                                     std::size_t instr_map_size) {
  assert(instr_map != 0);
  InstrMapEntry *begin = reinterpret_cast<InstrMapEntry*>(instr_map);
  InstrMapEntry target = {address, 0};

  std::pair<InstrMapEntry*, InstrMapEntry*> result;
  result = std::equal_range(begin, begin + instr_map_size, target,
                            CompareInstrMapEntries());

  if (result.first != result.second) {
    return result.first->jit_addr;
  }

  return 0;
}

inline cell rel_code_addr(jit::AMXPtr amx, cell address) {
  return address - reinterpret_cast<cell>(amx.code());
}

void collect_jump_targets(jit::AMXPtr amx, std::set<cell> &refs) {
  jit::AMXDisassembler disas(amx);
  jit::AMXInstruction instr;

  while (disas.decode(instr)) {
    jit::AMXOpcode opcode = instr.opcode();
    if ((opcode.is_call() || opcode.is_jump()) && instr.num_operands() == 1) {
      refs.insert(rel_code_addr(amx, instr.operand()));
    } else if (opcode.id() == jit::AMX_OP_CASETBL) {
      int n = instr.num_operands();
      for (int i = 1; i < n; i += 2) {
        refs.insert(rel_code_addr(amx, instr.operand(i)));
      }
    } else if (opcode.id() == jit::AMX_OP_PROC) {
      refs.insert(instr.address());
    }
  }
}

intptr_t *get_runtime_data(Assembler &as) {
  return reinterpret_cast<intptr_t*>(as.getCode());
}

void set_runtime_data(Assembler &as, int index, intptr_t data) {
  get_runtime_data(as)[index] = data;
}

void emit_runtime_data(Assembler &as) {
  as.bindFixed(RuntimeDataExecPtr);
    as.dd(0);
  as.bindFixed(RuntimeDataAmxPtr);
    as.dintptr(reinterpret_cast<intptr_t>(as.getAmx().amx()));
  as.bindFixed(RuntimeDataEbp);
    as.dd(0);
  as.bindFixed(RuntimeDataEsp);
    as.dd(0);
  as.bindFixed(RuntimeDataResetEbp);
    as.dd(0);
  as.bindFixed(RuntimeDataResetEsp);
    as.dd(0);
  as.bindFixed(RuntimeDataInstrMapSize);
    as.dd(0);
  as.bindFixed(RuntimeDataInstrMapPtr);
    as.dd(0);
}

void emit_instr_map(Assembler &as) {
  jit::AMXDisassembler disas(as.getAmx());
  jit::AMXInstruction instr;
  int size = 0;
  while (disas.decode(instr)) {
    size++;
  }

  set_runtime_data(as, RuntimeDataInstrMapSize, size);
  set_runtime_data(as, RuntimeDataInstrMapPtr, as.getCodeSize());

  InstrMapEntry dummy = {0};
  for (int i = 0; i < size; i++) {
    as.dstruct(dummy);
  }
}

void emit_get_amx_ptr(Assembler &as, const GpReg &reg) {
  as.mov(reg, dword_ptr(as.getFixedLabel(LabelAmxPtr)));
}

void emit_get_amx_data_ptr(Assembler &as, const GpReg &reg) {
  Label L_quit = as.newLabel();

    emit_get_amx_ptr(as, eax);

    as.mov(reg, dword_ptr(eax, offsetof(AMX, data)));
    as.test(reg, reg);
    as.jnz(L_quit);

    as.mov(reg, dword_ptr(eax, offsetof(AMX, base)));
    as.mov(eax, dword_ptr(reg, offsetof(AMX_HEADER, dat)));
    as.add(reg, eax);

  as.bind(L_quit);
}

// int JIT_CDECL exec(cell index, cell *retval);
void emit_exec(Assembler &as) {
  set_runtime_data(as, RuntimeDataExecPtr, as.getCodeSize());

  const Label &L_instr_map = as.getFixedLabel(LabelInstrMapPtr);
  const Label &L_instr_map_size = as.getFixedLabel(LabelInstrMapSize);
  const Label &L_reset_ebp = as.getFixedLabel(LabelResetEbp);
  const Label &L_reset_esp = as.getFixedLabel(LabelResetEsp);
  const Label &L_exec_helper = as.getFixedLabel(LabelExecHelper);
  Label L_stack_heap_overflow = as.newLabel();
  Label L_heap_underflow = as.newLabel();
  Label L_stack_underflow = as.newLabel();
  Label L_native_not_found = as.newLabel();
  Label L_public_not_found = as.newLabel();
  Label L_finish = as.newLabel();
  Label L_return = as.newLabel();

  // Offsets of exec() arguments relative to ebp.
  int arg_index = 8;
  int arg_retval = 12;
  int var_address = -4;
  int var_reset_ebp = -8;
  int var_reset_esp = -12;

  as.bindFixed(LabelExec);
    as.push(ebp);
    as.mov(ebp, esp);
    as.sub(esp, 12); // for locals

    as.push(esi);
    emit_get_amx_ptr(as, esi);

    // JIT code expects AMX data pointer to be in ebx.
    as.push(ebx);
    emit_get_amx_data_ptr(as, ebx);

    // Check for stack/heap collision (stack/heap overflow).
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, hea)));
    as.mov(edx, dword_ptr(esi, offsetof(AMX, stk)));
    as.cmp(ecx, edx);
    as.jge(L_stack_heap_overflow);

    // Check for stack underflow.
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, stk)));
    as.mov(edx, dword_ptr(esi, offsetof(AMX, stp)));
    as.cmp(ecx, edx);
    as.jg(L_stack_underflow);

    // Check for heap underflow.
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, hea)));
    as.mov(edx, dword_ptr(esi, offsetof(AMX, hlw)));
    as.cmp(ecx, edx);
    as.jl(L_heap_underflow);

    // Make sure all natives are registered (the AMX_FLAG_NTVREG flag
    // must be set).
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, flags)));
    as.test(ecx, AMX_FLAG_NTVREG);
    as.jz(L_native_not_found);

    // Reset the error code.
    as.mov(dword_ptr(esi, offsetof(AMX, error)), AMX_ERR_NONE);

    // Get address of the public function.
    as.push(dword_ptr(ebp, arg_index));
    emit_get_amx_ptr(as, eax);
    as.push(eax);
    as.call(asmjit_cast<void*>(&get_public_addr));
    as.add(esp, 8);

    // Check whether the function was found (address should be 0).
    as.test(eax, eax);
    as.jz(L_public_not_found);

    // Get pointer to the start of the function.
    as.push(dword_ptr(L_instr_map_size));
    as.push(dword_ptr(L_instr_map));
    as.push(eax);
    as.call(asmjit_cast<void*>(&get_instr_ptr));
    as.add(esp, 12);
    as.mov(dword_ptr(ebp, var_address), eax);

    // Push size of arguments and reset parameter count.
    // Pseudo code:
    //   stk = amx->stk - sizeof(cell);
    //   *(amx_data + stk) = amx->paramcount;
    //   amx->stk = stk;
    //   amx->paramcount = 0;
    as.mov(eax, dword_ptr(esi, offsetof(AMX, paramcount)));
    as.imul(eax, eax, sizeof(cell));
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, stk)));
    as.sub(ecx, sizeof(cell));
    as.mov(dword_ptr(ebx, ecx), eax);
    as.mov(dword_ptr(esi, offsetof(AMX, stk)), ecx);
    as.mov(dword_ptr(esi, offsetof(AMX, paramcount)), 0);

    // Keep a copy of the old reset_ebp and reset_esp on the stack.
    as.mov(eax, dword_ptr(L_reset_ebp));
    as.mov(dword_ptr(ebp, var_reset_ebp), eax);
    as.mov(eax, dword_ptr(L_reset_esp));
    as.mov(dword_ptr(ebp, var_reset_esp), eax);

    // Call the function.
    as.push(dword_ptr(ebp, var_address));
    as.call(L_exec_helper);
    as.add(esp, 4);

    // Copy return value to retval if it's not null.
    as.mov(ecx, dword_ptr(ebp, arg_retval));
    as.test(ecx, ecx);
    as.jz(L_finish);
    as.mov(dword_ptr(ecx), eax);

  as.bind(L_finish);
    // Restore reset_ebp and reset_esp (remember they are kept in locals?).
    as.mov(eax, dword_ptr(ebp, var_reset_ebp));
    as.mov(dword_ptr(L_reset_ebp), eax);
    as.mov(eax, dword_ptr(ebp, var_reset_esp));
    as.mov(dword_ptr(L_reset_esp), eax);

    // Copy amx->error for return and reset it.
    as.mov(eax, AMX_ERR_NONE);
    as.xchg(eax, dword_ptr(esi, offsetof(AMX, error)));

  as.bind(L_return);
    as.pop(ebx);
    as.pop(esi);
    as.mov(esp, ebp);
    as.pop(ebp);
    as.ret();

  as.bind(L_stack_heap_overflow);
    as.mov(eax, AMX_ERR_STACKERR);
    as.jmp(L_return);

  as.bind(L_heap_underflow);
    as.mov(eax, AMX_ERR_HEAPLOW);
    as.jmp(L_return);

  as.bind(L_stack_underflow);
    as.mov(eax, AMX_ERR_STACKLOW);
    as.jmp(L_return);

  as.bind(L_native_not_found);
    as.mov(eax, AMX_ERR_NOTFOUND);
    as.jmp(L_return);

  as.bind(L_public_not_found);
    as.mov(eax, AMX_ERR_INDEX);
    as.jmp(L_return);
}

// cell JIT_CDECL exec_helper(void *address);
void emit_exec_helper(Assembler &as) {
  const Label &L_ebp = as.getFixedLabel(LabelEbp);
  const Label &L_esp = as.getFixedLabel(LabelEsp);
  const Label &L_reset_ebp = as.getFixedLabel(LabelResetEbp);
  const Label &L_reset_esp = as.getFixedLabel(LabelResetEsp);

  as.bindFixed(LabelExecHelper);
    // Store function address in eax.
    as.mov(eax, dword_ptr(esp, 4));

    // esi and edi are not saved across function bounds but generally
    // can be utilized in JIT code (for instance, in MOVS).
    as.push(esi);
    as.push(edi);

    // In JIT code these are caller-saved registers:
    //  eax - primary register (PRI)
    //  ecx - alternate register (ALT)
    //  ebx - data base pointer (DAT + amx->base)
    //  edx - temporary storage
    as.push(ebx);
    as.push(ecx);
    as.push(edx);

    // Store old ebp and esp on the stack.
    as.push(dword_ptr(L_ebp));
    as.push(dword_ptr(L_esp));

    // Most recent ebp and esp are stored in member variables.
    as.mov(dword_ptr(L_ebp), ebp);
    as.mov(dword_ptr(L_esp), esp);

    // Switch from native stack to AMX stack.
    emit_get_amx_ptr(as, ecx);
    as.mov(edx, dword_ptr(ecx, offsetof(AMX, frm)));
    as.lea(ebp, dword_ptr(ebx, edx)); // ebp = amx_data + amx->frm
    as.mov(edx, dword_ptr(ecx, offsetof(AMX, stk)));
    as.lea(esp, dword_ptr(ebx, edx)); // esp = amx_data + amx->stk

    // In order to make halt() work we have to be able to return to this
    // point somehow. The easiest way it to set the stack registers as
    // if we called the offending instruction directly from here.
    as.lea(ecx, dword_ptr(esp, - 4));
    as.mov(dword_ptr(L_reset_esp), ecx);
    as.mov(dword_ptr(L_reset_ebp), ebp);

    // Call the function. Prior to his point ebx should point to the
    // AMX data and the both stack pointers should point to somewhere
    // in the AMX stack.
    as.call(eax);

    // Keep AMX stack registers up-to-date. This wouldn't be necessary if
    // RETN didn't modify them (it pops all arguments off the stack).
    emit_get_amx_ptr(as, eax);
    as.mov(edx, ebp);
    as.sub(edx, ebx);
    as.mov(dword_ptr(eax, offsetof(AMX, frm)), edx); // amx->frm = ebp - amx_data
    as.mov(edx, esp);
    as.sub(edx, ebx);
    as.mov(dword_ptr(eax, offsetof(AMX, stk)), edx); // amx->stk = esp - amx_data

    // Switch back to native stack.
    as.mov(ebp, dword_ptr(L_ebp));
    as.mov(esp, dword_ptr(L_esp));

    as.pop(dword_ptr(L_esp));
    as.pop(dword_ptr(L_ebp));

    as.pop(edx);
    as.pop(ecx);
    as.pop(ebx);
    as.pop(edi);
    as.pop(esi);

    as.ret();
}

// void halt_helper(int error [edx]);
void emit_halt_helper(Assembler &as) {
  const Label &L_reset_ebp = as.getFixedLabel(LabelResetEbp);
  const Label &L_reset_esp = as.getFixedLabel(LabelResetEsp);

  as.bindFixed(LabelHaltHelper);
    emit_get_amx_ptr(as, esi);
    as.mov(dword_ptr(esi, offsetof(AMX, error)), edx); // error code in edx

    // Reset stack so we can return right to call().
    as.mov(esp, dword_ptr(L_reset_esp));
    as.mov(ebp, dword_ptr(L_reset_ebp));

    // Pop public arguments as it would otherwise be done by RETN.
    as.pop(eax);
    as.add(esp, dword_ptr(esp));
    as.add(esp, 4);
    as.push(eax);

    as.ret();
}

// void jump_helper(void *address [edx], void *stack_base [esi],
//                  void *stack_ptr [edi]);
void emit_jump_helper(Assembler &as) {
  const Label &L_instr_map = as.getFixedLabel(LabelInstrMapPtr);
  const Label &L_instr_map_size = as.getFixedLabel(LabelInstrMapSize);
  Label L_invalid_address = as.newLabel();

  as.bindFixed(LabelJumpHelper);
    as.push(eax);
    as.push(ecx);

    as.push(dword_ptr(L_instr_map_size));
    as.push(dword_ptr(L_instr_map));
    as.push(edx);
    as.call(asmjit_cast<void*>(&get_instr_ptr));
    as.add(esp, 12);
    as.mov(edx, eax); // address

    as.pop(ecx);
    as.pop(eax);

    as.test(edx, edx);
    as.jz(L_invalid_address);

    as.mov(ebp, esi);
    as.mov(esp, edi);
    as.jmp(edx);

  // Continue execution as if no jump was initiated (this is what AMX does).
  as.bind(L_invalid_address);
    as.ret();
}

// cell JIT_CDECL sysreq_c_helper(int index, void *stack_base, void *stack_ptr);
void emit_sysreq_c_helper(Assembler &as) {
  const Label &L_sysreq_d_helper = as.getFixedLabel(LabelSysreqDHelper);
  Label L_native_not_found = as.newLabel();
  Label L_return = as.newLabel();

  int arg_index = 8;
  int arg_stack_base = 12;
  int arg_stack_ptr = 16;

  as.bindFixed(LabelSysreqCHelper);
    as.push(ebp);
    as.mov(ebp, esp);

    as.push(dword_ptr(ebp, arg_index));
    emit_get_amx_ptr(as, eax);
    as.push(eax);
    as.call(asmjit_cast<void*>(&get_native_addr));
    as.add(esp, 8);
    as.test(eax, eax);
    as.jz(L_native_not_found);

    as.push(dword_ptr(ebp, arg_stack_ptr));
    as.push(dword_ptr(ebp, arg_stack_base));
    as.push(eax); // address
    as.call(L_sysreq_d_helper);
    as.add(esp, 12);

  as.bind(L_return);
    as.mov(esp, ebp);
    as.pop(ebp);
    as.ret();

  as.bind(L_native_not_found);
    as.mov(eax, AMX_ERR_NOTFOUND);
    as.jmp(L_return);
}

// cell JIT_CDECL sysreq_d_helper(void *address, void *stack_base, void *stack_ptr);
void emit_sysreq_d_helper(Assembler &as) {
  const Label &L_ebp = as.getFixedLabel(LabelEbp);
  const Label &L_esp = as.getFixedLabel(LabelEsp);

  as.bindFixed(LabelSysreqDHelper);
    as.mov(eax, dword_ptr(esp, 4));   // address
    as.mov(ebp, dword_ptr(esp, 8));   // stack_base
    as.mov(esp, dword_ptr(esp, 12));  // stack_ptr
    as.mov(ecx, esp);                 // params
    as.mov(esi, dword_ptr(esp, -16)); // return address

    emit_get_amx_ptr(as, edx);

    // Switch to native stack.
    as.sub(ebp, ebx);
    as.mov(dword_ptr(edx, offsetof(AMX, frm)), ebp); // amx->frm = ebp - amx_data
    as.mov(ebp, dword_ptr(L_ebp));
    as.sub(esp, ebx);
    as.mov(dword_ptr(edx, offsetof(AMX, stk)), esp); // amx->stk = esp - amx_data
    as.mov(esp, dword_ptr(L_esp));

    // Call the native function.
    as.push(ecx); // params
    as.push(edx); // amx
    as.call(eax); // address
    as.add(esp, 8);
    // eax contains return value, the code below must not overwrite it!!

    // Switch back to AMX stack.
    emit_get_amx_ptr(as, edx);
    as.mov(dword_ptr(L_ebp), ebp);
    as.mov(ecx, dword_ptr(edx, offsetof(AMX, frm)));
    as.lea(ebp, dword_ptr(ebx, ecx)); // ebp = amx_data + amx->frm
    as.mov(dword_ptr(L_esp), esp);
    as.mov(ecx, dword_ptr(edx, offsetof(AMX, stk)));
    as.lea(esp, dword_ptr(ebx, ecx)); // ebp = amx_data + amx->stk

    // Modify the return address so we return next to the sysreq point.
    as.push(esi);
    as.ret();
}

// void break_helper();
void emit_break_helper(Assembler &as) {
  Label L_return = as.newLabel();

  as.bindFixed(LabelBreakHelper);
    emit_get_amx_ptr(as, edx);
    as.mov(esi, dword_ptr(edx, offsetof(AMX, debug)));
    as.test(esi, esi);
    as.jz(L_return);

    as.push(eax);
    as.push(ecx);

    as.push(edx);
    as.call(esi);
    as.add(esp, 4);

    as.pop(ecx);
    as.pop(eax);

  as.bind(L_return);
    as.ret();
}

void emit_load_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = [address]
  as.mov(eax, dword_ptr(ebx, instr.operand()));
}

void emit_load_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // ALT = [address]
  as.mov(ecx, dword_ptr(ebx, instr.operand()));
}

void emit_load_s_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = [FRM + offset]
  as.mov(eax, dword_ptr(ebp, instr.operand()));
}

void emit_load_s_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // ALT = [FRM + offset]
  as.mov(ecx, dword_ptr(ebp, instr.operand()));
}

void emit_lref_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = [ [address] ]
  as.mov(edx, dword_ptr(ebx, instr.operand()));
  as.mov(eax, dword_ptr(ebx, edx));
}

void emit_lref_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // ALT = [ [address] ]
  as.mov(edx, dword_ptr(ebx, + instr.operand()));
  as.mov(ecx, dword_ptr(ebx, edx));
}

void emit_lref_s_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = [ [FRM + offset] ]
  as.mov(edx, dword_ptr(ebp, instr.operand()));
  as.mov(eax, dword_ptr(ebx, edx));
}

void emit_lref_s_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = [ [FRM + offset] ]
  as.mov(edx, dword_ptr(ebp, instr.operand()));
  as.mov(ecx, dword_ptr(ebx, edx));
}

void emit_load_i(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = [PRI] (full cell)
  as.mov(eax, dword_ptr(ebx, eax));
}

void emit_lodb_i(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = "number" bytes from [PRI] (read 1/2/4 bytes)
  switch (instr.operand()) {
    case 1:
      as.movzx(eax, byte_ptr(ebx, eax));
      break;
    case 2:
      as.movzx(eax, word_ptr(ebx, eax));
      break;
    case 4:
      as.mov(eax, dword_ptr(ebx, eax));
      break;
    default:
      *error = true;
  }
}

void emit_const_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = value
  if (instr.operand() == 0) {
    as.xor_(eax, eax);
  } else {
    as.mov(eax, instr.operand());
  }
}

void emit_const_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // ALT = value
  if (instr.operand() == 0) {
    as.xor_(ecx, ecx);
  } else {
    as.mov(ecx, instr.operand());
  }
}

void emit_addr_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = FRM + offset
  as.lea(eax, dword_ptr(ebp, instr.operand()));
  as.sub(eax, ebx);
}

void emit_addr_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // ALT = FRM + offset
  as.lea(ecx, dword_ptr(ebp, instr.operand()));
  as.sub(ecx, ebx);
}

void emit_stor_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [address] = PRI
  as.mov(dword_ptr(ebx, instr.operand()), eax);
}

void emit_stor_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [address] = ALT
  as.mov(dword_ptr(ebx, instr.operand()), ecx);
}

void emit_stor_s_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [FRM + offset] = ALT
  as.mov(dword_ptr(ebp, instr.operand()), eax);
}

void emit_stor_s_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [FRM + offset] = ALT
  as.mov(dword_ptr(ebp, instr.operand()), ecx);
}

void emit_sref_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [ [address] ] = PRI
  as.mov(edx, dword_ptr(ebx, instr.operand()));
  as.mov(dword_ptr(ebx, edx), eax);
}

void emit_sref_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [ [address] ] = ALT
  as.mov(edx, dword_ptr(ebx, instr.operand()));
  as.mov(dword_ptr(ebx, edx), ecx);
}

void emit_sref_s_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [ [FRM + offset] ] = PRI
  as.mov(edx, dword_ptr(ebp, instr.operand()));
  as.mov(dword_ptr(ebx, edx), eax);
}

void emit_sref_s_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [ [FRM + offset] ] = ALT
  as.mov(edx, dword_ptr(ebp, instr.operand()));
  as.mov(dword_ptr(ebx, edx), ecx);
}

void emit_stor_i(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [ALT] = PRI (full cell)
  as.mov(dword_ptr(ebx, ecx), eax);
}

void emit_strb_i(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // "number" bytes at [ALT] = PRI (write 1/2/4 bytes)
  switch (instr.operand()) {
    case 1:
      as.mov(byte_ptr(ebx, ecx), al);
      break;
    case 2:
      as.mov(word_ptr(ebx, ecx), ax);
      break;
    case 4:
      as.mov(dword_ptr(ebx, ecx), eax);
      break;
    default:
      *error = true;
  }
}

void emit_lidx(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = [ ALT + (PRI x cell size) ]
  as.lea(edx, dword_ptr(ebx, ecx));
  as.mov(eax, dword_ptr(edx, eax, 2));
}

void emit_lidx_b(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = [ ALT + (PRI << shift) ]
  as.lea(edx, dword_ptr(ebx, ecx));
  as.mov(eax, dword_ptr(edx, eax, instr.operand()));
}

void emit_idxaddr(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = ALT + (PRI x cell size) (calculate indexed address)
  as.lea(eax, dword_ptr(ecx, eax, 2));
}

void emit_idxaddr_b(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = ALT + (PRI << shift) (calculate indexed address)
  as.lea(eax, dword_ptr(ecx, eax, instr.operand()));
}

void emit_align_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // Little Endian:
  #if BYTE_ORDER == LITTLE_ENDIAN
    if (static_cast<std::size_t>(instr.operand()) < sizeof(cell)) {
      as.xor_(eax, sizeof(cell) - instr.operand());
    }
  #endif
}

void emit_align_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // Little Endian:
  #if BYTE_ORDER == LITTLE_ENDIAN
    if (static_cast<std::size_t>(instr.operand()) < sizeof(cell)) {
      as.xor_(ecx, sizeof(cell) - instr.operand());
    }
  #endif
}

void emit_lctrl(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI is set to the current value of any of the special registers.
  // The index parameter must be:
  // 3=STP, 4=STK, 5=FRM, 6=CIP (of the next instruction)
  switch (instr.operand()) {
    case 0:
    case 1:
    case 2:
    case 3:
      emit_get_amx_ptr(as, eax);
      switch (instr.operand()) {
        case 0:
          as.mov(eax, dword_ptr(eax, offsetof(AMX, base)));
          as.mov(eax, dword_ptr(edx, offsetof(AMX_HEADER, cod)));
          break;
        case 1:
          as.mov(eax, dword_ptr(eax, offsetof(AMX, base)));
          as.mov(eax, dword_ptr(edx, offsetof(AMX_HEADER, dat)));
          break;
        case 2:
          as.mov(eax, dword_ptr(eax, offsetof(AMX, hea)));
          break;
        case 3:
          as.mov(eax, dword_ptr(eax, offsetof(AMX, stp)));
          break;
      }
      break;
    case 4:
      as.mov(eax, esp);
      as.sub(eax, ebx);
      break;
    case 5:
      as.mov(eax, ebp);
      as.sub(eax, ebx);
      break;
    case 6:
      as.mov(eax, instr.address() + instr.size());
      break;
    case 7:
      as.mov(eax, 1);
      break;
    default:
      *error = true;
  }
}

void emit_sctrl(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // set the indexed special registers to the value in PRI.
  // The index parameter must be:
  // 6=CIP
  switch (instr.operand()) {
    case 2:
      emit_get_amx_ptr(as, edx);
      as.mov(dword_ptr(edx, offsetof(AMX, hea)), eax);
      break;
    case 4:
      as.lea(esp, dword_ptr(ebx, eax));
      break;
    case 5:
      as.lea(ebp, dword_ptr(ebx, eax));
      break;
    case 6: {
      const Label &L_jump_helper = as.getFixedLabel(LabelJumpHelper);
      as.mov(edi, esp);
      as.mov(esi, ebp);
      as.mov(edx, eax);
      as.call(L_jump_helper);
      break;
    }
    default:
      *error = true;
  }
}

void emit_move_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = ALT
  as.mov(eax, ecx);
}

void emit_move_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // ALT = PRI
  as.mov(ecx, eax);
}

void emit_xchg(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // Exchange PRI and ALT
  as.xchg(eax, ecx);
}

void emit_push_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [STK] = PRI, STK = STK - cell size
  as.push(eax);
}

void emit_push_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [STK] = ALT, STK = STK - cell size
  as.push(ecx);
}

void emit_push_c(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [STK] = value, STK = STK - cell size
  as.push(instr.operand());
}

void emit_push(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [STK] = [address], STK = STK - cell size
  as.push(dword_ptr(ebx, instr.operand()));
}

void emit_push_s(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [STK] = [FRM + offset], STK = STK - cell size
  as.push(dword_ptr(ebp, instr.operand()));
}

void emit_pop_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // STK = STK + cell size, PRI = [STK]
  as.pop(eax);
}

void emit_pop_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // STK = STK + cell size, ALT = [STK]
  as.pop(ecx);
}

void emit_stack(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // ALT = STK, STK = STK + value
  as.mov(ecx, esp);
  as.sub(ecx, ebx);
  if (instr.operand() >= 0) {
    as.add(esp, instr.operand());
  } else {
    as.sub(esp, -instr.operand());
  }
}

void emit_heap(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // ALT = HEA, HEA = HEA + value
  emit_get_amx_ptr(as, edx);
  as.mov(ecx, dword_ptr(edx, offsetof(AMX, hea)));
  if (instr.operand() >= 0) {
    as.add(dword_ptr(edx, offsetof(AMX, hea)), instr.operand());
  } else {
    as.sub(dword_ptr(edx, offsetof(AMX, hea)), -instr.operand());
  }
}

void emit_proc(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [STK] = FRM, STK = STK - cell size, FRM = STK
  as.push(ebp);
  as.mov(ebp, esp);
  as.sub(dword_ptr(esp), ebx);
}

void emit_ret(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // STK = STK + cell size, FRM = [STK],
  // CIP = [STK], STK = STK + cell size
  as.pop(ebp);
  as.add(ebp, ebx);
  as.ret();
}

void emit_retn(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // STK = STK + cell size, FRM = [STK],
  // CIP = [STK], STK = STK + cell size
  // The RETN instruction removes a specified number of bytes
  // from the stack. The value to adjust STK with must be
  // pushed prior to the call.
  as.pop(ebp);
  as.add(ebp, ebx);
  as.pop(edx);
  as.add(esp, dword_ptr(esp));
  as.push(edx);
  as.ret(4);
}

void emit_call(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [STK] = CIP + 5, STK = STK - cell size
  // CIP = CIP + offset
  // The CALL instruction jumps to an address after storing the
  // address of the next sequential instruction on the stack.
  // The address jumped to is relative to the current CIP,
  // but the address on the stack is an absolute address.
  cell dest = rel_code_addr(as.getAmx(), instr.operand());
  as.call(as.getAmxLabel(dest));
}

void emit_jump_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // CIP = PRI (indirect jump)
  const Label &L_jump_helper = as.getFixedLabel(LabelJumpHelper);
  as.mov(edi, esp);
  as.mov(esi, ebp);
  as.mov(edx, eax);
  as.call(L_jump_helper);
}

const Label &get_jump_label(Assembler &as,
                                   const jit::AMXInstruction &instr) {
  cell dest = rel_code_addr(as.getAmx(), instr.operand());
  return as.getAmxLabel(dest);
}

void emit_jump(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // CIP = CIP + offset (jump to the address relative from
  // the current position)
  as.jmp(get_jump_label(as, instr));
}

void emit_jzer(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // if PRI == 0 then CIP = CIP + offset
  as.test(eax, eax);
  as.jz(get_jump_label(as, instr));
}

void emit_jnz(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // if PRI != 0 then CIP = CIP + offset
  as.test(eax, eax);
  as.jnz(get_jump_label(as, instr));
}

void emit_jeq(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // if PRI == ALT then CIP = CIP + offset
  as.cmp(eax, ecx);
  as.je(get_jump_label(as, instr));
}

void emit_jneq(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // if PRI != ALT then CIP = CIP + offset
  as.cmp(eax, ecx);
  as.jne(get_jump_label(as, instr));
}

void emit_jless(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // if PRI < ALT then CIP = CIP + offset (unsigned)
  as.cmp(eax, ecx);
  as.jb(get_jump_label(as, instr));
}

void emit_jleq(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // if PRI <= ALT then CIP = CIP + offset (unsigned)
  as.cmp(eax, ecx);
  as.jbe(get_jump_label(as, instr));
}

void emit_jgrtr(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // if PRI > ALT then CIP = CIP + offset (unsigned)
  as.cmp(eax, ecx);
  as.ja(get_jump_label(as, instr));
}

void emit_jgeq(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // if PRI >= ALT then CIP = CIP + offset (unsigned)
  as.cmp(eax, ecx);
  as.jae(get_jump_label(as, instr));
}

void emit_jsless(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // if PRI < ALT then CIP = CIP + offset (signed)
  as.cmp(eax, ecx);
  as.jl(get_jump_label(as, instr));
}

void emit_jsleq(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // if PRI <= ALT then CIP = CIP + offset (signed)
  as.cmp(eax, ecx);
  as.jle(get_jump_label(as, instr));
}

void emit_jsgrtr(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // if PRI > ALT then CIP = CIP + offset (signed)
  as.cmp(eax, ecx);
  as.jg(get_jump_label(as, instr));
}

void emit_jsgeq(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // if PRI >= ALT then CIP = CIP + offset (signed)
  as.cmp(eax, ecx);
  as.jge(get_jump_label(as, instr));
}

void emit_shl(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI << ALT
  as.shl(eax, cl);
}

void emit_shr(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI >> ALT (without sign extension)
  as.shr(eax, cl);
}

void emit_sshr(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI >> ALT with sign extension
  as.sar(eax, cl);
}

void emit_shl_c_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI << value
  as.shl(eax, static_cast<unsigned char>(instr.operand()));
}

void emit_shl_c_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // ALT = ALT << value
  as.shl(ecx, static_cast<unsigned char>(instr.operand()));
}

void emit_shr_c_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI >> value (without sign extension)
  as.shr(eax, static_cast<unsigned char>(instr.operand()));
}

void emit_shr_c_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI >> value (without sign extension)
  as.shl(ecx, static_cast<unsigned char>(instr.operand()));
}

void emit_smul(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI * ALT (signed multiply)
  as.xor_(edx, edx);
  as.imul(ecx);
}

void emit_sdiv(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI / ALT (signed divide), ALT = PRI mod ALT
  as.xor_(edx, edx);
  as.idiv(ecx);
  as.mov(ecx, edx);
}

void emit_sdiv_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = ALT / PRI (signed divide), ALT = ALT mod PRI
  as.xchg(eax, ecx);
  as.xor_(edx, edx);
  as.idiv(ecx);
  as.mov(ecx, edx);
}

void emit_umul(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI * ALT (unsigned multiply)
  as.xor_(edx, edx);
  as.mul(ecx);
}

void emit_udiv(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI / ALT (unsigned divide), ALT = PRI mod ALT
  as.xor_(edx, edx);
  as.div(ecx);
  as.mov(ecx, edx);
}

void emit_udiv_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = ALT / PRI (unsigned divide), ALT = ALT mod PRI
  as.xchg(eax, ecx);
  as.xor_(edx, edx);
  as.div(ecx);
  as.mov(ecx, edx);
}

void emit_add(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI + ALT
  as.add(eax, ecx);
}

void emit_sub(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI - ALT
  as.sub(eax, ecx);
}

void emit_sub_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = ALT - PRI
  // or:
  // PRI = -(PRI - ALT)
  as.sub(eax, ecx);
  as.neg(eax);
}

void emit_and(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI & ALT
  as.and_(eax, ecx);
}

void emit_or(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI | ALT
  as.or_(eax, ecx);
}

void emit_xor(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI ^ ALT
  as.xor_(eax, ecx);
}

void emit_not(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = !PRI
  as.test(eax, eax);
  as.setz(al);
  as.movzx(eax, al);
}

void emit_neg(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = -PRI
  as.neg(eax);
}

void emit_invert(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = ~PRI
  as.not_(eax);
}

void emit_add_c(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI + value
  if (instr.operand() >= 0) {
    as.add(eax, instr.operand());
  } else {
    as.sub(eax, -instr.operand());
  }
}

void emit_smul_c(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI * value
  as.imul(eax, instr.operand());
}

void emit_zero_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = 0
  as.xor_(eax, eax);
}

void emit_zero_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // ALT = 0
  as.xor_(ecx, ecx);
}

void emit_zero(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [address] = 0
  as.mov(dword_ptr(ebx, instr.operand()), 0);
}

void emit_zero_s(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [FRM + offset] = 0
  as.mov(dword_ptr(ebp, instr.operand()), 0);
}

void emit_sign_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // sign extent the byte in PRI to a cell
  as.movsx(eax, al);
}

void emit_sign_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // sign extent the byte in ALT to a cell
  as.movsx(ecx, cl);
}

void emit_eq(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI == ALT ? 1 :
  as.cmp(eax, ecx);
  as.sete(al);
  as.movzx(eax, al);
}

void emit_neq(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI != ALT ? 1 :
  as.cmp(eax, ecx);
  as.setne(al);
  as.movzx(eax, al);
}

void emit_less(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI < ALT ? 1 :
  as.cmp(eax, ecx);
  as.setb(al);
  as.movzx(eax, al);
}

void emit_leq(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI <= ALT ? 1 :
  as.cmp(eax, ecx);
  as.setbe(al);
  as.movzx(eax, al);
}

void emit_grtr(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI > ALT ? 1 :
  as.cmp(eax, ecx);
  as.seta(al);
  as.movzx(eax, al);
}

void emit_geq(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI >= ALT ? 1 :
  as.cmp(eax, ecx);
  as.setae(al);
  as.movzx(eax, al);
}

void emit_sless(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI < ALT ? 1 :
  as.cmp(eax, ecx);
  as.setl(al);
  as.movzx(eax, al);
}

void emit_sleq(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI <= ALT ? 1 :
  as.cmp(eax, ecx);
  as.setle(al);
  as.movzx(eax, al);
}

void emit_sgrtr(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI > ALT ? 1 :
  as.cmp(eax, ecx);
  as.setg(al);
  as.movzx(eax, al);
}

void emit_sgeq(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI >= ALT ? 1 :
  as.cmp(eax, ecx);
  as.setge(al);
  as.movzx(eax, al);
}

void emit_eq_c_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI == value ? 1 :
  as.cmp(eax, instr.operand());
  as.sete(al);
  as.movzx(eax, al);
}

void emit_eq_c_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = ALT == value ? 1 :
  as.cmp(ecx, instr.operand());
  as.sete(al);
  as.movzx(eax, al);
}

void emit_inc_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI + 1
  as.inc(eax);
}

void emit_inc_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // ALT = ALT + 1
  as.inc(ecx);
}

void emit_inc(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [address] = [address] + 1
  as.inc(dword_ptr(ebx, instr.operand()));
}

void emit_inc_s(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [FRM + offset] = [FRM + offset] + 1
  as.inc(dword_ptr(ebp, instr.operand()));
}

void emit_inc_i(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [PRI] = [PRI] + 1
  as.inc(dword_ptr(ebx, eax));
}

void emit_dec_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // PRI = PRI - 1
  as.dec(eax);
}

void emit_dec_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // ALT = ALT - 1
  as.dec(ecx);
}

void emit_dec(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [address] = [address] - 1
  as.dec(dword_ptr(ebx, instr.operand()));
}

void emit_dec_s(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [FRM + offset] = [FRM + offset] - 1
  as.dec(dword_ptr(ebp, instr.operand()));
}

void emit_dec_i(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [PRI] = [PRI] - 1
  as.dec(dword_ptr(ebx, eax));
}

void emit_movs(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // Copy memory from [PRI] to [ALT]. The parameter
  // specifies the number of bytes. The blocks should not
  // overlap.
  as.cld();
  as.lea(esi, dword_ptr(ebx, eax));
  as.lea(edi, dword_ptr(ebx, ecx));
  as.push(ecx);
  if (instr.operand() % 4 == 0) {
    as.mov(ecx, instr.operand() / 4);
    as.rep_movsd();
  } else if (instr.operand() % 2 == 0) {
    as.mov(ecx, instr.operand() / 2);
    as.rep_movsw();
  } else {
    as.mov(ecx, instr.operand());
    as.rep_movsb();
  }
  as.pop(ecx);
}

void emit_cmps(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // Compare memory blocks at [PRI] and [ALT]. The parameter
  // specifies the number of bytes. The blocks should not
  // overlap.
  Label L_above = as.newLabel();
  Label L_below = as.newLabel();
  Label L_equal = as.newLabel();
  Label L_continue = as.newLabel();
    as.cld();
    as.lea(edi, dword_ptr(ebx, eax));
    as.lea(esi, dword_ptr(ebx, ecx));
    as.push(ecx);
    as.mov(ecx, instr.operand());
    as.repe_cmpsb();
    as.pop(ecx);
    as.ja(L_above);
    as.jb(L_below);
    as.jz(L_equal);
  as.bind(L_above);
    as.mov(eax, 1);
    as.jmp(L_continue);
  as.bind(L_below);
    as.mov(eax, -1);
    as.jmp(L_continue);
  as.bind(L_equal);
    as.xor_(eax, eax);
  as.bind(L_continue);
}

void emit_fill(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // Fill memory at [ALT] with value in [PRI]. The parameter
  // specifies the number of bytes, which must be a multiple
  // of the cell size.
  as.cld();
  as.lea(edi, dword_ptr(ebx, ecx));
  as.push(ecx);
  as.mov(ecx, instr.operand() / sizeof(cell));
  as.rep_stosd();
  as.pop(ecx);
}

void emit_halt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // Abort execution (exit value in PRI), parameters other than 0
  // have a special meaning.
  const Label &L_halt_helper = as.getFixedLabel(LabelHaltHelper);
  as.mov(edx, instr.operand());
  as.jmp(L_halt_helper);
}

void emit_bounds(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // Abort execution if PRI > value or if PRI < 0.
  const Label &L_halt_helper = as.getFixedLabel(LabelHaltHelper);
  Label L_halt = as.newLabel();
  Label L_good = as.newLabel();

    as.cmp(eax, instr.operand());
    as.jg(L_halt);

    as.test(eax, eax);
    as.jl(L_halt);

    as.jmp(L_good);

  as.bind(L_halt);
    as.mov(edx, AMX_ERR_BOUNDS);
    as.jmp(L_halt_helper);

  as.bind(L_good);
}

void emit_sysreq_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // call system service, service number in PRI
  const Label &L_sysreq_c_helper = as.getFixedLabel(LabelSysreqCHelper);
  as.push(esp); // stack_ptr
  as.push(ebp); // stack_base
  as.push(eax); // index
  as.call(L_sysreq_c_helper);
}

void emit_sysreq_c(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // call system service
  const Label &L_sysreq_c_helper = as.getFixedLabel(LabelSysreqCHelper);
  const char *name = as.getAmx().get_native_name(instr.operand());
  if (name == 0) {
    *error = true;
  } else {
    if (!emit_intrinsic(as, name)) {
      as.push(esp); // stack_ptr
      as.push(ebp); // stack_base
      as.push(instr.operand()); // index
      as.call(L_sysreq_c_helper);
    }
  }
}

void emit_sysreq_d(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // call system service
  const Label &L_sysreq_d_helper = as.getFixedLabel(LabelSysreqDHelper);
  cell index = as.getAmx().find_native(instr.operand());
  const char *name = as.getAmx().get_native_name(index);
  if (name == 0) {
    *error = true;
  } else {
    if (!emit_intrinsic(as, name)) {
      as.push(esp); // stack_ptr
      as.push(ebp); // stack_base
      as.push(instr.operand()); // address
      as.call(L_sysreq_d_helper);
    }
  }
}

void emit_switch(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // Compare PRI to the values in the case table (whose address
  // is passed as an offset from CIP) and jump to the associated
  // the address in the matching record.

  struct CaseRecord {
    cell value;    // case value
    cell address;  // address to jump to (absolute)
  } *case_table;

  // Get pointer to the start of the case table.
  case_table = reinterpret_cast<CaseRecord*>(instr.operand() + sizeof(cell));

  // Get address of the "default" record.
  cell default_case = rel_code_addr(as.getAmx(), case_table[0].address);

  // The number of cases follows the CASETBL opcode (which follows the SWITCH).
  int num_records = *(reinterpret_cast<cell*>(instr.operand()) + 1);

  if (num_records > 0) {
    // Get minimum and maximum values.
    cell *min_value = 0;
    cell *max_value = 0;
    for (int i = 0; i < num_records; i++) {
      cell *value = &case_table[i + 1].value;
      if (min_value == 0 || *value < *min_value) {
        min_value = value;
      }
      if (max_value == 0 || *value > *max_value) {
        max_value = value;
      }
    }

    // Check if the value in eax is in the allowed range.
    // If not, jump to the default case (i.e. no match).
    as.cmp(eax, *min_value);
    as.jl(as.getAmxLabel(default_case));
    as.cmp(eax, *max_value);
    as.jg(as.getAmxLabel(default_case));

    // OK now sequentially compare eax with each value.
    // This is pretty slow so I probably should optimize
    // this in future...
    for (int i = 0; i < num_records; i++) {
      cell dest = rel_code_addr(as.getAmx(), case_table[i + 1].address);
      as.cmp(eax, case_table[i + 1].value);
      as.je(as.getAmxLabel(dest));
    }
  }

  // No match found - go for default case.
  as.jmp(as.getAmxLabel(default_case));
}

void emit_casetbl(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // A variable number of case records follows this opcode, where
  // each record takes two cells.
}

void emit_swap_pri(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [STK] = PRI and PRI = [STK]
  as.xchg(dword_ptr(esp), eax);
}

void emit_swap_alt(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [STK] = ALT and ALT = [STK]
  as.xchg(dword_ptr(esp), ecx);
}

void emit_push_adr(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // [STK] = FRM + offset, STK = STK - cell size
  as.lea(edx, dword_ptr(ebp, instr.operand()));
  as.sub(edx, ebx);
  as.push(edx);
}

void emit_nop(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // no-operation, for code alignment
}

void emit_break(Assembler &as, const jit::AMXInstruction &instr, bool *error) {
  // conditional breakpoint
  #ifdef DEBUG
    as.call(as.getFixedLabel(LabelBreakHelper));
  #endif
}

// 0 means obsolete opcode (not implemented and thus won't compile).
EmitInstruction emit_opcode[] = {
  0,                 emit_load_pri,     emit_load_alt,     emit_load_s_pri,
  emit_load_s_alt,   emit_lref_pri,     emit_lref_alt,     emit_lref_s_pri,
  emit_lref_s_alt,   emit_load_i,       emit_lodb_i,       emit_const_pri,
  emit_const_alt,    emit_addr_pri,     emit_addr_alt,     emit_stor_pri,
  emit_stor_alt,     emit_stor_s_pri,   emit_stor_s_alt,   emit_sref_pri,
  emit_sref_alt,     emit_sref_s_pri,   emit_sref_s_alt,   emit_stor_i,
  emit_strb_i,       emit_lidx,         emit_lidx_b,       emit_idxaddr,
  emit_idxaddr_b,    emit_align_pri,    emit_align_alt,    emit_lctrl,
  emit_sctrl,        emit_move_pri,     emit_move_alt,     emit_xchg,
  emit_push_pri,     emit_push_alt,     0,                 emit_push_c,
  emit_push,         emit_push_s,       emit_pop_pri,      emit_pop_alt,
  emit_stack,        emit_heap,         emit_proc,         emit_ret,
  emit_retn,         emit_call,         0,                 emit_jump,
  0,                 emit_jzer,         emit_jnz,          emit_jeq,
  emit_jneq,         emit_jless,        emit_jleq,         emit_jgrtr,
  emit_jgeq,         emit_jsless,       emit_jsleq,        emit_jsgrtr,
  emit_jsgeq,        emit_shl,          emit_shr,          emit_sshr,
  emit_shl_c_pri,    emit_shl_c_alt,    emit_shr_c_pri,    emit_shr_c_alt,
  emit_smul,         emit_sdiv,         emit_sdiv_alt,     emit_umul,
  emit_udiv,         emit_udiv_alt,     emit_add,          emit_sub,
  emit_sub_alt,      emit_and,          emit_or,           emit_xor,
  emit_not,          emit_neg,          emit_invert,       emit_add_c,
  emit_smul_c,       emit_zero_pri,     emit_zero_alt,     emit_zero,
  emit_zero_s,       emit_sign_pri,     emit_sign_alt,     emit_eq,
  emit_neq,          emit_less,         emit_leq,          emit_grtr,
  emit_geq,          emit_sless,        emit_sleq,         emit_sgrtr,
  emit_sgeq,         emit_eq_c_pri,     emit_eq_c_alt,     emit_inc_pri,
  emit_inc_alt,      emit_inc,          emit_inc_s,        emit_inc_i,
  emit_dec_pri,      emit_dec_alt,      emit_dec,          emit_dec_s,
  emit_dec_i,        emit_movs,         emit_cmps,         emit_fill,
  emit_halt,         emit_bounds,       emit_sysreq_pri,   emit_sysreq_c,
  0,                 0,                 0,                 0,
  emit_jump_pri,     emit_switch,       emit_casetbl,      emit_swap_pri,
  emit_swap_alt,     emit_push_adr,     emit_nop,          emit_sysreq_d,
  0,                 emit_break
};

} // anonymouse namespace

namespace jit {

CompilerOutputAsmjit::CompilerOutputAsmjit(void *code, std::size_t code_size)
  : code_(code), code_size_(code_size)
{
}

CompilerOutputAsmjit::~CompilerOutputAsmjit() {
  AsmJit::MemoryManager::getGlobal()->free(code_);
}

CompilerAsmjit::CompilerAsmjit() {}
CompilerAsmjit::~CompilerAsmjit() {}

CompilerOutput *CompilerAsmjit::compile(AMXPtr amx,
                                        CompileErrorHandler *error_handler)
{ 
  Assembler as(amx);

  std::vector<Label> labels;
  for (int i = 0; i < LabelLast_; i++) {
    labels.push_back(as.newLabel());
  }

  as.setFixedLabels(labels);

  emit_runtime_data(as);
  emit_instr_map(as);
  emit_exec(as);
  emit_exec_helper(as);
  emit_halt_helper(as);
  emit_jump_helper(as);
  emit_sysreq_c_helper(as);
  emit_sysreq_d_helper(as);
  emit_break_helper(as);

  std::set<cell> jump_targets;
  collect_jump_targets(amx, jump_targets);

  AMXDisassembler disas(amx);
  AMXInstruction instr;
  bool error = false;

  typedef std::vector<std::pair<cell, int> > InstrMap;
  InstrMap instr_map;

  while (!error && disas.decode(instr, &error)) {
    cell cip = instr.address();
    
    instr_map.push_back(std::make_pair(cip, as.getCodeSize()));

    if (instr.opcode().id() == AMX_OP_PROC) {
      as.align(16);
    }

    if (jump_targets.find(cip) != jump_targets.end()) {
      as.bind(as.getAmxLabel(cip));
    }

    AMXOpcodeID opcode = instr.opcode().id();
    if (opcode >= 0 &&
        opcode < sizeof(emit_opcode) / sizeof(emit_opcode[0])) {
      emit_opcode[opcode](as, instr, &error);
    } else {
      error = true;
    }
  }

  if (error) {
    if (error_handler != 0) {
      error_handler->execute(instr);
    }
    return 0;
  }

  intptr_t code_ptr = reinterpret_cast<intptr_t>(as.make());
  size_t code_size = as.getCodeSize();

  intptr_t *runtime_data = reinterpret_cast<intptr_t*>(code_ptr);

  runtime_data[RuntimeDataExecPtr] += code_ptr;
  runtime_data[RuntimeDataInstrMapPtr] += code_ptr;

  intptr_t instr_map_ptr = runtime_data[RuntimeDataInstrMapPtr];
  InstrMapEntry *entry = reinterpret_cast<InstrMapEntry*>(instr_map_ptr);

  for (std::size_t i = 0; i < instr_map.size(); i++) {
    entry->amx_addr = instr_map[i].first;
    entry->jit_addr = reinterpret_cast<void*>(instr_map[i].second + code_ptr);
    entry++;
  }

  return new CompilerOutputAsmjit(reinterpret_cast<void*>(code_ptr), code_size);
}

} // namespace jit
