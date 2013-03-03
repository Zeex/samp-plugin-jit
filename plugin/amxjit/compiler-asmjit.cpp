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
#include "amxdisasm.h"
#include "compiler-asmjit.h"

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

namespace {

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

cell AMXJIT_CDECL get_public_addr(AMX *amx, int index) {
  return amxjit::AMXPtr(amx).get_public_addr(index);
}

cell AMXJIT_CDECL get_native_addr(AMX *amx, int index) {
  return amxjit::AMXPtr(amx).get_native_addr(index);
}

void *AMXJIT_CDECL get_instr_ptr(cell address, void *instr_map,
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

} // anonymous namespace

namespace amxjit {

CompilerOutputAsmjit::CompilerOutputAsmjit(void *code, std::size_t code_size)
  : code_(code), code_size_(code_size)
{
}

CompilerOutputAsmjit::~CompilerOutputAsmjit() {
  AsmJit::MemoryManager::getGlobal()->free(code_);
}

CompilerAsmjit::CompilerAsmjit()
  : as_(),
    L_exec_ptr_(as_.newLabel()),
    L_amx_ptr_(as_.newLabel()),
    L_ebp_ptr_(as_.newLabel()),
    L_esp_ptr_(as_.newLabel()),
    L_reset_ebp_ptr_(as_.newLabel()),
    L_reset_esp_ptr_(as_.newLabel()),
    L_instr_map_ptr_(as_.newLabel()),
    L_instr_map_size_(as_.newLabel()),
    L_exec_(as_.newLabel()),
    L_exec_helper_(as_.newLabel()),
    L_halt_helper_(as_.newLabel()),
    L_jump_helper_(as_.newLabel()),
    L_sysreq_c_helper_(as_.newLabel()),
    L_sysreq_d_helper_(as_.newLabel()),
    L_break_helper_(as_.newLabel())
{
}

CompilerAsmjit::~CompilerAsmjit()
{
}

bool CompilerAsmjit::setup(AMXPtr amx) { 
  emit_runtime_data(amx);
  emit_instr_map(amx);
  emit_exec();
  emit_exec_helper();
  emit_halt_helper();
  emit_jump_helper();
  emit_sysreq_c_helper();
  emit_sysreq_d_helper();
  emit_break_helper();
  return true;
}

bool CompilerAsmjit::process(const AMXInstruction &instr) {
  cell cip = instr.address();

  as_.bind(amx_label(cip));
  instr_map_.push_back(std::make_pair(cip, as_.getCodeSize()));

  // Align functions on 16-byte boundary.
  if (instr.opcode().id() == AMX_OP_PROC) {
    as_.align(16);
  }

  return true;
}

void CompilerAsmjit::abort() {
  // do nothing
}

CompilerOutput *CompilerAsmjit::finish() {
  intptr_t code_ptr = reinterpret_cast<intptr_t>(as_.make());
  size_t code_size = as_.getCodeSize();

  intptr_t *runtime_data = reinterpret_cast<intptr_t*>(code_ptr);

  runtime_data[RuntimeDataExecPtr] += code_ptr;
  runtime_data[RuntimeDataInstrMapPtr] += code_ptr;

  intptr_t instr_map_ptr = runtime_data[RuntimeDataInstrMapPtr];
  InstrMapEntry *entry = reinterpret_cast<InstrMapEntry*>(instr_map_ptr);

  for (std::size_t i = 0; i < instr_map_.size(); i++) {
    entry->amx_addr = instr_map_[i].first;
    entry->jit_addr = reinterpret_cast<void*>(instr_map_[i].second + code_ptr);
    entry++;
  }

  return new CompilerOutputAsmjit(reinterpret_cast<void*>(code_ptr), code_size);
}

void CompilerAsmjit::emit_load_pri(cell address) {
  // PRI = [address]
  as_.mov(eax, dword_ptr(ebx, address));
}

void CompilerAsmjit::emit_load_alt(cell address) {
  // ALT = [address]
  as_.mov(ecx, dword_ptr(ebx, address));
}

void CompilerAsmjit::emit_load_s_pri(cell offset) {
  // PRI = [FRM + offset]
  as_.mov(eax, dword_ptr(ebp, offset));
}

void CompilerAsmjit::emit_load_s_alt(cell offset) {
  // ALT = [FRM + offset]
  as_.mov(ecx, dword_ptr(ebp, offset));
}

void CompilerAsmjit::emit_lref_pri(cell address) {
  // PRI = [ [address] ]
  as_.mov(edx, dword_ptr(ebx, address));
  as_.mov(eax, dword_ptr(ebx, edx));
}

void CompilerAsmjit::emit_lref_alt(cell address) {
  // ALT = [ [address] ]
  as_.mov(edx, dword_ptr(ebx, + address));
  as_.mov(ecx, dword_ptr(ebx, edx));
}

void CompilerAsmjit::emit_lref_s_pri(cell offset) {
  // PRI = [ [FRM + offset] ]
  as_.mov(edx, dword_ptr(ebp, offset));
  as_.mov(eax, dword_ptr(ebx, edx));
}

void CompilerAsmjit::emit_lref_s_alt(cell offset) {
  // PRI = [ [FRM + offset] ]
  as_.mov(edx, dword_ptr(ebp, offset));
  as_.mov(ecx, dword_ptr(ebx, edx));
}

void CompilerAsmjit::emit_load_i() {
  // PRI = [PRI] (full cell)
  as_.mov(eax, dword_ptr(ebx, eax));
}

void CompilerAsmjit::emit_lodb_i(cell number) {
  // PRI = "number" bytes from [PRI] (read 1/2/4 bytes)
  switch (number) {
    case 1:
      as_.movzx(eax, byte_ptr(ebx, eax));
      break;
    case 2:
      as_.movzx(eax, word_ptr(ebx, eax));
      break;
    case 4:
      as_.mov(eax, dword_ptr(ebx, eax));
      break;
  }
}

void CompilerAsmjit::emit_const_pri(cell value) {
  // PRI = value
  if (value == 0) {
    as_.xor_(eax, eax);
  } else {
    as_.mov(eax, value);
  }
}

void CompilerAsmjit::emit_const_alt(cell value) {
  // ALT = value
  if (value == 0) {
    as_.xor_(ecx, ecx);
  } else {
    as_.mov(ecx, value);
  }
}

void CompilerAsmjit::emit_addr_pri(cell offset) {
  // PRI = FRM + offset
  as_.lea(eax, dword_ptr(ebp, offset));
  as_.sub(eax, ebx);
}

void CompilerAsmjit::emit_addr_alt(cell offset) {
  // ALT = FRM + offset
  as_.lea(ecx, dword_ptr(ebp, offset));
  as_.sub(ecx, ebx);
}

void CompilerAsmjit::emit_stor_pri(cell address) {
  // [address] = PRI
  as_.mov(dword_ptr(ebx, address), eax);
}

void CompilerAsmjit::emit_stor_alt(cell address) {
  // [address] = ALT
  as_.mov(dword_ptr(ebx, address), ecx);
}

void CompilerAsmjit::emit_stor_s_pri(cell offset) {
  // [FRM + offset] = ALT
  as_.mov(dword_ptr(ebp, offset), eax);
}

void CompilerAsmjit::emit_stor_s_alt(cell offset) {
  // [FRM + offset] = ALT
  as_.mov(dword_ptr(ebp, offset), ecx);
}

void CompilerAsmjit::emit_sref_pri(cell address) {
  // [ [address] ] = PRI
  as_.mov(edx, dword_ptr(ebx, address));
  as_.mov(dword_ptr(ebx, edx), eax);
}

void CompilerAsmjit::emit_sref_alt(cell address) {
  // [ [address] ] = ALT
  as_.mov(edx, dword_ptr(ebx, address));
  as_.mov(dword_ptr(ebx, edx), ecx);
}

void CompilerAsmjit::emit_sref_s_pri(cell offset) {
  // [ [FRM + offset] ] = PRI
  as_.mov(edx, dword_ptr(ebp, offset));
  as_.mov(dword_ptr(ebx, edx), eax);
}

void CompilerAsmjit::emit_sref_s_alt(cell offset) {
  // [ [FRM + offset] ] = ALT
  as_.mov(edx, dword_ptr(ebp, offset));
  as_.mov(dword_ptr(ebx, edx), ecx);
}

void CompilerAsmjit::emit_stor_i() {
  // [ALT] = PRI (full cell)
  as_.mov(dword_ptr(ebx, ecx), eax);
}

void CompilerAsmjit::emit_strb_i(cell number) {
  // "number" bytes at [ALT] = PRI (write 1/2/4 bytes)
  switch (number) {
    case 1:
      as_.mov(byte_ptr(ebx, ecx), al);
      break;
    case 2:
      as_.mov(word_ptr(ebx, ecx), ax);
      break;
    case 4:
      as_.mov(dword_ptr(ebx, ecx), eax);
      break;
  }
}

void CompilerAsmjit::emit_lidx() {
  // PRI = [ ALT + (PRI x cell size) ]
  as_.lea(edx, dword_ptr(ebx, ecx));
  as_.mov(eax, dword_ptr(edx, eax, 2));
}

void CompilerAsmjit::emit_lidx_b(cell shift) {
  // PRI = [ ALT + (PRI << shift) ]
  as_.lea(edx, dword_ptr(ebx, ecx));
  as_.mov(eax, dword_ptr(edx, eax, shift));
}

void CompilerAsmjit::emit_idxaddr() {
  // PRI = ALT + (PRI x cell size) (calculate indexed address)
  as_.lea(eax, dword_ptr(ecx, eax, 2));
}

void CompilerAsmjit::emit_idxaddr_b(cell shift) {
  // PRI = ALT + (PRI << shift) (calculate indexed address)
  as_.lea(eax, dword_ptr(ecx, eax, shift));
}

void CompilerAsmjit::emit_align_pri(cell number) {
  // Little Endian: PRI ^= cell size - number
  #if BYTE_ORDER == LITTLE_ENDIAN
    if (static_cast<std::size_t>(number) < sizeof(cell)) {
      as_.xor_(eax, sizeof(cell) - number);
    }
  #endif
}

void CompilerAsmjit::emit_align_alt(cell number) {
  // Little Endian: ALT ^= cell size - number
  #if BYTE_ORDER == LITTLE_ENDIAN
    if (static_cast<std::size_t>(number) < sizeof(cell)) {
      as_.xor_(ecx, sizeof(cell) - number);
    }
  #endif
}

void CompilerAsmjit::emit_lctrl(cell index) {
  // PRI is set to the current value of any of the special registers.
  // The index parameter must be:
  // 3=STP, 4=STK, 5=FRM, 6=CIP (of the next instruction)
  switch (index) {
    case 0:
    case 1:
    case 2:
    case 3:
      emit_get_amx_ptr(eax);
      switch (index) {
        case 0:
          as_.mov(eax, dword_ptr(eax, offsetof(AMX, base)));
          as_.mov(eax, dword_ptr(edx, offsetof(AMX_HEADER, cod)));
          break;
        case 1:
          as_.mov(eax, dword_ptr(eax, offsetof(AMX, base)));
          as_.mov(eax, dword_ptr(edx, offsetof(AMX_HEADER, dat)));
          break;
        case 2:
          as_.mov(eax, dword_ptr(eax, offsetof(AMX, hea)));
          break;
        case 3:
          as_.mov(eax, dword_ptr(eax, offsetof(AMX, stp)));
          break;
      }
      break;
    case 4:
      as_.mov(eax, esp);
      as_.sub(eax, ebx);
      break;
    case 5:
      as_.mov(eax, ebp);
      as_.sub(eax, ebx);
      break;
    case 6:
      as_.mov(eax, get_instr()->address() + get_instr()->size());
      break;
    case 7:
      as_.mov(eax, 1);
      break;
  }
}

void CompilerAsmjit::emit_sctrl(cell index) {
  // set the indexed special registers to the value in PRI.
  // The index parameter must be:
  // 6=CIP
  switch (index) {
    case 2:
      emit_get_amx_ptr(edx);
      as_.mov(dword_ptr(edx, offsetof(AMX, hea)), eax);
      break;
    case 4:
      as_.lea(esp, dword_ptr(ebx, eax));
      break;
    case 5:
      as_.lea(ebp, dword_ptr(ebx, eax));
      break;
    case 6:
      as_.mov(edi, esp);
      as_.mov(esi, ebp);
      as_.mov(edx, eax);
      as_.call(L_jump_helper_);
      break;
  }
}

void CompilerAsmjit::emit_move_pri() {
  // PRI = ALT
  as_.mov(eax, ecx);
}

void CompilerAsmjit::emit_move_alt() {
  // ALT = PRI
  as_.mov(ecx, eax);
}

void CompilerAsmjit::emit_xchg() {
  // Exchange PRI and ALT
  as_.xchg(eax, ecx);
}

void CompilerAsmjit::emit_push_pri() {
  // [STK] = PRI, STK = STK - cell size
  as_.push(eax);
}

void CompilerAsmjit::emit_push_alt() {
  // [STK] = ALT, STK = STK - cell size
  as_.push(ecx);
}

void CompilerAsmjit::emit_push_c(cell value) {
  // [STK] = value, STK = STK - cell size
  as_.push(value);
}

void CompilerAsmjit::emit_push(cell address) {
  // [STK] = [address], STK = STK - cell size
  as_.push(dword_ptr(ebx, address));
}

void CompilerAsmjit::emit_push_s(cell offset) {
  // [STK] = [FRM + offset], STK = STK - cell size
  as_.push(dword_ptr(ebp, offset));
}

void CompilerAsmjit::emit_pop_pri() {
  // STK = STK + cell size, PRI = [STK]
  as_.pop(eax);
}

void CompilerAsmjit::emit_pop_alt() {
  // STK = STK + cell size, ALT = [STK]
  as_.pop(ecx);
}

void CompilerAsmjit::emit_stack(cell value) {
  // ALT = STK, STK = STK + value
  as_.mov(ecx, esp);
  as_.sub(ecx, ebx);
  if (value >= 0) {
    as_.add(esp, value);
  } else {
    as_.sub(esp, -value);
  }
}

void CompilerAsmjit::emit_heap(cell value) {
  // ALT = HEA, HEA = HEA + value
  emit_get_amx_ptr(edx);
  as_.mov(ecx, dword_ptr(edx, offsetof(AMX, hea)));
  if (value >= 0) {
    as_.add(dword_ptr(edx, offsetof(AMX, hea)), value);
  } else {
    as_.sub(dword_ptr(edx, offsetof(AMX, hea)), -value);
  }
}

void CompilerAsmjit::emit_proc() {
  // [STK] = FRM, STK = STK - cell size, FRM = STK
  as_.push(ebp);
  as_.mov(ebp, esp);
  as_.sub(dword_ptr(esp), ebx);
}

void CompilerAsmjit::emit_ret() {
  // STK = STK + cell size, FRM = [STK],
  // CIP = [STK], STK = STK + cell size
  as_.pop(ebp);
  as_.add(ebp, ebx);
  as_.ret();
}

void CompilerAsmjit::emit_retn() {
  // STK = STK + cell size, FRM = [STK],
  // CIP = [STK], STK = STK + cell size
  // The RETN instruction removes a specified number of bytes
  // from the stack. The value to adjust STK with must be
  // pushed prior to the call.
  as_.pop(ebp);
  as_.add(ebp, ebx);
  as_.pop(edx);
  as_.add(esp, dword_ptr(esp));
  as_.push(edx);
  as_.ret(4);
}

void CompilerAsmjit::emit_call(cell address) {
  // [STK] = CIP + 5, STK = STK - cell size
  // CIP = CIP + offset
  // The CALL instruction jumps to an address after storing the
  // address of the next sequential instruction on the stack.
  // The address jumped to is relative to the current CIP,
  // but the address on the stack is an absolute address.
  as_.call(amx_label(address));
}

void CompilerAsmjit::emit_jump_pri() {
  // CIP = PRI (indirect jump)
  as_.mov(edi, esp);
  as_.mov(esi, ebp);
  as_.mov(edx, eax);
  as_.call(L_jump_helper_);
}

void CompilerAsmjit::emit_jump(cell address) {
  // CIP = CIP + offset (jump to the address relative from
  // the current position)
  as_.jmp(amx_label(address));
}

void CompilerAsmjit::emit_jzer(cell address) {
  // if PRI == 0 then CIP = CIP + offset
  as_.test(eax, eax);
  as_.jz(amx_label(address));
}

void CompilerAsmjit::emit_jnz(cell address) {
  // if PRI != 0 then CIP = CIP + offset
  as_.test(eax, eax);
  as_.jnz(amx_label(address));
}

void CompilerAsmjit::emit_jeq(cell address) {
  // if PRI == ALT then CIP = CIP + offset
  as_.cmp(eax, ecx);
  as_.je(amx_label(address));
}

void CompilerAsmjit::emit_jneq(cell address) {
  // if PRI != ALT then CIP = CIP + offset
  as_.cmp(eax, ecx);
  as_.jne(amx_label(address));
}

void CompilerAsmjit::emit_jless(cell address) {
  // if PRI < ALT then CIP = CIP + offset (unsigned)
  as_.cmp(eax, ecx);
  as_.jb(amx_label(address));
}

void CompilerAsmjit::emit_jleq(cell address) {
  // if PRI <= ALT then CIP = CIP + offset (unsigned)
  as_.cmp(eax, ecx);
  as_.jbe(amx_label(address));
}

void CompilerAsmjit::emit_jgrtr(cell address) {
  // if PRI > ALT then CIP = CIP + offset (unsigned)
  as_.cmp(eax, ecx);
  as_.ja(amx_label(address));
}

void CompilerAsmjit::emit_jgeq(cell address) {
  // if PRI >= ALT then CIP = CIP + offset (unsigned)
  as_.cmp(eax, ecx);
  as_.jae(amx_label(address));
}

void CompilerAsmjit::emit_jsless(cell address) {
  // if PRI < ALT then CIP = CIP + offset (signed)
  as_.cmp(eax, ecx);
  as_.jl(amx_label(address));
}

void CompilerAsmjit::emit_jsleq(cell address) {
  // if PRI <= ALT then CIP = CIP + offset (signed)
  as_.cmp(eax, ecx);
  as_.jle(amx_label(address));
}

void CompilerAsmjit::emit_jsgrtr(cell address) {
  // if PRI > ALT then CIP = CIP + offset (signed)
  as_.cmp(eax, ecx);
  as_.jg(amx_label(address));
}

void CompilerAsmjit::emit_jsgeq(cell address) {
  // if PRI >= ALT then CIP = CIP + offset (signed)
  as_.cmp(eax, ecx);
  as_.jge(amx_label(address));
}

void CompilerAsmjit::emit_shl() {
  // PRI = PRI << ALT
  as_.shl(eax, cl);
}

void CompilerAsmjit::emit_shr() {
  // PRI = PRI >> ALT (without sign extension)
  as_.shr(eax, cl);
}

void CompilerAsmjit::emit_sshr() {
  // PRI = PRI >> ALT with sign extension
  as_.sar(eax, cl);
}

void CompilerAsmjit::emit_shl_c_pri(cell value) {
  // PRI = PRI << value
  as_.shl(eax, static_cast<unsigned char>(value));
}

void CompilerAsmjit::emit_shl_c_alt(cell value) {
  // ALT = ALT << value
  as_.shl(ecx, static_cast<unsigned char>(value));
}

void CompilerAsmjit::emit_shr_c_pri(cell value) {
  // PRI = PRI >> value (without sign extension)
  as_.shr(eax, static_cast<unsigned char>(value));
}

void CompilerAsmjit::emit_shr_c_alt(cell value) {
  // PRI = PRI >> value (without sign extension)
  as_.shl(ecx, static_cast<unsigned char>(value));
}

void CompilerAsmjit::emit_smul() {
  // PRI = PRI * ALT (signed multiply)
  as_.xor_(edx, edx);
  as_.imul(ecx);
}

void CompilerAsmjit::emit_sdiv() {
  // PRI = PRI / ALT (signed divide), ALT = PRI mod ALT
  as_.xor_(edx, edx);
  as_.idiv(ecx);
  as_.mov(ecx, edx);
}

void CompilerAsmjit::emit_sdiv_alt() {
  // PRI = ALT / PRI (signed divide), ALT = ALT mod PRI
  as_.xchg(eax, ecx);
  as_.xor_(edx, edx);
  as_.idiv(ecx);
  as_.mov(ecx, edx);
}

void CompilerAsmjit::emit_umul() {
  // PRI = PRI * ALT (unsigned multiply)
  as_.xor_(edx, edx);
  as_.mul(ecx);
}

void CompilerAsmjit::emit_udiv() {
  // PRI = PRI / ALT (unsigned divide), ALT = PRI mod ALT
  as_.xor_(edx, edx);
  as_.div(ecx);
  as_.mov(ecx, edx);
}

void CompilerAsmjit::emit_udiv_alt() {
  // PRI = ALT / PRI (unsigned divide), ALT = ALT mod PRI
  as_.xchg(eax, ecx);
  as_.xor_(edx, edx);
  as_.div(ecx);
  as_.mov(ecx, edx);
}

void CompilerAsmjit::emit_add() {
  // PRI = PRI + ALT
  as_.add(eax, ecx);
}

void CompilerAsmjit::emit_sub() {
  // PRI = PRI - ALT
  as_.sub(eax, ecx);
}

void CompilerAsmjit::emit_sub_alt() {
  // PRI = ALT - PRI
  // or:
  // PRI = -(PRI - ALT)
  as_.sub(eax, ecx);
  as_.neg(eax);
}

void CompilerAsmjit::emit_and() {
  // PRI = PRI & ALT
  as_.and_(eax, ecx);
}

void CompilerAsmjit::emit_or() {
  // PRI = PRI | ALT
  as_.or_(eax, ecx);
}

void CompilerAsmjit::emit_xor() {
  // PRI = PRI ^ ALT
  as_.xor_(eax, ecx);
}

void CompilerAsmjit::emit_not() {
  // PRI = !PRI
  as_.test(eax, eax);
  as_.setz(al);
  as_.movzx(eax, al);
}

void CompilerAsmjit::emit_neg() {
  // PRI = -PRI
  as_.neg(eax);
}

void CompilerAsmjit::emit_invert() {
  // PRI = ~PRI
  as_.not_(eax);
}

void CompilerAsmjit::emit_add_c(cell value) {
  // PRI = PRI + value
  if (value >= 0) {
    as_.add(eax, value);
  } else {
    as_.sub(eax, -value);
  }
}

void CompilerAsmjit::emit_smul_c(cell value) {
  // PRI = PRI * value
  as_.imul(eax, value);
}

void CompilerAsmjit::emit_zero_pri() {
  // PRI = 0
  as_.xor_(eax, eax);
}

void CompilerAsmjit::emit_zero_alt() {
  // ALT = 0
  as_.xor_(ecx, ecx);
}

void CompilerAsmjit::emit_zero(cell address) {
  // [address] = 0
  as_.mov(dword_ptr(ebx, address), 0);
}

void CompilerAsmjit::emit_zero_s(cell offset) {
  // [FRM + offset] = 0
  as_.mov(dword_ptr(ebp, offset), 0);
}

void CompilerAsmjit::emit_sign_pri() {
  // sign extent the byte in PRI to a cell
  as_.movsx(eax, al);
}

void CompilerAsmjit::emit_sign_alt() {
  // sign extent the byte in ALT to a cell
  as_.movsx(ecx, cl);
}

void CompilerAsmjit::emit_eq() {
  // PRI = PRI == ALT ? 1 :
  as_.cmp(eax, ecx);
  as_.sete(al);
  as_.movzx(eax, al);
}

void CompilerAsmjit::emit_neq() {
  // PRI = PRI != ALT ? 1 :
  as_.cmp(eax, ecx);
  as_.setne(al);
  as_.movzx(eax, al);
}

void CompilerAsmjit::emit_less() {
  // PRI = PRI < ALT ? 1 :
  as_.cmp(eax, ecx);
  as_.setb(al);
  as_.movzx(eax, al);
}

void CompilerAsmjit::emit_leq() {
  // PRI = PRI <= ALT ? 1 :
  as_.cmp(eax, ecx);
  as_.setbe(al);
  as_.movzx(eax, al);
}

void CompilerAsmjit::emit_grtr() {
  // PRI = PRI > ALT ? 1 :
  as_.cmp(eax, ecx);
  as_.seta(al);
  as_.movzx(eax, al);
}

void CompilerAsmjit::emit_geq() {
  // PRI = PRI >= ALT ? 1 :
  as_.cmp(eax, ecx);
  as_.setae(al);
  as_.movzx(eax, al);
}

void CompilerAsmjit::emit_sless() {
  // PRI = PRI < ALT ? 1 :
  as_.cmp(eax, ecx);
  as_.setl(al);
  as_.movzx(eax, al);
}

void CompilerAsmjit::emit_sleq() {
  // PRI = PRI <= ALT ? 1 :
  as_.cmp(eax, ecx);
  as_.setle(al);
  as_.movzx(eax, al);
}

void CompilerAsmjit::emit_sgrtr() {
  // PRI = PRI > ALT ? 1 :
  as_.cmp(eax, ecx);
  as_.setg(al);
  as_.movzx(eax, al);
}

void CompilerAsmjit::emit_sgeq() {
  // PRI = PRI >= ALT ? 1 :
  as_.cmp(eax, ecx);
  as_.setge(al);
  as_.movzx(eax, al);
}

void CompilerAsmjit::emit_eq_c_pri(cell value) {
  // PRI = PRI == value ? 1 :
  as_.cmp(eax, value);
  as_.sete(al);
  as_.movzx(eax, al);
}

void CompilerAsmjit::emit_eq_c_alt(cell value) {
  // PRI = ALT == value ? 1 :
  as_.cmp(ecx, value);
  as_.sete(al);
  as_.movzx(eax, al);
}

void CompilerAsmjit::emit_inc_pri() {
  // PRI = PRI + 1
  as_.inc(eax);
}

void CompilerAsmjit::emit_inc_alt() {
  // ALT = ALT + 1
  as_.inc(ecx);
}

void CompilerAsmjit::emit_inc(cell address) {
  // [address] = [address] + 1
  as_.inc(dword_ptr(ebx, address));
}

void CompilerAsmjit::emit_inc_s(cell offset) {
  // [FRM + offset] = [FRM + offset] + 1
  as_.inc(dword_ptr(ebp, offset));
}

void CompilerAsmjit::emit_inc_i() {
  // [PRI] = [PRI] + 1
  as_.inc(dword_ptr(ebx, eax));
}

void CompilerAsmjit::emit_dec_pri() {
  // PRI = PRI - 1
  as_.dec(eax);
}

void CompilerAsmjit::emit_dec_alt() {
  // ALT = ALT - 1
  as_.dec(ecx);
}

void CompilerAsmjit::emit_dec(cell address) {
  // [address] = [address] - 1
  as_.dec(dword_ptr(ebx, address));
}

void CompilerAsmjit::emit_dec_s(cell offset) {
  // [FRM + offset] = [FRM + offset] - 1
  as_.dec(dword_ptr(ebp, offset));
}

void CompilerAsmjit::emit_dec_i() {
  // [PRI] = [PRI] - 1
  as_.dec(dword_ptr(ebx, eax));
}

void CompilerAsmjit::emit_movs(cell num_bytes) {
  // Copy memory from [PRI] to [ALT]. The parameter
  // specifies the number of bytes. The blocks should not
  // overlap.
  as_.cld();
  as_.lea(esi, dword_ptr(ebx, eax));
  as_.lea(edi, dword_ptr(ebx, ecx));
  as_.push(ecx);
  if (num_bytes % 4 == 0) {
    as_.mov(ecx, num_bytes / 4);
    as_.rep_movsd();
  } else if (num_bytes % 2 == 0) {
    as_.mov(ecx, num_bytes / 2);
    as_.rep_movsw();
  } else {
    as_.mov(ecx, num_bytes);
    as_.rep_movsb();
  }
  as_.pop(ecx);
}

void CompilerAsmjit::emit_cmps(cell num_bytes) {
  // Compare memory blocks at [PRI] and [ALT]. The parameter
  // specifies the number of bytes. The blocks should not
  // overlap.
  Label L_above = as_.newLabel();
  Label L_below = as_.newLabel();
  Label L_equal = as_.newLabel();
  Label L_continue = as_.newLabel();
    as_.cld();
    as_.lea(edi, dword_ptr(ebx, eax));
    as_.lea(esi, dword_ptr(ebx, ecx));
    as_.push(ecx);
    as_.mov(ecx, num_bytes);
    as_.repe_cmpsb();
    as_.pop(ecx);
    as_.ja(L_above);
    as_.jb(L_below);
    as_.jz(L_equal);
  as_.bind(L_above);
    as_.mov(eax, 1);
    as_.jmp(L_continue);
  as_.bind(L_below);
    as_.mov(eax, -1);
    as_.jmp(L_continue);
  as_.bind(L_equal);
    as_.xor_(eax, eax);
  as_.bind(L_continue);
}

void CompilerAsmjit::emit_fill(cell num_bytes) {
  // Fill memory at [ALT] with value in [PRI]. The parameter
  // specifies the number of bytes, which must be a multiple
  // of the cell size.
  as_.cld();
  as_.lea(edi, dword_ptr(ebx, ecx));
  as_.push(ecx);
  as_.mov(ecx, num_bytes / sizeof(cell));
  as_.rep_stosd();
  as_.pop(ecx);
}

void CompilerAsmjit::emit_halt(cell error_code) {
  // Abort execution (exit value in PRI), parameters other than 0
  // have a special meaning.
  as_.mov(edx, error_code);
  as_.jmp(L_halt_helper_);
}

void CompilerAsmjit::emit_bounds(cell value) {
  // Abort execution if PRI > value or if PRI < 0.
  Label L_halt = as_.newLabel();
  Label L_good = as_.newLabel();

    as_.cmp(eax, value);
    as_.jg(L_halt);

    as_.test(eax, eax);
    as_.jl(L_halt);

    as_.jmp(L_good);

  as_.bind(L_halt);
    as_.mov(edx, AMX_ERR_BOUNDS);
    as_.jmp(L_halt_helper_);

  as_.bind(L_good);
}

void CompilerAsmjit::emit_sysreq_pri() {
  // call system service, service number in PRI
  as_.push(esp); // stack_ptr
  as_.push(ebp); // stack_base
  as_.push(eax); // index
  as_.call(L_sysreq_c_helper_);
}

void CompilerAsmjit::emit_sysreq_c(cell index, const char *name) {
  // call system service
  if (name != 0) {
    if (!emit_intrinsic(name)) {
      as_.push(esp);   // stack_ptr
      as_.push(ebp);   // stack_base
      as_.push(index); // index
      as_.call(L_sysreq_c_helper_);
    }
  }
}

void CompilerAsmjit::emit_sysreq_d(cell address, const char *name) {
  // call system service
  if (name != 0) {
    if (!emit_intrinsic(name)) {
      as_.push(esp);     // stack_ptr
      as_.push(ebp);     // stack_base
      as_.push(address); // address
      as_.call(L_sysreq_d_helper_);
    }
  }
}

void CompilerAsmjit::emit_switch(const AMXCaseTable &case_table) {
  // Compare PRI to the values in the case table (whose address
  // is passed as an offset from CIP) and jump to the associated
  // address in the matching record.

  if (case_table.num_cases() > 0) {
    // Get minimum and maximum values.
    cell min_value = case_table.find_min_value();
    cell max_value = case_table.find_max_value();

    // Check if the value in eax is in the allowed range.
    // If not, jump to the default case (i.e. no match).
    as_.cmp(eax, min_value);
    as_.jl(amx_label(case_table.default_address()));
    as_.cmp(eax, max_value);
    as_.jg(amx_label(case_table.default_address()));

    // OK now sequentially compare eax with each value.
    // This is pretty slow so I probably should optimize
    // this in future...
    for (int i = 0; i < case_table.num_cases(); i++) {
      as_.cmp(eax, case_table.value_at(i));
      as_.je(amx_label(case_table.address_at(i)));
    }
  }

  // No match found - go for default case.
  as_.jmp(amx_label(case_table.default_address()));
}

void CompilerAsmjit::emit_casetbl() {
  // A variable number of case records follows this opcode, where
  // each record takes two cells.
}

void CompilerAsmjit::emit_swap_pri() {
  // [STK] = PRI and PRI = [STK]
  as_.xchg(dword_ptr(esp), eax);
}

void CompilerAsmjit::emit_swap_alt() {
  // [STK] = ALT and ALT = [STK]
  as_.xchg(dword_ptr(esp), ecx);
}

void CompilerAsmjit::emit_push_adr(cell offset) {
  // [STK] = FRM + offset, STK = STK - cell size
  as_.lea(edx, dword_ptr(ebp, offset));
  as_.sub(edx, ebx);
  as_.push(edx);
}

void CompilerAsmjit::emit_nop() {
  // no-operation, for code alignment
}

void CompilerAsmjit::emit_break() {
  // conditional breakpoint
  #ifdef DEBUG
    as_.call(L_break_helper_);
  #endif
}

intptr_t *CompilerAsmjit::get_runtime_data() {
  return reinterpret_cast<intptr_t*>(as_.getCode());
}

void CompilerAsmjit::set_runtime_data(int index, intptr_t data) {
  get_runtime_data()[index] = data;
}

void CompilerAsmjit::emit_runtime_data(AMXPtr amx) {
  as_.bind(L_exec_ptr_);
    as_.dd(0);
  as_.bind(L_amx_ptr_);
    as_.dintptr(reinterpret_cast<intptr_t>(amx.amx()));
  as_.bind(L_ebp_ptr_);
    as_.dd(0);
  as_.bind(L_esp_ptr_);
    as_.dd(0);
  as_.bind(L_reset_ebp_ptr_);
    as_.dd(0);
  as_.bind(L_reset_esp_ptr_);
    as_.dd(0);
  as_.bind(L_instr_map_size_);
    as_.dd(0);
  as_.bind(L_instr_map_ptr_);
    as_.dd(0);
}

void CompilerAsmjit::emit_instr_map(AMXPtr amx) {
  amxjit::AMXDisassembler disas(amx);
  amxjit::AMXInstruction instr;
  int size = 0;
  while (disas.decode(instr)) {
    size++;
  }

  set_runtime_data(RuntimeDataInstrMapSize, size);
  set_runtime_data(RuntimeDataInstrMapPtr, as_.getCodeSize());

  InstrMapEntry dummy = {0};
  for (int i = 0; i < size; i++) {
    as_.dstruct(dummy);
  }
}

void CompilerAsmjit::emit_get_amx_ptr(const GpReg &reg) {
  as_.mov(reg, dword_ptr(L_amx_ptr_));
}

void CompilerAsmjit::emit_get_amx_data_ptr(const GpReg &reg) {
  Label L_quit = as_.newLabel();

    emit_get_amx_ptr(eax);

    as_.mov(reg, dword_ptr(eax, offsetof(AMX, data)));
    as_.test(reg, reg);
    as_.jnz(L_quit);

    as_.mov(reg, dword_ptr(eax, offsetof(AMX, base)));
    as_.mov(eax, dword_ptr(reg, offsetof(AMX_HEADER, dat)));
    as_.add(reg, eax);

  as_.bind(L_quit);
}

// int AMXJIT_CDECL exec(cell index, cell *retval);
void CompilerAsmjit::emit_exec() {
  set_runtime_data(RuntimeDataExecPtr, as_.getCodeSize());

  Label L_stack_heap_overflow = as_.newLabel();
  Label L_heap_underflow = as_.newLabel();
  Label L_stack_underflow = as_.newLabel();
  Label L_native_not_found = as_.newLabel();
  Label L_public_not_found = as_.newLabel();
  Label L_finish = as_.newLabel();
  Label L_return = as_.newLabel();

  // Offsets of exec() arguments relative to ebp.
  int arg_index = 8;
  int arg_retval = 12;
  int var_address = -4;
  int var_reset_ebp = -8;
  int var_reset_esp = -12;

  as_.bind(L_exec_);
    as_.push(ebp);
    as_.mov(ebp, esp);
    as_.sub(esp, 12); // for locals

    as_.push(esi);
    emit_get_amx_ptr(esi);

    // JIT code expects AMX data pointer to be in ebx.
    as_.push(ebx);
    emit_get_amx_data_ptr(ebx);

    // Check for stack/heap collision (stack/heap overflow).
    as_.mov(ecx, dword_ptr(esi, offsetof(AMX, hea)));
    as_.mov(edx, dword_ptr(esi, offsetof(AMX, stk)));
    as_.cmp(ecx, edx);
    as_.jge(L_stack_heap_overflow);

    // Check for stack underflow.
    as_.mov(ecx, dword_ptr(esi, offsetof(AMX, stk)));
    as_.mov(edx, dword_ptr(esi, offsetof(AMX, stp)));
    as_.cmp(ecx, edx);
    as_.jg(L_stack_underflow);

    // Check for heap underflow.
    as_.mov(ecx, dword_ptr(esi, offsetof(AMX, hea)));
    as_.mov(edx, dword_ptr(esi, offsetof(AMX, hlw)));
    as_.cmp(ecx, edx);
    as_.jl(L_heap_underflow);

    // Make sure all natives are registered (the AMX_FLAG_NTVREG flag
    // must be set).
    as_.mov(ecx, dword_ptr(esi, offsetof(AMX, flags)));
    as_.test(ecx, AMX_FLAG_NTVREG);
    as_.jz(L_native_not_found);

    // Reset the error code.
    as_.mov(dword_ptr(esi, offsetof(AMX, error)), AMX_ERR_NONE);

    // Get address of the public function.
    as_.push(dword_ptr(ebp, arg_index));
    emit_get_amx_ptr(eax);
    as_.push(eax);
    as_.call(asmjit_cast<void*>(&get_public_addr));
    as_.add(esp, 8);

    // Check whether the function was found (address should be 0).
    as_.test(eax, eax);
    as_.jz(L_public_not_found);

    // Get pointer to the start of the function.
    as_.push(dword_ptr(L_instr_map_size_));
    as_.push(dword_ptr(L_instr_map_ptr_));
    as_.push(eax);
    as_.call(asmjit_cast<void*>(&get_instr_ptr));
    as_.add(esp, 12);
    as_.mov(dword_ptr(ebp, var_address), eax);

    // Push size of arguments and reset parameter count.
    // Pseudo code:
    //   stk = amx->stk - sizeof(cell);
    //   *(amx_data + stk) = amx->paramcount;
    //   amx->stk = stk;
    //   amx->paramcount = 0;
    as_.mov(eax, dword_ptr(esi, offsetof(AMX, paramcount)));
    as_.imul(eax, eax, sizeof(cell));
    as_.mov(ecx, dword_ptr(esi, offsetof(AMX, stk)));
    as_.sub(ecx, sizeof(cell));
    as_.mov(dword_ptr(ebx, ecx), eax);
    as_.mov(dword_ptr(esi, offsetof(AMX, stk)), ecx);
    as_.mov(dword_ptr(esi, offsetof(AMX, paramcount)), 0);

    // Keep a copy of the old reset_ebp and reset_esp on the stack.
    as_.mov(eax, dword_ptr(L_reset_ebp_ptr_));
    as_.mov(dword_ptr(ebp, var_reset_ebp), eax);
    as_.mov(eax, dword_ptr(L_reset_esp_ptr_));
    as_.mov(dword_ptr(ebp, var_reset_esp), eax);

    // Call the function.
    as_.push(dword_ptr(ebp, var_address));
    as_.call(L_exec_helper_);
    as_.add(esp, 4);

    // Copy return value to retval if it's not null.
    as_.mov(ecx, dword_ptr(ebp, arg_retval));
    as_.test(ecx, ecx);
    as_.jz(L_finish);
    as_.mov(dword_ptr(ecx), eax);

  as_.bind(L_finish);
    // Restore reset_ebp and reset_esp (remember they are kept in locals?).
    as_.mov(eax, dword_ptr(ebp, var_reset_ebp));
    as_.mov(dword_ptr(L_reset_ebp_ptr_), eax);
    as_.mov(eax, dword_ptr(ebp, var_reset_esp));
    as_.mov(dword_ptr(L_reset_esp_ptr_), eax);

    // Copy amx->error for return and reset it.
    as_.mov(eax, AMX_ERR_NONE);
    as_.xchg(eax, dword_ptr(esi, offsetof(AMX, error)));

  as_.bind(L_return);
    as_.pop(ebx);
    as_.pop(esi);
    as_.mov(esp, ebp);
    as_.pop(ebp);
    as_.ret();

  as_.bind(L_stack_heap_overflow);
    as_.mov(eax, AMX_ERR_STACKERR);
    as_.jmp(L_return);

  as_.bind(L_heap_underflow);
    as_.mov(eax, AMX_ERR_HEAPLOW);
    as_.jmp(L_return);

  as_.bind(L_stack_underflow);
    as_.mov(eax, AMX_ERR_STACKLOW);
    as_.jmp(L_return);

  as_.bind(L_native_not_found);
    as_.mov(eax, AMX_ERR_NOTFOUND);
    as_.jmp(L_return);

  as_.bind(L_public_not_found);
    as_.mov(eax, AMX_ERR_INDEX);
    as_.jmp(L_return);
}

// cell AMXJIT_CDECL exec_helper(void *address);
void CompilerAsmjit::emit_exec_helper() {
  as_.bind(L_exec_helper_);
    // Store function address in eax.
    as_.mov(eax, dword_ptr(esp, 4));

    // esi and edi are not saved across function bounds but generally
    // can be utilized in JIT code (for instance, in MOVS).
    as_.push(esi);
    as_.push(edi);

    // In JIT code these are caller-saved registers:
    //  eax - primary register (PRI)
    //  ecx - alternate register (ALT)
    //  ebx - data base pointer (DAT + amx->base)
    //  edx - temporary storage
    as_.push(ebx);
    as_.push(ecx);
    as_.push(edx);

    // Store old ebp and esp on the stack.
    as_.push(dword_ptr(L_ebp_ptr_));
    as_.push(dword_ptr(L_esp_ptr_));

    // Most recent ebp and esp are stored in member variables.
    as_.mov(dword_ptr(L_ebp_ptr_), ebp);
    as_.mov(dword_ptr(L_esp_ptr_), esp);

    // Switch from native stack to AMX stack.
    emit_get_amx_ptr(ecx);
    as_.mov(edx, dword_ptr(ecx, offsetof(AMX, frm)));
    as_.lea(ebp, dword_ptr(ebx, edx)); // ebp = amx_data + amx->frm
    as_.mov(edx, dword_ptr(ecx, offsetof(AMX, stk)));
    as_.lea(esp, dword_ptr(ebx, edx)); // esp = amx_data + amx->stk

    // In order to make halt() work we have to be able to return to this
    // point somehow. The easiest way it to set the stack registers as
    // if we called the offending instruction directly from here.
    as_.lea(ecx, dword_ptr(esp, - 4));
    as_.mov(dword_ptr(L_reset_esp_ptr_), ecx);
    as_.mov(dword_ptr(L_reset_ebp_ptr_), ebp);

    // Call the function. Prior to his point ebx should point to the
    // AMX data and the both stack pointers should point to somewhere
    // in the AMX stack.
    as_.call(eax);

    // Keep AMX stack registers up-to-date. This wouldn't be necessary if
    // RETN didn't modify them (it pops all arguments off the stack).
    emit_get_amx_ptr(eax);
    as_.mov(edx, ebp);
    as_.sub(edx, ebx);
    as_.mov(dword_ptr(eax, offsetof(AMX, frm)), edx); // amx->frm = ebp - amx_data
    as_.mov(edx, esp);
    as_.sub(edx, ebx);
    as_.mov(dword_ptr(eax, offsetof(AMX, stk)), edx); // amx->stk = esp - amx_data

    // Switch back to native stack.
    as_.mov(ebp, dword_ptr(L_ebp_ptr_));
    as_.mov(esp, dword_ptr(L_esp_ptr_));

    as_.pop(dword_ptr(L_esp_ptr_));
    as_.pop(dword_ptr(L_ebp_ptr_));

    as_.pop(edx);
    as_.pop(ecx);
    as_.pop(ebx);
    as_.pop(edi);
    as_.pop(esi);

    as_.ret();
}

// void halt_helper(int error [edx]);
void CompilerAsmjit::emit_halt_helper() {
  as_.bind(L_halt_helper_);
    emit_get_amx_ptr(esi);
    as_.mov(dword_ptr(esi, offsetof(AMX, error)), edx); // error code in edx

    // Reset stack so we can return right to call().
    as_.mov(esp, dword_ptr(L_reset_esp_ptr_));
    as_.mov(ebp, dword_ptr(L_reset_ebp_ptr_));

    // Pop public arguments as it would otherwise be done by RETN.
    as_.pop(eax);
    as_.add(esp, dword_ptr(esp));
    as_.add(esp, 4);
    as_.push(eax);

    as_.ret();
}

// void jump_helper(void *address [edx], void *stack_base [esi],
//                  void *stack_ptr [edi]);
void CompilerAsmjit::emit_jump_helper() {
  Label L_invalid_address = as_.newLabel();

  as_.bind(L_jump_helper_);
    as_.push(eax);
    as_.push(ecx);

    as_.push(dword_ptr(L_instr_map_size_));
    as_.push(dword_ptr(L_instr_map_ptr_));
    as_.push(edx);
    as_.call(asmjit_cast<void*>(&get_instr_ptr));
    as_.add(esp, 12);
    as_.mov(edx, eax); // address

    as_.pop(ecx);
    as_.pop(eax);

    as_.test(edx, edx);
    as_.jz(L_invalid_address);

    as_.mov(ebp, esi);
    as_.mov(esp, edi);
    as_.jmp(edx);

  // Continue execution as if no jump was initiated (this is what AMX does).
  as_.bind(L_invalid_address);
    as_.ret();
}

// cell AMXJIT_CDECL sysreq_c_helper(int index, void *stack_base, void *stack_ptr);
void CompilerAsmjit::emit_sysreq_c_helper() {
  Label L_native_not_found = as_.newLabel();
  Label L_return = as_.newLabel();

  int arg_index = 8;
  int arg_stack_base = 12;
  int arg_stack_ptr = 16;

  as_.bind(L_sysreq_c_helper_);
    as_.push(ebp);
    as_.mov(ebp, esp);

    as_.push(dword_ptr(ebp, arg_index));
    emit_get_amx_ptr(eax);
    as_.push(eax);
    as_.call(asmjit_cast<void*>(&get_native_addr));
    as_.add(esp, 8);
    as_.test(eax, eax);
    as_.jz(L_native_not_found);

    as_.push(dword_ptr(ebp, arg_stack_ptr));
    as_.push(dword_ptr(ebp, arg_stack_base));
    as_.push(eax); // address
    as_.call(L_sysreq_d_helper_);
    as_.add(esp, 12);

  as_.bind(L_return);
    as_.mov(esp, ebp);
    as_.pop(ebp);
    as_.ret();

  as_.bind(L_native_not_found);
    as_.mov(eax, AMX_ERR_NOTFOUND);
    as_.jmp(L_return);
}

// cell AMXJIT_CDECL sysreq_d_helper(void *address, void *stack_base, void *stack_ptr);
void CompilerAsmjit::emit_sysreq_d_helper() {
  as_.bind(L_sysreq_d_helper_);
    as_.mov(eax, dword_ptr(esp, 4));   // address
    as_.mov(ebp, dword_ptr(esp, 8));   // stack_base
    as_.mov(esp, dword_ptr(esp, 12));  // stack_ptr
    as_.mov(ecx, esp);                 // params
    as_.mov(esi, dword_ptr(esp, -16)); // return address

    emit_get_amx_ptr(edx);

    // Switch to native stack.
    as_.sub(ebp, ebx);
    as_.mov(dword_ptr(edx, offsetof(AMX, frm)), ebp); // amx->frm = ebp - amx_data
    as_.mov(ebp, dword_ptr(L_ebp_ptr_));
    as_.sub(esp, ebx);
    as_.mov(dword_ptr(edx, offsetof(AMX, stk)), esp); // amx->stk = esp - amx_data
    as_.mov(esp, dword_ptr(L_esp_ptr_));

    // Call the native function.
    as_.push(ecx); // params
    as_.push(edx); // amx
    as_.call(eax); // address
    as_.add(esp, 8);
    // eax contains return value, the code below must not overwrite it!!

    // Switch back to AMX stack.
    emit_get_amx_ptr(edx);
    as_.mov(dword_ptr(L_ebp_ptr_), ebp);
    as_.mov(ecx, dword_ptr(edx, offsetof(AMX, frm)));
    as_.lea(ebp, dword_ptr(ebx, ecx)); // ebp = amx_data + amx->frm
    as_.mov(dword_ptr(L_esp_ptr_), esp);
    as_.mov(ecx, dword_ptr(edx, offsetof(AMX, stk)));
    as_.lea(esp, dword_ptr(ebx, ecx)); // ebp = amx_data + amx->stk

    // Modify the return address so we return next to the sysreq point.
    as_.push(esi);
    as_.ret();
}

// void break_helper();
void CompilerAsmjit::emit_break_helper() {
  Label L_return = as_.newLabel();

  as_.bind(L_break_helper_);
    emit_get_amx_ptr(edx);
    as_.mov(esi, dword_ptr(edx, offsetof(AMX, debug)));
    as_.test(esi, esi);
    as_.jz(L_return);

    as_.push(eax);
    as_.push(ecx);

    as_.push(edx);
    as_.call(esi);
    as_.add(esp, 4);

    as_.pop(ecx);
    as_.pop(eax);

  as_.bind(L_return);
    as_.ret();
}

void CompilerAsmjit::emit_float() {
  as_.fild(dword_ptr(esp, 4));
  as_.sub(esp, 4);
  as_.fstp(dword_ptr(esp));
  as_.mov(eax, dword_ptr(esp));
  as_.add(esp, 4);
}

void CompilerAsmjit::emit_floatabs() {
  as_.fld(dword_ptr(esp, 4));
  as_.fabs();
  as_.sub(esp, 4);
  as_.fstp(dword_ptr(esp));
  as_.mov(eax, dword_ptr(esp));
  as_.add(esp, 4);
}

void CompilerAsmjit::emit_floatadd() {
  as_.fld(dword_ptr(esp, 4));
  as_.fadd(dword_ptr(esp, 8));
  as_.sub(esp, 4);
  as_.fstp(dword_ptr(esp));
  as_.mov(eax, dword_ptr(esp));
  as_.add(esp, 4);
}

void CompilerAsmjit::emit_floatsub() {
  as_.fld(dword_ptr(esp, 4));
  as_.fsub(dword_ptr(esp, 8));
  as_.sub(esp, 4);
  as_.fstp(dword_ptr(esp));
  as_.mov(eax, dword_ptr(esp));
  as_.add(esp, 4);
}

void CompilerAsmjit::emit_floatmul() {
  as_.fld(dword_ptr(esp, 4));
  as_.fmul(dword_ptr(esp, 8));
  as_.sub(esp, 4);
  as_.fstp(dword_ptr(esp));
  as_.mov(eax, dword_ptr(esp));
  as_.add(esp, 4);
}

void CompilerAsmjit::emit_floatdiv() {
  as_.fld(dword_ptr(esp, 4));
  as_.fdiv(dword_ptr(esp, 8));
  as_.sub(esp, 4);
  as_.fstp(dword_ptr(esp));
  as_.mov(eax, dword_ptr(esp));
  as_.add(esp, 4);
}

void CompilerAsmjit::emit_floatsqroot() {
  as_.fld(dword_ptr(esp, 4));
  as_.fsqrt();
  as_.sub(esp, 4);
  as_.fstp(dword_ptr(esp));
  as_.mov(eax, dword_ptr(esp));
  as_.add(esp, 4);
}

void CompilerAsmjit::emit_floatlog() {
  as_.fld1();
  as_.fld(dword_ptr(esp, 8));
  as_.fyl2x();
  as_.fld1();
  as_.fdivrp(st(1));
  as_.fld(dword_ptr(esp, 4));
  as_.fyl2x();
  as_.sub(esp, 4);
  as_.fstp(dword_ptr(esp));
  as_.mov(eax, dword_ptr(esp));
  as_.add(esp, 4);
}

bool CompilerAsmjit::emit_intrinsic(const char *name) {
  struct Intrinsic {
    const char    *name;
    EmitIntrinsic  emit;
  };
  
  Intrinsic intrinsics[] = {
    {"float",       &CompilerAsmjit::emit_float},
    {"floatabs",    &CompilerAsmjit::emit_floatabs},
    {"floatadd",    &CompilerAsmjit::emit_floatadd},
    {"floatsub",    &CompilerAsmjit::emit_floatsub},
    {"floatmul",    &CompilerAsmjit::emit_floatmul},
    {"floatdiv",    &CompilerAsmjit::emit_floatdiv},
    {"floatsqroot", &CompilerAsmjit::emit_floatsqroot},
    {"floatlog",    &CompilerAsmjit::emit_floatlog}
  };

  for (std::size_t i = 0; i < sizeof(intrinsics) / sizeof(*intrinsics); i++) {
    if (std::strcmp(intrinsics[i].name, name) == 0) {
      (this->*intrinsics[i].emit)();
      return true;
    }
  }

  return false;
}

} // namespace amxjit
