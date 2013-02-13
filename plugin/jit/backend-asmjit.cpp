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
#include "backend-asmjit.h"
#include "callconv.h"
#include "compiler.h"

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

enum RuntimeDataIndex {
  RuntimeDataExecPtr = jit::BackendRuntimeDataExec,
  RuntimeDataAmxPtr,
  RuntimeDataEbp,
  RuntimeDataEsp,
  RuntimeDataResetEbp,
  RuntimeDataResetEsp,
  RuntimeDataInstrMapSize,
  RuntimeDataInstrMapPtr
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

class NamedLabel : public Label {
 public:
  NamedLabel(const char *name, const Label &label)
    : Label(label), name_(name)
  {}

  NamedLabel(const char *name)
   : Label(), name_(name)
  {}

  std::string name() const { return name_; }

  bool operator<(const NamedLabel &rhs) const {
    return name_ < rhs.name_;
  }

 private:
  std::string name_;
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

class Assembler : public X86Assembler {
 public:
  Assembler(jit::AMXPtr amx) : amx_(amx) {}
  virtual ~Assembler() {}

  jit::AMXPtr &amx() { return amx_; }
  const jit::AMXPtr &amx() const { return amx_; }

  const NamedLabel &getLabel(const char *name) {
    assert(name != 0);
    std::set<NamedLabel>::const_iterator it = labels_.find(name);
    if (it != labels_.end()) {
      return *it;
    } else {
      std::pair<std::set<NamedLabel>::iterator, bool> result =
        labels_.insert(NamedLabel(name, newLabel()));
      return *result.first;
    }
  }

  const AddressedLabel &getAmxLabel(cell address) {
    std::set<AddressedLabel>::const_iterator it = amx_labels_.find(address);
    if (it != amx_labels_.end()) {
      return *it;
    } else {
      std::pair<std::set<AddressedLabel>::iterator, bool> result =
        amx_labels_.insert(AddressedLabel(address, newLabel()));
      return *result.first;
    }
  }

  void bindByName(const char *name) {
    bind(getLabel(name));
  }

 private:
  jit::AMXPtr amx_;
  std::set<NamedLabel> labels_;
  std::set<AddressedLabel> amx_labels_;
};

static void emit_float(Assembler &as) {
  as.fild(dword_ptr(esp, 4));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

static void emit_floatabs(Assembler &as) {
  as.fld(dword_ptr(esp, 4));
  as.fabs();
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

static void emit_floatadd(Assembler &as) {
  as.fld(dword_ptr(esp, 4));
  as.fadd(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

static void emit_floatsub(Assembler &as) {
  as.fld(dword_ptr(esp, 4));
  as.fsub(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

static void emit_floatmul(Assembler &as) {
  as.fld(dword_ptr(esp, 4));
  as.fmul(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

static void emit_floatdiv(Assembler &as) {
  as.fld(dword_ptr(esp, 4));
  as.fdiv(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

static void emit_floatsqroot(Assembler &as) {
  as.fld(dword_ptr(esp, 4));
  as.fsqrt();
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

static void emit_floatlog(Assembler &as) {
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

static bool emit_intrinsic(Assembler &as, const char *name) {
  static Intrinsic intrinsics[] = {
    {"float",       &emit_float},
    {"floatabs",    &emit_floatabs},
    {"floatadd",    &emit_floatadd},
    {"floatsub",    &emit_floatsub},
    {"floatmul",    &emit_floatmul},
    {"floatdiv",    &emit_floatdiv},
    {"floatsqroot", &emit_floatsqroot},
    {"floatlog",    &emit_floatlog}
  };

  for (int i = 0; i < sizeof(intrinsics) / sizeof(*intrinsics); i++) {
    if (std::strcmp(intrinsics[i].name, name) == 0) {
      intrinsics[i].emit(as);
      return true;
    }
  }

  return false;
}

static cell JIT_CDECL get_public_addr(AMX *amx, int index) {
  return jit::AMXPtr(amx).get_public_addr(index);
}

static cell JIT_CDECL get_native_addr(AMX *amx, int index) {
  return jit::AMXPtr(amx).get_native_addr(index);
}

static void *JIT_CDECL get_instr_ptr(cell address, void *instr_map,
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

static inline cell rel_code_addr(jit::AMXPtr amx, cell address) {
  return address - reinterpret_cast<cell>(amx.code());
}

static void collect_jump_targets(jit::AMXPtr amx, std::set<cell> &refs) {
  jit::AMXDisassembler disas(amx);
  jit::AMXInstruction instr;

  while (disas.decode(instr)) {
    jit::AMXOpcode opcode = instr.opcode();
    if (opcode.is_call() || opcode.is_jump() && instr.num_operands() == 1) {
      refs.insert(rel_code_addr(amx, instr.operand()));
    } else if (opcode.id() == jit::OP_CASETBL) {
      int n = instr.num_operands();
      for (int i = 1; i < n; i += 2) {
        refs.insert(rel_code_addr(amx, instr.operand(i)));
      }
    } else if (opcode.id() == jit::OP_PROC) {
      refs.insert(instr.address());
    }
  }
}

static intptr_t *get_runtime_data(Assembler &as) {
  return reinterpret_cast<intptr_t*>(as.getCode());
}

static void set_runtime_data(Assembler &as, int index, intptr_t data) {
  get_runtime_data(as)[index] = data;
}

static void emit_runtime_data(Assembler &as) {
  as.bindByName("exec_ptr");
    as.dd(0);
  as.bindByName("amx");
    as.dintptr(reinterpret_cast<intptr_t>(as.amx().amx()));
  as.bindByName("ebp");
    as.dd(0);
  as.bindByName("esp");
    as.dd(0);
  as.bindByName("reset_ebp");
    as.dd(0);
  as.bindByName("reset_esp");
    as.dd(0);
  as.bindByName("instr_map_size");
    as.dd(0);
  as.bindByName("instr_map");
    as.dd(0);
}

static void emit_instr_map(Assembler &as) {
  jit::AMXDisassembler disas(as.amx());
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

static void emit_get_amx_ptr(Assembler &as, const Label &amx_ptr,
                             const GpReg &reg) {
  as.mov(reg, dword_ptr(amx_ptr));
}

static void emit_get_amx_data_ptr(Assembler &as, const Label &amx_ptr,
                                  const GpReg &reg) {
  Label L_quit = as.newLabel();

    emit_get_amx_ptr(as, amx_ptr, eax);

    as.mov(reg, dword_ptr(eax, offsetof(AMX, data)));
    as.cmp(reg, 0);
    as.jnz(L_quit);

    as.mov(reg, dword_ptr(eax, offsetof(AMX, base)));
    as.mov(eax, dword_ptr(reg, offsetof(AMX_HEADER, dat)));
    as.add(reg, eax);

  as.bind(L_quit);
}

// int JIT_CDECL exec(cell index, cell *retval);
static void emit_exec(Assembler &as) {
  set_runtime_data(as, RuntimeDataExecPtr, as.getCodeSize());

  const NamedLabel &L_amx = as.getLabel("amx");
  const NamedLabel &L_instr_map = as.getLabel("instr_map");
  const NamedLabel &L_instr_map_size = as.getLabel("instr_map_size");
  const NamedLabel &L_reset_ebp = as.getLabel("reset_ebp");
  const NamedLabel &L_reset_esp = as.getLabel("reset_esp");
  const NamedLabel &L_exec_helper = as.getLabel("exec_helper");
  Label L_do_call = as.newLabel();
  Label L_check_heap = as.newLabel();
  Label L_check_stack = as.newLabel();
  Label L_check_natives = as.newLabel();
  Label L_checks_done = as.newLabel();
  Label L_cleanup = as.newLabel();
  Label L_return = as.newLabel();

  // Offsets of exec() arguments relative to ebp.
  int arg_index = 8;
  int arg_retval = 12;
  int var_address = -4;
  int var_reset_ebp = -8;
  int var_reset_esp = -12;

  as.bindByName("exec");
    as.push(ebp);
    as.mov(ebp, esp);
    as.sub(esp, 12); // for locals

    as.push(esi);
    emit_get_amx_ptr(as, L_amx, esi);

    // JIT code expects AMX data pointer to be in ebx.
    as.push(ebx);
    emit_get_amx_data_ptr(as, L_amx, ebx);

    // if (amx->hea >= amx->stk) return AMX_ERR_STACKERR;
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, hea)));
    as.mov(edx, dword_ptr(esi, offsetof(AMX, stk)));
    as.cmp(ecx, edx);
    as.jl(L_check_heap);
    as.mov(eax, AMX_ERR_STACKERR);
    as.jmp(L_return);

  as.bind(L_check_heap);
    // if (amx->hea < amx->hlw) return AMX_ERR_HEAPLOW;
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, hea)));
    as.mov(edx, dword_ptr(esi, offsetof(AMX, hlw)));
    as.cmp(ecx, edx);
    as.jge(L_check_stack);
    as.mov(eax, AMX_ERR_HEAPLOW);
    as.jmp(L_return);

  as.bind(L_check_stack);
    // if (amx->stk > amx->stp) return AMX_ERR_STACKLOW;
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, stk)));
    as.mov(edx, dword_ptr(esi, offsetof(AMX, stp)));
    as.cmp(ecx, edx);
    as.jle(L_check_natives);
    as.mov(eax, AMX_ERR_STACKLOW);
    as.jmp(L_return);

  // Make sure all natives are registered.
  as.bind(L_check_natives);
    // if ((amx->flags & AMX_FLAG_NTVREG) == 0) return AMX_ERR_NOTFOUND;
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, flags)));
    as.and_(ecx, AMX_FLAG_NTVREG);
    as.cmp(ecx, 0);
    as.jne(L_checks_done);
    as.mov(eax, AMX_ERR_NOTFOUND);
    as.jmp(L_return);

  as.bind(L_checks_done);
    // Reset the error code.
    as.mov(dword_ptr(esi, offsetof(AMX, error)), AMX_ERR_NONE);

    // Get address of the public function.
    as.push(dword_ptr(ebp, arg_index));
    emit_get_amx_ptr(as, L_amx, eax);
    as.push(eax);
    as.call(asmjit_cast<void*>(&get_public_addr));
    as.add(esp, 8);

    // If the function was not found, exit with error.
    as.cmp(eax, 0);
    as.jne(L_do_call);
    as.mov(eax, AMX_ERR_INDEX);
    as.jmp(L_return);

  as.bind(L_do_call);

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

    // Copt the return value if retval is not NULL.
    as.mov(ecx, dword_ptr(esp, arg_retval));
    as.cmp(ecx, 0);
    as.je(L_cleanup);
    as.mov(dword_ptr(ecx), eax);

  as.bind(L_cleanup);
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
}

// cell JIT_CDECL exec_helper(void *address);
static void emit_exec_helper(Assembler &as) {
  const NamedLabel &L_amx = as.getLabel("amx");
  const NamedLabel &L_ebp = as.getLabel("ebp");
  const NamedLabel &L_esp = as.getLabel("esp");
  const NamedLabel &L_reset_ebp = as.getLabel("reset_ebp");
  const NamedLabel &L_reset_esp = as.getLabel("reset_esp");

  as.bindByName("exec_helper");
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
    emit_get_amx_ptr(as, L_amx, ecx);
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
    emit_get_amx_ptr(as, L_amx, eax);
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

// void JIT_CDECL halt_helper(int error);
static void emit_halt_helper(Assembler &as) {
  const NamedLabel &L_amx = as.getLabel("amx");
  const NamedLabel &L_reset_ebp = as.getLabel("reset_ebp");
  const NamedLabel &L_reset_esp = as.getLabel("reset_esp");

  as.bindByName("halt_helper");
    as.mov(eax, dword_ptr(esp, 4));
    emit_get_amx_ptr(as, L_amx, ecx);
    as.mov(dword_ptr(ecx, offsetof(AMX, error)), eax);

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

// void JIT_CDECL jump_helper(void *address, void *stack_base, void *stack_ptr);
static void emit_jump_helper(Assembler &as) {
  const NamedLabel &L_instr_map = as.getLabel("instr_map");
  const NamedLabel &L_instr_map_size = as.getLabel("instr_map_size");
  Label L_do_jump = as.newLabel();

  int arg_address = 4;
  int arg_stack_base = 8;
  int arg_stack_ptr = 12;

  as.bindByName("jump_helper");
    as.mov(eax, dword_ptr(esp, arg_address));

    // Get destination address.
    as.push(dword_ptr(L_instr_map_size));
    as.push(dword_ptr(L_instr_map));
    as.push(eax);
    as.call(asmjit_cast<void*>(&get_instr_ptr));
    as.add(esp, 12);

    // If the address wasn't valid, continue execution as if no jump
    // was initiated (this is what AMX does).
    as.cmp(eax, 0);
    as.jne(L_do_jump);
    as.ret();

  as.bind(L_do_jump);

    // Jump to the destination address.
    as.mov(ebp, dword_ptr(esp, arg_stack_base));
    as.mov(esp, dword_ptr(esp, arg_stack_ptr));
    as.jmp(eax);
}

// cell JIT_CDECL sysreq_c_helper(int index, void *stack_base, void *stack_ptr);
static void emit_sysreq_c_helper(Assembler &as) {
  const NamedLabel &L_amx = as.getLabel("amx");
  const NamedLabel &L_sysreq_d_helper = as.getLabel("sysreq_d_helper");
  Label L_call = as.newLabel();
  Label L_return = as.newLabel();

  int arg_index = 8;
  int arg_stack_base = 12;
  int arg_stack_ptr = 16;

  as.bindByName("sysreq_c_helper");
    as.push(ebp);
    as.mov(ebp, esp);

    as.push(dword_ptr(ebp, arg_index));
    emit_get_amx_ptr(as, L_amx, eax);
    as.push(eax);
    as.call(asmjit_cast<void*>(&get_native_addr));
    as.add(esp, 8);

    as.cmp(eax, 0);
    as.jne(L_call);
    as.mov(eax, AMX_ERR_NOTFOUND);
    as.jmp(L_return);

  as.bind(L_call);
    as.push(dword_ptr(ebp, arg_stack_ptr));
    as.push(dword_ptr(ebp, arg_stack_base));
    as.push(eax); // address
    as.call(L_sysreq_d_helper);
    as.add(esp, 12);

  as.bind(L_return);
    as.mov(esp, ebp);
    as.pop(ebp);
    as.ret();
}

// cell sysreq_d_helper(void *address, void *stack_base, void *stack_ptr);
static void emit_sysreq_d_helper(Assembler &as) {
  const NamedLabel &L_amx = as.getLabel("amx");
  const NamedLabel &L_ebp = as.getLabel("ebp");
  const NamedLabel &L_esp = as.getLabel("esp");

  as.bindByName("sysreq_d_helper");
    as.mov(eax, dword_ptr(esp, 4));   // address
    as.mov(ebp, dword_ptr(esp, 8));   // stack_base
    as.mov(esp, dword_ptr(esp, 12));  // stack_ptr
    as.mov(ecx, esp);                 // params
    as.mov(esi, dword_ptr(esp, -16)); // return address

    emit_get_amx_ptr(as, L_amx, edx);

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
    emit_get_amx_ptr(as, L_amx, edx);
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

static void emit_load_pri(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // PRI = [address]
  as.mov(eax, dword_ptr(ebx, instr.operand()));
}

static void emit_load_alt(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // ALT = [address]
  as.mov(ecx, dword_ptr(ebx, instr.operand()));
}

static void emit_load_s_pri(Assembler &as, const jit::AMXInstruction &instr,
                            bool *error) {
  // PRI = [FRM + offset]
  as.mov(eax, dword_ptr(ebp, instr.operand()));
}

static void emit_load_s_alt(Assembler &as, const jit::AMXInstruction &instr,
                            bool *error) {
  // ALT = [FRM + offset]
  as.mov(ecx, dword_ptr(ebp, instr.operand()));
}

static void emit_lref_pri(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // PRI = [ [address] ]
  as.mov(edx, dword_ptr(ebx, instr.operand()));
  as.mov(eax, dword_ptr(ebx, edx));
}

static void emit_lref_alt(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // ALT = [ [address] ]
  as.mov(edx, dword_ptr(ebx, + instr.operand()));
  as.mov(ecx, dword_ptr(ebx, edx));
}

static void emit_lref_s_pri(Assembler &as, const jit::AMXInstruction &instr,
                            bool *error) {
  // PRI = [ [FRM + offset] ]
  as.mov(edx, dword_ptr(ebp, instr.operand()));
  as.mov(eax, dword_ptr(ebx, edx));
}

static void emit_lref_s_alt(Assembler &as, const jit::AMXInstruction &instr,
                            bool *error) {
  // PRI = [ [FRM + offset] ]
  as.mov(edx, dword_ptr(ebp, instr.operand()));
  as.mov(ecx, dword_ptr(ebx, edx));
}

static void emit_load_i(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
  // PRI = [PRI] (full cell)
  as.mov(eax, dword_ptr(ebx, eax));
}

static void emit_lodb_i(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
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

static void emit_const_pri(Assembler &as, const jit::AMXInstruction &instr,
                           bool *error) {
  // PRI = value
  if (instr.operand() == 0) {
    as.xor_(eax, eax);
  } else {
    as.mov(eax, instr.operand());
  }
}

static void emit_const_alt(Assembler &as, const jit::AMXInstruction &instr,
                           bool *error) {
  // ALT = value
  if (instr.operand() == 0) {
    as.xor_(ecx, ecx);
  } else {
    as.mov(ecx, instr.operand());
  }
}

static void emit_addr_pri(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // PRI = FRM + offset
  as.lea(eax, dword_ptr(ebp, instr.operand()));
  as.sub(eax, ebx);
}

static void emit_addr_alt(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // ALT = FRM + offset
  as.lea(ecx, dword_ptr(ebp, instr.operand()));
  as.sub(ecx, ebx);
}

static void emit_stor_pri(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // [address] = PRI
  as.mov(dword_ptr(ebx, instr.operand()), eax);
}

static void emit_stor_alt(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // [address] = ALT
  as.mov(dword_ptr(ebx, instr.operand()), ecx);
}

static void emit_stor_s_pri(Assembler &as, const jit::AMXInstruction &instr,
                            bool *error) {
  // [FRM + offset] = ALT
  as.mov(dword_ptr(ebp, instr.operand()), eax);
}

static void emit_stor_s_alt(Assembler &as, const jit::AMXInstruction &instr,
                            bool *error) {
  // [FRM + offset] = ALT
  as.mov(dword_ptr(ebp, instr.operand()), ecx);
}

static void emit_sref_pri(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // [ [address] ] = PRI
  as.mov(edx, dword_ptr(ebx, instr.operand()));
  as.mov(dword_ptr(ebx, edx), eax);
}

static void emit_sref_alt(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // [ [address] ] = ALT
  as.mov(edx, dword_ptr(ebx, instr.operand()));
  as.mov(dword_ptr(ebx, edx), ecx);
}

static void emit_sref_s_pri(Assembler &as, const jit::AMXInstruction &instr,
                            bool *error) {
  // [ [FRM + offset] ] = PRI
  as.mov(edx, dword_ptr(ebp, instr.operand()));
  as.mov(dword_ptr(ebx, edx), eax);
}

static void emit_sref_s_alt(Assembler &as, const jit::AMXInstruction &instr,
                            bool *error) {
  // [ [FRM + offset] ] = ALT
  as.mov(edx, dword_ptr(ebp, instr.operand()));
  as.mov(dword_ptr(ebx, edx), ecx);
}

static void emit_stor_i(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
  // [ALT] = PRI (full cell)
  as.mov(dword_ptr(ebx, ecx), eax);
}

static void emit_strb_i(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
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

static void emit_lidx(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // PRI = [ ALT + (PRI x cell size) ]
  as.lea(edx, dword_ptr(ebx, ecx));
  as.mov(eax, dword_ptr(edx, eax, 2));
}

static void emit_lidx_b(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
  // PRI = [ ALT + (PRI << shift) ]
  as.lea(edx, dword_ptr(ebx, ecx));
  as.mov(eax, dword_ptr(edx, eax, instr.operand()));
}

static void emit_idxaddr(Assembler &as, const jit::AMXInstruction &instr,
                         bool *error) {
  // PRI = ALT + (PRI x cell size) (calculate indexed address)
  as.lea(eax, dword_ptr(ecx, eax, 2));
}

static void emit_idxaddr_b(Assembler &as, const jit::AMXInstruction &instr,
                           bool *error) {
  // PRI = ALT + (PRI << shift) (calculate indexed address)
  as.lea(eax, dword_ptr(ecx, eax, instr.operand()));
}

static void emit_align_pri(Assembler &as, const jit::AMXInstruction &instr,
                           bool *error) {
  // Little Endian:
  #if BYTE_ORDER == LITTLE_ENDIAN
    if (instr.operand() < sizeof(cell)) {
      as.xor_(eax, sizeof(cell) - instr.operand());
    }
  #endif
}

static void emit_align_alt(Assembler &as, const jit::AMXInstruction &instr,
                           bool *error) {
  // Little Endian:
  #if BYTE_ORDER == LITTLE_ENDIAN
    if (instr.operand() < sizeof(cell)) {
      as.xor_(ecx, sizeof(cell) - instr.operand());
    }
  #endif
}

static void emit_lctrl(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // PRI is set to the current value of any of the special registers.
  // The index parameter must be:
  // 3=STP, 4=STK, 5=FRM, 6=CIP (of the next instruction)
  const NamedLabel &L_amx = as.getLabel("amx");
  switch (instr.operand()) {
    case 0:
    case 1:
    case 2:
    case 3:
      emit_get_amx_ptr(as, L_amx, eax);
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

static void emit_sctrl(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // set the indexed special registers to the value in PRI.
  // The index parameter must be:
  // 6=CIP
  const NamedLabel &L_amx = as.getLabel("amx");
  switch (instr.operand()) {
    case 2:
      emit_get_amx_ptr(as, L_amx, edx);
      as.mov(dword_ptr(edx, offsetof(AMX, hea)), eax);
      break;
    case 4:
      as.lea(esp, dword_ptr(ebx, eax));
      break;
    case 5:
      as.lea(ebp, dword_ptr(ebx, eax));
      break;
    case 6: {
      const NamedLabel &L_jump_helper = as.getLabel("jump_helper");
      as.push(esp);
      as.push(ebp);
      as.push(eax);
      as.call(L_jump_helper);
      break;
    }
    default:
      *error = true;
  }
}

static void emit_move_pri(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // PRI = ALT
  as.mov(eax, ecx);
}

static void emit_move_alt(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // ALT = PRI
  as.mov(ecx, eax);
}

static void emit_xchg(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // Exchange PRI and ALT
  as.xchg(eax, ecx);
}

static void emit_push_pri(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // [STK] = PRI, STK = STK - cell size
  as.push(eax);
}

static void emit_push_alt(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // [STK] = ALT, STK = STK - cell size
  as.push(ecx);
}

static void emit_push_c(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
  // [STK] = value, STK = STK - cell size
  as.push(instr.operand());
}

static void emit_push(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // [STK] = [address], STK = STK - cell size
  as.push(dword_ptr(ebx, instr.operand()));
}

static void emit_push_s(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
  // [STK] = [FRM + offset], STK = STK - cell size
  as.push(dword_ptr(ebp, instr.operand()));
}

static void emit_pop_pri(Assembler &as, const jit::AMXInstruction &instr,
                         bool *error) {
  // STK = STK + cell size, PRI = [STK]
  as.pop(eax);
}

static void emit_pop_alt(Assembler &as, const jit::AMXInstruction &instr,
                         bool *error) {
  // STK = STK + cell size, ALT = [STK]
  as.pop(ecx);
}

static void emit_stack(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // ALT = STK, STK = STK + value
  as.mov(ecx, esp);
  as.sub(ecx, ebx);
  if (instr.operand() >= 0) {
    as.add(esp, instr.operand());
  } else {
    as.sub(esp, -instr.operand());
  }
}

static void emit_heap(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // ALT = HEA, HEA = HEA + value
  const NamedLabel &L_amx = as.getLabel("amx");

  emit_get_amx_ptr(as, L_amx, edx);
  as.mov(ecx, dword_ptr(edx, offsetof(AMX, hea)));

  if (instr.operand() >= 0) {
    as.add(dword_ptr(edx, offsetof(AMX, hea)), instr.operand());
  } else {
    as.sub(dword_ptr(edx, offsetof(AMX, hea)), -instr.operand());
  }
}

static void emit_proc(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // [STK] = FRM, STK = STK - cell size, FRM = STK
  as.push(ebp);
  as.mov(ebp, esp);
  as.sub(dword_ptr(esp), ebx);
}

static void emit_ret(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // STK = STK + cell size, FRM = [STK],
  // CIP = [STK], STK = STK + cell size
  as.pop(ebp);
  as.add(ebp, ebx);
  as.ret();
}

static void emit_retn(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
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

static void emit_call(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // [STK] = CIP + 5, STK = STK - cell size
  // CIP = CIP + offset
  // The CALL instruction jumps to an address after storing the
  // address of the next sequential instruction on the stack.
  // The address jumped to is relative to the current CIP,
  // but the address on the stack is an absolute address.
  cell dest = rel_code_addr(as.amx(), instr.operand());
  as.call(as.getAmxLabel(dest));
}

static void emit_jump_pri(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // CIP = PRI (indirect jump)
  const NamedLabel &L_jump_helper = as.getLabel("jump_helper");
  as.push(esp);
  as.push(ebp);
  as.push(eax);
  as.call(L_jump_helper);
}

static const Label &get_jump_label(Assembler &as,
                                   const jit::AMXInstruction &instr) {
  cell dest = rel_code_addr(as.amx(), instr.operand());
  return as.getAmxLabel(dest);
}

static void emit_jump(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // CIP = CIP + offset (jump to the address relative from
  // the current position)
  as.jmp(get_jump_label(as, instr));
}

static void emit_jzer(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // if PRI == 0 then CIP = CIP + offset
  as.cmp(eax, 0);
  as.jz(get_jump_label(as, instr));
}

static void emit_jnz(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // if PRI != 0 then CIP = CIP + offset
  as.cmp(eax, 0);
  as.jnz(get_jump_label(as, instr));
}

static void emit_jeq(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // if PRI == ALT then CIP = CIP + offset
  as.cmp(eax, ecx);
  as.je(get_jump_label(as, instr));
}

static void emit_jneq(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // if PRI != ALT then CIP = CIP + offset
  as.cmp(eax, ecx);
  as.jne(get_jump_label(as, instr));
}

static void emit_jless(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // if PRI < ALT then CIP = CIP + offset (unsigned)
  as.cmp(eax, ecx);
  as.jb(get_jump_label(as, instr));
}

static void emit_jleq(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // if PRI <= ALT then CIP = CIP + offset (unsigned)
  as.cmp(eax, ecx);
  as.jbe(get_jump_label(as, instr));
}

static void emit_jgrtr(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // if PRI > ALT then CIP = CIP + offset (unsigned)
  as.cmp(eax, ecx);
  as.ja(get_jump_label(as, instr));
}

static void emit_jgeq(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // if PRI >= ALT then CIP = CIP + offset (unsigned)
  as.cmp(eax, ecx);
  as.jae(get_jump_label(as, instr));
}

static void emit_jsless(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
  // if PRI < ALT then CIP = CIP + offset (signed)
  as.cmp(eax, ecx);
  as.jl(get_jump_label(as, instr));
}

static void emit_jsleq(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // if PRI <= ALT then CIP = CIP + offset (signed)
  as.cmp(eax, ecx);
  as.jle(get_jump_label(as, instr));
}

static void emit_jsgrtr(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
  // if PRI > ALT then CIP = CIP + offset (signed)
  as.cmp(eax, ecx);
  as.jg(get_jump_label(as, instr));
}

static void emit_jsgeq(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // if PRI >= ALT then CIP = CIP + offset (signed)
  as.cmp(eax, ecx);
  as.jge(get_jump_label(as, instr));
}

static void emit_shl(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // PRI = PRI << ALT
  as.shl(eax, cl);
}

static void emit_shr(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // PRI = PRI >> ALT (without sign extension)
  as.shr(eax, cl);
}

static void emit_sshr(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // PRI = PRI >> ALT with sign extension
  as.sar(eax, cl);
}

static void emit_shl_c_pri(Assembler &as, const jit::AMXInstruction &instr,
                           bool *error) {
  // PRI = PRI << value
  as.shl(eax, static_cast<unsigned char>(instr.operand()));
}

static void emit_shl_c_alt(Assembler &as, const jit::AMXInstruction &instr,
                           bool *error) {
  // ALT = ALT << value
  as.shl(ecx, static_cast<unsigned char>(instr.operand()));
}

static void emit_shr_c_pri(Assembler &as, const jit::AMXInstruction &instr,
                           bool *error) {
  // PRI = PRI >> value (without sign extension)
  as.shr(eax, static_cast<unsigned char>(instr.operand()));
}

static void emit_shr_c_alt(Assembler &as, const jit::AMXInstruction &instr,
                           bool *error) {
  // PRI = PRI >> value (without sign extension)
  as.shl(ecx, static_cast<unsigned char>(instr.operand()));
}

static void emit_smul(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // PRI = PRI * ALT (signed multiply)
  as.xor_(edx, edx);
  as.imul(ecx);
}

static void emit_sdiv(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // PRI = PRI / ALT (signed divide), ALT = PRI mod ALT
  as.xor_(edx, edx);
  as.idiv(ecx);
  as.mov(ecx, edx);
}

static void emit_sdiv_alt(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // PRI = ALT / PRI (signed divide), ALT = ALT mod PRI
  as.xchg(eax, ecx);
  as.xor_(edx, edx);
  as.idiv(ecx);
  as.mov(ecx, edx);
}

static void emit_umul(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // PRI = PRI * ALT (unsigned multiply)
  as.xor_(edx, edx);
  as.mul(ecx);
}

static void emit_udiv(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // PRI = PRI / ALT (unsigned divide), ALT = PRI mod ALT
  as.xor_(edx, edx);
  as.div(ecx);
  as.mov(ecx, edx);
}

static void emit_udiv_alt(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // PRI = ALT / PRI (unsigned divide), ALT = ALT mod PRI
  as.xchg(eax, ecx);
  as.xor_(edx, edx);
  as.div(ecx);
  as.mov(ecx, edx);
}

static void emit_add(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // PRI = PRI + ALT
  as.add(eax, ecx);
}

static void emit_sub(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // PRI = PRI - ALT
  as.sub(eax, ecx);
}

static void emit_sub_alt(Assembler &as, const jit::AMXInstruction &instr,
                         bool *error) {
  // PRI = ALT - PRI
  // or:
  // PRI = -(PRI - ALT)
  as.sub(eax, ecx);
  as.neg(eax);
}

static void emit_and(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // PRI = PRI & ALT
  as.and_(eax, ecx);
}

static void emit_or(Assembler &as, const jit::AMXInstruction &instr,
                    bool *error) {
  // PRI = PRI | ALT
  as.or_(eax, ecx);
}

static void emit_xor(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // PRI = PRI ^ ALT
  as.xor_(eax, ecx);
}

static void emit_not(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // PRI = !PRI
  as.test(eax, eax);
  as.setz(al);
  as.movzx(eax, al);
}

static void emit_neg(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // PRI = -PRI
  as.neg(eax);
}

static void emit_invert(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
  // PRI = ~PRI
  as.not_(eax);
}

static void emit_add_c(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // PRI = PRI + value
  if (instr.operand() >= 0) {
    as.add(eax, instr.operand());
  } else {
    as.sub(eax, -instr.operand());
  }
}

static void emit_smul_c(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
  // PRI = PRI * value
  as.imul(eax, instr.operand());
}

static void emit_zero_pri(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // PRI = 0
  as.xor_(eax, eax);
}

static void emit_zero_alt(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // ALT = 0
  as.xor_(ecx, ecx);
}

static void emit_zero(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // [address] = 0
  as.mov(dword_ptr(ebx, instr.operand()), 0);
}

static void emit_zero_s(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
  // [FRM + offset] = 0
  as.mov(dword_ptr(ebp, instr.operand()), 0);
}

static void emit_sign_pri(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // sign extent the byte in PRI to a cell
  as.movsx(eax, al);
}

static void emit_sign_alt(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // sign extent the byte in ALT to a cell
  as.movsx(ecx, cl);
}

static void emit_eq(Assembler &as, const jit::AMXInstruction &instr,
                    bool *error) {
  // PRI = PRI == ALT ? 1 :
  as.cmp(eax, ecx);
  as.sete(al);
  as.movzx(eax, al);
}

static void emit_neq(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // PRI = PRI != ALT ? 1 :
  as.cmp(eax, ecx);
  as.setne(al);
  as.movzx(eax, al);
}

static void emit_less(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // PRI = PRI < ALT ? 1 :
  as.cmp(eax, ecx);
  as.setb(al);
  as.movzx(eax, al);
}

static void emit_leq(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // PRI = PRI <= ALT ? 1 :
  as.cmp(eax, ecx);
  as.setbe(al);
  as.movzx(eax, al);
}

static void emit_grtr(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // PRI = PRI > ALT ? 1 :
  as.cmp(eax, ecx);
  as.seta(al);
  as.movzx(eax, al);
}

static void emit_geq(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // PRI = PRI >= ALT ? 1 :
  as.cmp(eax, ecx);
  as.setae(al);
  as.movzx(eax, al);
}

static void emit_sless(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // PRI = PRI < ALT ? 1 :
  as.cmp(eax, ecx);
  as.setl(al);
  as.movzx(eax, al);
}

static void emit_sleq(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // PRI = PRI <= ALT ? 1 :
  as.cmp(eax, ecx);
  as.setle(al);
  as.movzx(eax, al);
}

static void emit_sgrtr(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // PRI = PRI > ALT ? 1 :
  as.cmp(eax, ecx);
  as.setg(al);
  as.movzx(eax, al);
}

static void emit_sgeq(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // PRI = PRI >= ALT ? 1 :
  as.cmp(eax, ecx);
  as.setge(al);
  as.movzx(eax, al);
}

static void emit_eq_c_pri(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // PRI = PRI == value ? 1 :
  as.cmp(eax, instr.operand());
  as.sete(al);
  as.movzx(eax, al);
}

static void emit_eq_c_alt(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // PRI = ALT == value ? 1 :
  as.cmp(ecx, instr.operand());
  as.sete(al);
  as.movzx(eax, al);
}

static void emit_inc_pri(Assembler &as, const jit::AMXInstruction &instr,
                         bool *error) {
  // PRI = PRI + 1
  as.inc(eax);
}

static void emit_inc_alt(Assembler &as, const jit::AMXInstruction &instr,
                         bool *error) {
  // ALT = ALT + 1
  as.inc(ecx);
}

static void emit_inc(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // [address] = [address] + 1
  as.inc(dword_ptr(ebx, instr.operand()));
}

static void emit_inc_s(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // [FRM + offset] = [FRM + offset] + 1
  as.inc(dword_ptr(ebp, instr.operand()));
}

static void emit_inc_i(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // [PRI] = [PRI] + 1
  as.inc(dword_ptr(ebx, eax));
}

static void emit_dec_pri(Assembler &as, const jit::AMXInstruction &instr,
                         bool *error) {
  // PRI = PRI - 1
  as.dec(eax);
}

static void emit_dec_alt(Assembler &as, const jit::AMXInstruction &instr,
                         bool *error) {
  // ALT = ALT - 1
  as.dec(ecx);
}

static void emit_dec(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // [address] = [address] - 1
  as.dec(dword_ptr(ebx, instr.operand()));
}

static void emit_dec_s(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // [FRM + offset] = [FRM + offset] - 1
  as.dec(dword_ptr(ebp, instr.operand()));
}

static void emit_dec_i(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // [PRI] = [PRI] - 1
  as.dec(dword_ptr(ebx, eax));
}

static void emit_movs(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
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

static void emit_cmps(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
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

static void emit_fill(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
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

static void emit_halt(Assembler &as, const jit::AMXInstruction &instr,
                      bool *error) {
  // Abort execution (exit value in PRI), parameters other than 0
  // have a special meaning.
  const NamedLabel &L_do_halt = as.getLabel("do_halt");
  as.mov(ecx, instr.operand());
  as.jmp(L_do_halt);
}

static void emit_bounds(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
  // Abort execution if PRI > value or if PRI < 0.
  const NamedLabel &L_do_halt = as.getLabel("do_halt");
  Label L_halt = as.newLabel();
  Label L_good = as.newLabel();
    as.cmp(eax, instr.operand());
  as.jg(L_halt);
    as.cmp(eax, 0);
    as.jl(L_halt);
    as.jmp(L_good);
  as.bind(L_halt);
    as.mov(ecx, AMX_ERR_BOUNDS);
    as.jmp(L_do_halt);
  as.bind(L_good);
}

static void emit_sysreq_pri(Assembler &as, const jit::AMXInstruction &instr,
                            bool *error) {
  // call system service, service number in PRI
  const NamedLabel &L_sysreq_c_helper = as.getLabel("sysreq_c_helper");
  as.push(esp); // stack_ptr
  as.push(ebp); // stack_base
  as.push(eax); // index
  as.call(L_sysreq_c_helper);
}

static void emit_sysreq_c(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // call system service
  const NamedLabel &L_sysreq_c_helper = as.getLabel("sysreq_c_helper");
  const char *name = as.amx().get_native_name(instr.operand());
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

static void emit_sysreq_d(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // call system service
  const NamedLabel &L_sysreq_d_helper = as.getLabel("sysreq_d_helper");
  cell index = as.amx().find_native(instr.operand());
  const char *name = as.amx().get_native_name(index);
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

static void emit_switch(Assembler &as, const jit::AMXInstruction &instr,
                        bool *error) {
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
  cell default_case = rel_code_addr(as.amx(), case_table[0].address);

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
      cell dest = rel_code_addr(as.amx(), case_table[i + 1].address);
      as.cmp(eax, case_table[i + 1].value);
      as.je(as.getAmxLabel(dest));
    }
  }

  // No match found - go for default case.
  as.jmp(as.getAmxLabel(default_case));
}

static void emit_casetbl(Assembler &as, const jit::AMXInstruction &instr,
                         bool *error) {
  // A variable number of case records follows this opcode, where
  // each record takes two cells.
}

static void emit_swap_pri(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // [STK] = PRI and PRI = [STK]
  as.xchg(dword_ptr(esp), eax);
}

static void emit_swap_alt(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // [STK] = ALT and ALT = [STK]
  as.xchg(dword_ptr(esp), ecx);
}

static void emit_push_adr(Assembler &as, const jit::AMXInstruction &instr,
                          bool *error) {
  // [STK] = FRM + offset, STK = STK - cell size
  as.lea(edx, dword_ptr(ebp, instr.operand()));
  as.sub(edx, ebx);
  as.push(edx);
}

static void emit_nop(Assembler &as, const jit::AMXInstruction &instr,
                     bool *error) {
  // no-operation, for code alignment
}

static void emit_break(Assembler &as, const jit::AMXInstruction &instr,
                       bool *error) {
  // conditional breakpoint
}

static EmitInstruction emit_opcode[] = {
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

namespace jit {

AsmjitBackendOutput::AsmjitBackendOutput(void *code, std::size_t code_size)
  : code_(code), code_size_(code_size)
{
}

AsmjitBackendOutput::~AsmjitBackendOutput() {
  AsmJit::MemoryManager::getGlobal()->free(code_);
}

AsmjitBackend::AsmjitBackend() {}
AsmjitBackend::~AsmjitBackend() {}

BackendOutput *AsmjitBackend::compile(AMXPtr amx,
                                      CompileErrorHandler *error_handler)
{ 
  Assembler as(amx);

  emit_runtime_data(as);
  emit_instr_map(as);
  emit_exec(as);
  emit_exec_helper(as);
  emit_halt_helper(as);
  emit_jump_helper(as);
  emit_sysreq_c_helper(as);
  emit_sysreq_d_helper(as);

  std::set<cell> jump_targets;
  collect_jump_targets(amx, jump_targets);

  AMXDisassembler disas(amx);
  AMXInstruction instr;
  bool error = false;

  typedef std::vector<std::pair<cell, int> > InstrMap;
  InstrMap instr_map;

  while (!error && disas.decode(instr, &error)) {
    cell cip = instr.address();
    
    if (instr.opcode().id() == OP_PROC) {
      as.align(16);
    }

    if (jump_targets.find(cip) != jump_targets.end()) {
      as.bind(as.getAmxLabel(cip));
    }

    // Add this instruction to the opcode map.
    instr_map.push_back(std::make_pair(instr.address(), as.getCodeSize()));

    AMXOpcodeID opcode = instr.opcode().id();
    if (opcode >= 0 &&
        opcode < sizeof(emit_opcode) / sizeof(emit_opcode[0])) {
      emit_opcode[opcode](as, instr, &error);
    } else {
      error = true;
    }
  }

  as.bindByName("do_halt");
    as.push(ecx);
    as.call(as.getLabel("halt_helper"));

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

  return new AsmjitBackendOutput(reinterpret_cast<void*>(code_ptr), code_size);
}

} // namespace jit
