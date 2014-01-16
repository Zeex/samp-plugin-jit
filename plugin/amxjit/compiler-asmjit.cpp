// Copyright (c) 2012-2014 Zeex
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
#include <string>
#include "compiler-asmjit.h"
#include "cstdint.h"
#include "disasm.h"

// AsmJit core
using AsmJit::Label;

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

struct RuntimeInfoBlock {
  amxjit::intptr_t exec;
  amxjit::intptr_t amx;
  amxjit::intptr_t ebp;
  amxjit::intptr_t esp;
  amxjit::intptr_t reset_ebp;
  amxjit::intptr_t reset_esp;
  amxjit::intptr_t instr_table;
  amxjit::intptr_t instr_table_size;
};

cell AMXJIT_CDECL GetPublicAddress(AMX *amx, int index) {
  return amxjit::AMXPtr(amx).GetPublicAddress(index);
}

cell AMXJIT_CDECL GetNativeAddress(AMX *amx, int index) {
  return amxjit::AMXPtr(amx).GetNativeAddress(index);
}

class InstrTableEntry {
 public:
  InstrTableEntry(): address(), start() {}
  InstrTableEntry(cell address): address(address), start() {}
  bool operator<(const InstrTableEntry &other) const {
    return address < other.address;
  }
 public:
  cell address;
  void *start;
};

void *AMXJIT_CDECL GetInstrStartPtr(cell address, RuntimeInfoBlock *rib) {
  assert(rib->instr_table != 0);
  assert(rib->instr_table_size > 0);
  InstrTableEntry *instr_table =
    reinterpret_cast<InstrTableEntry*>(rib->instr_table);
  InstrTableEntry target(address);
  std::pair<InstrTableEntry*, InstrTableEntry*> result = 
    std::equal_range(instr_table, instr_table + rib->instr_table_size, target);
  if (result.first != result.second) {
    return result.first->start;
  }
  return 0;
}

} // anonymous namespace

namespace amxjit {

CompilerOutputAsmjit::CompilerOutputAsmjit(void *code,
                                           std::size_t code_size):
  code_(code),
  code_size_(code_size)
{
}

CompilerOutputAsmjit::~CompilerOutputAsmjit() {
  AsmJit::MemoryManager::getGlobal()->free(code_);
}

EntryPoint CompilerOutputAsmjit::GetEntryPoint() const {
  assert(code_ != 0);
  return (EntryPoint)*reinterpret_cast<void**>(GetCode());
}

CompilerAsmjit::CompilerAsmjit():
  asm_(),
  rib_start_label_(asm_.newLabel()),
  exec_ptr_label_(asm_.newLabel()),
  amx_ptr_label_(asm_.newLabel()),
  ebp_ptr_label_(asm_.newLabel()),
  esp_ptr_label_(asm_.newLabel()),
  reset_ebp_ptr_label_(asm_.newLabel()),
  reset_esp_ptr_label_(asm_.newLabel()),
  instr_table_ptr_label_(asm_.newLabel()),
  instr_table_size_label_(asm_.newLabel()),
  exec_label_(asm_.newLabel()),
  exec_helper_label_(asm_.newLabel()),
  halt_helper_label_(asm_.newLabel()),
  jump_helper_label_(asm_.newLabel()),
  sysreq_c_helper_label_(asm_.newLabel()),
  sysreq_d_helper_label_(asm_.newLabel())
{
}

CompilerAsmjit::~CompilerAsmjit()
{
}

bool CompilerAsmjit::Setup() { 
  EmitRuntimeInfo();
  EmitInstrTable();
  EmitExec();
  EmitExecHelper();
  EmitHaltHelper();
  EmitJumpHelper();
  EmitSysreqCHelper();
  EmitSysreqDHelper();
  return true;
}

bool CompilerAsmjit::Process(const Instruction &instr) {
  cell cip = instr.address();
  asm_.bind(GetLabel(cip));
  instr_map_[cip] = asm_.getCodeSize();
  return true;
}

void CompilerAsmjit::Abort() {
  // do nothing
}

CompilerOutput *CompilerAsmjit::Finish() {
  void *code = asm_.make();
  RuntimeInfoBlock *rib = reinterpret_cast<RuntimeInfoBlock*>(code);

  rib->exec += reinterpret_cast<intptr_t>(code);
  rib->instr_table += reinterpret_cast<intptr_t>(code);

  InstrTableEntry *ite = reinterpret_cast<InstrTableEntry*>(rib->instr_table);
  for (InstrMap::const_iterator it = instr_map_.begin();
       it != instr_map_.end(); it++) {
    ite->address = it->first;
    ite->start = static_cast<unsigned char*>(code) + it->second;
    ite++;
  }

  return new CompilerOutputAsmjit(code, asm_.getCodeSize());
}

void CompilerAsmjit::load_pri(cell address) {
  // PRI = [address]
  asm_.mov(eax, dword_ptr(ebx, address));
}

void CompilerAsmjit::load_alt(cell address) {
  // ALT = [address]
  asm_.mov(ecx, dword_ptr(ebx, address));
}

void CompilerAsmjit::load_s_pri(cell offset) {
  // PRI = [FRM + offset]
  asm_.mov(eax, dword_ptr(ebp, offset));
}

void CompilerAsmjit::load_s_alt(cell offset) {
  // ALT = [FRM + offset]
  asm_.mov(ecx, dword_ptr(ebp, offset));
}

void CompilerAsmjit::lref_pri(cell address) {
  // PRI = [ [address] ]
  asm_.mov(edx, dword_ptr(ebx, address));
  asm_.mov(eax, dword_ptr(ebx, edx));
}

void CompilerAsmjit::lref_alt(cell address) {
  // ALT = [ [address] ]
  asm_.mov(edx, dword_ptr(ebx, + address));
  asm_.mov(ecx, dword_ptr(ebx, edx));
}

void CompilerAsmjit::lref_s_pri(cell offset) {
  // PRI = [ [FRM + offset] ]
  asm_.mov(edx, dword_ptr(ebp, offset));
  asm_.mov(eax, dword_ptr(ebx, edx));
}

void CompilerAsmjit::lref_s_alt(cell offset) {
  // PRI = [ [FRM + offset] ]
  asm_.mov(edx, dword_ptr(ebp, offset));
  asm_.mov(ecx, dword_ptr(ebx, edx));
}

void CompilerAsmjit::load_i() {
  // PRI = [PRI] (full cell)
  asm_.mov(eax, dword_ptr(ebx, eax));
}

void CompilerAsmjit::lodb_i(cell number) {
  // PRI = "number" bytes from [PRI] (read 1/2/4 bytes)
  switch (number) {
    case 1:
      asm_.movzx(eax, byte_ptr(ebx, eax));
      break;
    case 2:
      asm_.movzx(eax, word_ptr(ebx, eax));
      break;
    case 4:
      asm_.mov(eax, dword_ptr(ebx, eax));
      break;
  }
}

void CompilerAsmjit::const_pri(cell value) {
  // PRI = value
  if (value == 0) {
    asm_.xor_(eax, eax);
  } else {
    asm_.mov(eax, value);
  }
}

void CompilerAsmjit::const_alt(cell value) {
  // ALT = value
  if (value == 0) {
    asm_.xor_(ecx, ecx);
  } else {
    asm_.mov(ecx, value);
  }
}

void CompilerAsmjit::addr_pri(cell offset) {
  // PRI = FRM + offset
  asm_.lea(eax, dword_ptr(ebp, offset));
  asm_.sub(eax, ebx);
}

void CompilerAsmjit::addr_alt(cell offset) {
  // ALT = FRM + offset
  asm_.lea(ecx, dword_ptr(ebp, offset));
  asm_.sub(ecx, ebx);
}

void CompilerAsmjit::stor_pri(cell address) {
  // [address] = PRI
  asm_.mov(dword_ptr(ebx, address), eax);
}

void CompilerAsmjit::stor_alt(cell address) {
  // [address] = ALT
  asm_.mov(dword_ptr(ebx, address), ecx);
}

void CompilerAsmjit::stor_s_pri(cell offset) {
  // [FRM + offset] = ALT
  asm_.mov(dword_ptr(ebp, offset), eax);
}

void CompilerAsmjit::stor_s_alt(cell offset) {
  // [FRM + offset] = ALT
  asm_.mov(dword_ptr(ebp, offset), ecx);
}

void CompilerAsmjit::sref_pri(cell address) {
  // [ [address] ] = PRI
  asm_.mov(edx, dword_ptr(ebx, address));
  asm_.mov(dword_ptr(ebx, edx), eax);
}

void CompilerAsmjit::sref_alt(cell address) {
  // [ [address] ] = ALT
  asm_.mov(edx, dword_ptr(ebx, address));
  asm_.mov(dword_ptr(ebx, edx), ecx);
}

void CompilerAsmjit::sref_s_pri(cell offset) {
  // [ [FRM + offset] ] = PRI
  asm_.mov(edx, dword_ptr(ebp, offset));
  asm_.mov(dword_ptr(ebx, edx), eax);
}

void CompilerAsmjit::sref_s_alt(cell offset) {
  // [ [FRM + offset] ] = ALT
  asm_.mov(edx, dword_ptr(ebp, offset));
  asm_.mov(dword_ptr(ebx, edx), ecx);
}

void CompilerAsmjit::stor_i() {
  // [ALT] = PRI (full cell)
  asm_.mov(dword_ptr(ebx, ecx), eax);
}

void CompilerAsmjit::strb_i(cell number) {
  // "number" bytes at [ALT] = PRI (write 1/2/4 bytes)
  switch (number) {
    case 1:
      asm_.mov(byte_ptr(ebx, ecx), al);
      break;
    case 2:
      asm_.mov(word_ptr(ebx, ecx), ax);
      break;
    case 4:
      asm_.mov(dword_ptr(ebx, ecx), eax);
      break;
  }
}

void CompilerAsmjit::lidx() {
  // PRI = [ ALT + (PRI x cell size) ]
  asm_.lea(edx, dword_ptr(ebx, ecx));
  asm_.mov(eax, dword_ptr(edx, eax, 2));
}

void CompilerAsmjit::lidx_b(cell shift) {
  // PRI = [ ALT + (PRI << shift) ]
  asm_.lea(edx, dword_ptr(ebx, ecx));
  asm_.mov(eax, dword_ptr(edx, eax, shift));
}

void CompilerAsmjit::idxaddr() {
  // PRI = ALT + (PRI x cell size) (calculate indexed address)
  asm_.lea(eax, dword_ptr(ecx, eax, 2));
}

void CompilerAsmjit::idxaddr_b(cell shift) {
  // PRI = ALT + (PRI << shift) (calculate indexed address)
  asm_.lea(eax, dword_ptr(ecx, eax, shift));
}

void CompilerAsmjit::align_pri(cell number) {
  // Little Endian: PRI ^= cell size - number
  #if BYTE_ORDER == LITTLE_ENDIAN
    if (static_cast<std::size_t>(number) < sizeof(cell)) {
      asm_.xor_(eax, sizeof(cell) - number);
    }
  #endif
}

void CompilerAsmjit::align_alt(cell number) {
  // Little Endian: ALT ^= cell size - number
  #if BYTE_ORDER == LITTLE_ENDIAN
    if (static_cast<std::size_t>(number) < sizeof(cell)) {
      asm_.xor_(ecx, sizeof(cell) - number);
    }
  #endif
}

void CompilerAsmjit::lctrl(cell index) {
  // PRI is set to the current value of any of the special registers.
  // The index parameter must be:
  // 3=STP, 4=STK, 5=FRM, 6=CIP (of the next instruction)
  switch (index) {
    case 0:
    case 1:
    case 2:
    case 3:
      EmitAmxPtrMove(eax);
      switch (index) {
        case 0:
          asm_.mov(eax, dword_ptr(eax, offsetof(AMX, base)));
          asm_.mov(eax, dword_ptr(edx, offsetof(AMX_HEADER, cod)));
          break;
        case 1:
          asm_.mov(eax, dword_ptr(eax, offsetof(AMX, base)));
          asm_.mov(eax, dword_ptr(edx, offsetof(AMX_HEADER, dat)));
          break;
        case 2:
          asm_.mov(eax, dword_ptr(eax, offsetof(AMX, hea)));
          break;
        case 3:
          asm_.mov(eax, dword_ptr(eax, offsetof(AMX, stp)));
          break;
      }
      break;
    case 4:
      asm_.mov(eax, esp);
      asm_.sub(eax, ebx);
      break;
    case 5:
      asm_.mov(eax, ebp);
      asm_.sub(eax, ebx);
      break;
    case 6:
      asm_.mov(eax, GetCurrentInstr().address() + GetCurrentInstr().size());
      break;
    case 7:
      asm_.mov(eax, 1);
      break;
  }
}

void CompilerAsmjit::sctrl(cell index) {
  // set the indexed special registers to the value in PRI.
  // The index parameter must be:
  // 6=CIP
  switch (index) {
    case 2:
      EmitAmxPtrMove(edx);
      asm_.mov(dword_ptr(edx, offsetof(AMX, hea)), eax);
      break;
    case 4:
      asm_.lea(esp, dword_ptr(ebx, eax));
      break;
    case 5:
      asm_.lea(ebp, dword_ptr(ebx, eax));
      break;
    case 6:
      asm_.mov(edi, esp);
      asm_.mov(esi, ebp);
      asm_.mov(edx, eax);
      asm_.call(jump_helper_label_);
      break;
  }
}

void CompilerAsmjit::move_pri() {
  // PRI = ALT
  asm_.mov(eax, ecx);
}

void CompilerAsmjit::move_alt() {
  // ALT = PRI
  asm_.mov(ecx, eax);
}

void CompilerAsmjit::xchg() {
  // Exchange PRI and ALT
  asm_.xchg(eax, ecx);
}

void CompilerAsmjit::push_pri() {
  // [STK] = PRI, STK = STK - cell size
  asm_.push(eax);
}

void CompilerAsmjit::push_alt() {
  // [STK] = ALT, STK = STK - cell size
  asm_.push(ecx);
}

void CompilerAsmjit::push_c(cell value) {
  // [STK] = value, STK = STK - cell size
  asm_.push(value);
}

void CompilerAsmjit::push(cell address) {
  // [STK] = [address], STK = STK - cell size
  asm_.push(dword_ptr(ebx, address));
}

void CompilerAsmjit::push_s(cell offset) {
  // [STK] = [FRM + offset], STK = STK - cell size
  asm_.push(dword_ptr(ebp, offset));
}

void CompilerAsmjit::pop_pri() {
  // STK = STK + cell size, PRI = [STK]
  asm_.pop(eax);
}

void CompilerAsmjit::pop_alt() {
  // STK = STK + cell size, ALT = [STK]
  asm_.pop(ecx);
}

void CompilerAsmjit::stack(cell value) {
  // ALT = STK, STK = STK + value
  asm_.mov(ecx, esp);
  asm_.sub(ecx, ebx);
  if (value >= 0) {
    asm_.add(esp, value);
  } else {
    asm_.sub(esp, -value);
  }
}

void CompilerAsmjit::heap(cell value) {
  // ALT = HEA, HEA = HEA + value
  EmitAmxPtrMove(edx);
  asm_.mov(ecx, dword_ptr(edx, offsetof(AMX, hea)));
  if (value >= 0) {
    asm_.add(dword_ptr(edx, offsetof(AMX, hea)), value);
  } else {
    asm_.sub(dword_ptr(edx, offsetof(AMX, hea)), -value);
  }
}

void CompilerAsmjit::proc() {
  // [STK] = FRM, STK = STK - cell size, FRM = STK
  asm_.align(16);
  asm_.push(ebp);
  asm_.mov(ebp, esp);
  asm_.sub(dword_ptr(esp), ebx);
}

void CompilerAsmjit::ret() {
  // STK = STK + cell size, FRM = [STK],
  // CIP = [STK], STK = STK + cell size
  asm_.pop(ebp);
  asm_.add(ebp, ebx);
  asm_.ret();
}

void CompilerAsmjit::retn() {
  // STK = STK + cell size, FRM = [STK],
  // CIP = [STK], STK = STK + cell size
  // The RETN instruction removes a specified number of bytes
  // from the stack. The value to adjust STK with must be
  // pushed prior to the call.
  asm_.pop(ebp);
  asm_.add(ebp, ebx);
  asm_.pop(edx);
  asm_.add(esp, dword_ptr(esp));
  asm_.push(edx);
  asm_.ret(4);
}

void CompilerAsmjit::call(cell address) {
  // [STK] = CIP + 5, STK = STK - cell size
  // CIP = CIP + offset
  // The CALL instruction jumps to an address after storing the
  // address of the next sequential instruction on the stack.
  // The address jumped to is relative to the current CIP,
  // but the address on the stack is an absolute address.
  asm_.call(GetLabel(address));
}

void CompilerAsmjit::jump_pri() {
  // CIP = PRI (indirect jump)
  asm_.mov(edi, esp);
  asm_.mov(esi, ebp);
  asm_.mov(edx, eax);
  asm_.call(jump_helper_label_);
}

void CompilerAsmjit::jump(cell address) {
  // CIP = CIP + offset (jump to the address relative from
  // the current position)
  asm_.jmp(GetLabel(address));
}

void CompilerAsmjit::jzer(cell address) {
  // if PRI == 0 then CIP = CIP + offset
  asm_.test(eax, eax);
  asm_.jz(GetLabel(address));
}

void CompilerAsmjit::jnz(cell address) {
  // if PRI != 0 then CIP = CIP + offset
  asm_.test(eax, eax);
  asm_.jnz(GetLabel(address));
}

void CompilerAsmjit::jeq(cell address) {
  // if PRI == ALT then CIP = CIP + offset
  asm_.cmp(eax, ecx);
  asm_.je(GetLabel(address));
}

void CompilerAsmjit::jneq(cell address) {
  // if PRI != ALT then CIP = CIP + offset
  asm_.cmp(eax, ecx);
  asm_.jne(GetLabel(address));
}

void CompilerAsmjit::jless(cell address) {
  // if PRI < ALT then CIP = CIP + offset (unsigned)
  asm_.cmp(eax, ecx);
  asm_.jb(GetLabel(address));
}

void CompilerAsmjit::jleq(cell address) {
  // if PRI <= ALT then CIP = CIP + offset (unsigned)
  asm_.cmp(eax, ecx);
  asm_.jbe(GetLabel(address));
}

void CompilerAsmjit::jgrtr(cell address) {
  // if PRI > ALT then CIP = CIP + offset (unsigned)
  asm_.cmp(eax, ecx);
  asm_.ja(GetLabel(address));
}

void CompilerAsmjit::jgeq(cell address) {
  // if PRI >= ALT then CIP = CIP + offset (unsigned)
  asm_.cmp(eax, ecx);
  asm_.jae(GetLabel(address));
}

void CompilerAsmjit::jsless(cell address) {
  // if PRI < ALT then CIP = CIP + offset (signed)
  asm_.cmp(eax, ecx);
  asm_.jl(GetLabel(address));
}

void CompilerAsmjit::jsleq(cell address) {
  // if PRI <= ALT then CIP = CIP + offset (signed)
  asm_.cmp(eax, ecx);
  asm_.jle(GetLabel(address));
}

void CompilerAsmjit::jsgrtr(cell address) {
  // if PRI > ALT then CIP = CIP + offset (signed)
  asm_.cmp(eax, ecx);
  asm_.jg(GetLabel(address));
}

void CompilerAsmjit::jsgeq(cell address) {
  // if PRI >= ALT then CIP = CIP + offset (signed)
  asm_.cmp(eax, ecx);
  asm_.jge(GetLabel(address));
}

void CompilerAsmjit::shl() {
  // PRI = PRI << ALT
  asm_.shl(eax, cl);
}

void CompilerAsmjit::shr() {
  // PRI = PRI >> ALT (without sign extension)
  asm_.shr(eax, cl);
}

void CompilerAsmjit::sshr() {
  // PRI = PRI >> ALT with sign extension
  asm_.sar(eax, cl);
}

void CompilerAsmjit::shl_c_pri(cell value) {
  // PRI = PRI << value
  asm_.shl(eax, static_cast<unsigned char>(value));
}

void CompilerAsmjit::shl_c_alt(cell value) {
  // ALT = ALT << value
  asm_.shl(ecx, static_cast<unsigned char>(value));
}

void CompilerAsmjit::shr_c_pri(cell value) {
  // PRI = PRI >> value (without sign extension)
  asm_.shr(eax, static_cast<unsigned char>(value));
}

void CompilerAsmjit::shr_c_alt(cell value) {
  // PRI = PRI >> value (without sign extension)
  asm_.shl(ecx, static_cast<unsigned char>(value));
}

void CompilerAsmjit::smul() {
  // PRI = PRI * ALT (signed multiply)
  asm_.imul(ecx);
}

void CompilerAsmjit::sdiv() {
  // PRI = PRI / ALT (signed divide), ALT = PRI mod ALT
  asm_.cdq();
  asm_.idiv(ecx);
  asm_.mov(ecx, edx);
}

void CompilerAsmjit::sdiv_alt() {
  // PRI = ALT / PRI (signed divide), ALT = ALT mod PRI
  asm_.xchg(eax, ecx);
  asm_.cdq();
  asm_.idiv(ecx);
  asm_.mov(ecx, edx);
}

void CompilerAsmjit::umul() {
  // PRI = PRI * ALT (unsigned multiply)
  asm_.mul(ecx);
}

void CompilerAsmjit::udiv() {
  // PRI = PRI / ALT (unsigned divide), ALT = PRI mod ALT
  asm_.xor_(edx, edx);
  asm_.div(ecx);
  asm_.mov(ecx, edx);
}

void CompilerAsmjit::udiv_alt() {
  // PRI = ALT / PRI (unsigned divide), ALT = ALT mod PRI
  asm_.xchg(eax, ecx);
  asm_.xor_(edx, edx);
  asm_.div(ecx);
  asm_.mov(ecx, edx);
}

void CompilerAsmjit::add() {
  // PRI = PRI + ALT
  asm_.add(eax, ecx);
}

void CompilerAsmjit::sub() {
  // PRI = PRI - ALT
  asm_.sub(eax, ecx);
}

void CompilerAsmjit::sub_alt() {
  // PRI = ALT - PRI
  // or:
  // PRI = -(PRI - ALT)
  asm_.sub(eax, ecx);
  asm_.neg(eax);
}

void CompilerAsmjit::and_() {
  // PRI = PRI & ALT
  asm_.and_(eax, ecx);
}

void CompilerAsmjit::or_() {
  // PRI = PRI | ALT
  asm_.or_(eax, ecx);
}

void CompilerAsmjit::xor_() {
  // PRI = PRI ^ ALT
  asm_.xor_(eax, ecx);
}

void CompilerAsmjit::not_() {
  // PRI = !PRI
  asm_.test(eax, eax);
  asm_.setz(al);
  asm_.movzx(eax, al);
}

void CompilerAsmjit::neg() {
  // PRI = -PRI
  asm_.neg(eax);
}

void CompilerAsmjit::invert() {
  // PRI = ~PRI
  asm_.not_(eax);
}

void CompilerAsmjit::add_c(cell value) {
  // PRI = PRI + value
  if (value >= 0) {
    asm_.add(eax, value);
  } else {
    asm_.sub(eax, -value);
  }
}

void CompilerAsmjit::smul_c(cell value) {
  // PRI = PRI * value
  asm_.imul(eax, value);
}

void CompilerAsmjit::zero_pri() {
  // PRI = 0
  asm_.xor_(eax, eax);
}

void CompilerAsmjit::zero_alt() {
  // ALT = 0
  asm_.xor_(ecx, ecx);
}

void CompilerAsmjit::zero(cell address) {
  // [address] = 0
  asm_.mov(dword_ptr(ebx, address), 0);
}

void CompilerAsmjit::zero_s(cell offset) {
  // [FRM + offset] = 0
  asm_.mov(dword_ptr(ebp, offset), 0);
}

void CompilerAsmjit::sign_pri() {
  // sign extent the byte in PRI to a cell
  asm_.movsx(eax, al);
}

void CompilerAsmjit::sign_alt() {
  // sign extent the byte in ALT to a cell
  asm_.movsx(ecx, cl);
}

void CompilerAsmjit::eq() {
  // PRI = PRI == ALT ? 1 :
  asm_.cmp(eax, ecx);
  asm_.sete(al);
  asm_.movzx(eax, al);
}

void CompilerAsmjit::neq() {
  // PRI = PRI != ALT ? 1 :
  asm_.cmp(eax, ecx);
  asm_.setne(al);
  asm_.movzx(eax, al);
}

void CompilerAsmjit::less() {
  // PRI = PRI < ALT ? 1 :
  asm_.cmp(eax, ecx);
  asm_.setb(al);
  asm_.movzx(eax, al);
}

void CompilerAsmjit::leq() {
  // PRI = PRI <= ALT ? 1 :
  asm_.cmp(eax, ecx);
  asm_.setbe(al);
  asm_.movzx(eax, al);
}

void CompilerAsmjit::grtr() {
  // PRI = PRI > ALT ? 1 :
  asm_.cmp(eax, ecx);
  asm_.seta(al);
  asm_.movzx(eax, al);
}

void CompilerAsmjit::geq() {
  // PRI = PRI >= ALT ? 1 :
  asm_.cmp(eax, ecx);
  asm_.setae(al);
  asm_.movzx(eax, al);
}

void CompilerAsmjit::sless() {
  // PRI = PRI < ALT ? 1 :
  asm_.cmp(eax, ecx);
  asm_.setl(al);
  asm_.movzx(eax, al);
}

void CompilerAsmjit::sleq() {
  // PRI = PRI <= ALT ? 1 :
  asm_.cmp(eax, ecx);
  asm_.setle(al);
  asm_.movzx(eax, al);
}

void CompilerAsmjit::sgrtr() {
  // PRI = PRI > ALT ? 1 :
  asm_.cmp(eax, ecx);
  asm_.setg(al);
  asm_.movzx(eax, al);
}

void CompilerAsmjit::sgeq() {
  // PRI = PRI >= ALT ? 1 :
  asm_.cmp(eax, ecx);
  asm_.setge(al);
  asm_.movzx(eax, al);
}

void CompilerAsmjit::eq_c_pri(cell value) {
  // PRI = PRI == value ? 1 :
  asm_.cmp(eax, value);
  asm_.sete(al);
  asm_.movzx(eax, al);
}

void CompilerAsmjit::eq_c_alt(cell value) {
  // PRI = ALT == value ? 1 :
  asm_.cmp(ecx, value);
  asm_.sete(al);
  asm_.movzx(eax, al);
}

void CompilerAsmjit::inc_pri() {
  // PRI = PRI + 1
  asm_.inc(eax);
}

void CompilerAsmjit::inc_alt() {
  // ALT = ALT + 1
  asm_.inc(ecx);
}

void CompilerAsmjit::inc(cell address) {
  // [address] = [address] + 1
  asm_.inc(dword_ptr(ebx, address));
}

void CompilerAsmjit::inc_s(cell offset) {
  // [FRM + offset] = [FRM + offset] + 1
  asm_.inc(dword_ptr(ebp, offset));
}

void CompilerAsmjit::inc_i() {
  // [PRI] = [PRI] + 1
  asm_.inc(dword_ptr(ebx, eax));
}

void CompilerAsmjit::dec_pri() {
  // PRI = PRI - 1
  asm_.dec(eax);
}

void CompilerAsmjit::dec_alt() {
  // ALT = ALT - 1
  asm_.dec(ecx);
}

void CompilerAsmjit::dec(cell address) {
  // [address] = [address] - 1
  asm_.dec(dword_ptr(ebx, address));
}

void CompilerAsmjit::dec_s(cell offset) {
  // [FRM + offset] = [FRM + offset] - 1
  asm_.dec(dword_ptr(ebp, offset));
}

void CompilerAsmjit::dec_i() {
  // [PRI] = [PRI] - 1
  asm_.dec(dword_ptr(ebx, eax));
}

void CompilerAsmjit::movs(cell num_bytes) {
  // Copy memory from [PRI] to [ALT]. The parameter
  // specifies the number of bytes. The blocks should not
  // overlap.
  asm_.cld();
  asm_.lea(esi, dword_ptr(ebx, eax));
  asm_.lea(edi, dword_ptr(ebx, ecx));
  asm_.push(ecx);
  if (num_bytes % 4 == 0) {
    asm_.mov(ecx, num_bytes / 4);
    asm_.rep_movsd();
  } else if (num_bytes % 2 == 0) {
    asm_.mov(ecx, num_bytes / 2);
    asm_.rep_movsw();
  } else {
    asm_.mov(ecx, num_bytes);
    asm_.rep_movsb();
  }
  asm_.pop(ecx);
}

void CompilerAsmjit::cmps(cell num_bytes) {
  // Compare memory blocks at [PRI] and [ALT]. The parameter
  // specifies the number of bytes. The blocks should not
  // overlap.
  Label above_label = asm_.newLabel();
  Label below_label = asm_.newLabel();
  Label equal_label = asm_.newLabel();
  Label continue_label = asm_.newLabel();
    asm_.cld();
    asm_.lea(edi, dword_ptr(ebx, eax));
    asm_.lea(esi, dword_ptr(ebx, ecx));
    asm_.push(ecx);
    asm_.mov(ecx, num_bytes);
    asm_.repe_cmpsb();
    asm_.pop(ecx);
    asm_.ja(above_label);
    asm_.jb(below_label);
    asm_.jz(equal_label);
  asm_.bind(above_label);
    asm_.mov(eax, 1);
    asm_.jmp(continue_label);
  asm_.bind(below_label);
    asm_.mov(eax, -1);
    asm_.jmp(continue_label);
  asm_.bind(equal_label);
    asm_.xor_(eax, eax);
  asm_.bind(continue_label);
}

void CompilerAsmjit::fill(cell num_bytes) {
  // Fill memory at [ALT] with value in [PRI]. The parameter
  // specifies the number of bytes, which must be a multiple
  // of the cell size.
  asm_.cld();
  asm_.lea(edi, dword_ptr(ebx, ecx));
  asm_.push(ecx);
  asm_.mov(ecx, num_bytes / sizeof(cell));
  asm_.rep_stosd();
  asm_.pop(ecx);
}

void CompilerAsmjit::halt(cell error_code) {
  // Abort execution (exit value in PRI), parameters other than 0
  // have a special meaning.
  asm_.mov(edx, error_code);
  asm_.jmp(halt_helper_label_);
}

void CompilerAsmjit::bounds(cell value) {
  // Abort execution if PRI > value or if PRI < 0.
  Label halt_label = asm_.newLabel();
  Label exit_label = asm_.newLabel();
    asm_.cmp(eax, value);
    asm_.jg(halt_label);
    asm_.test(eax, eax);
    asm_.jl(halt_label);
    asm_.jmp(exit_label);
  asm_.bind(halt_label);
    asm_.mov(edx, AMX_ERR_BOUNDS);
    asm_.jmp(halt_helper_label_);
  asm_.bind(exit_label);
}

void CompilerAsmjit::sysreq_pri() {
  // call system service, service number in PRI
  asm_.push(esp); // stackPtr
  asm_.push(ebp); // stackBase
  asm_.push(eax); // index
  asm_.call(sysreq_c_helper_label_);
}

void CompilerAsmjit::sysreq_c(cell index, const char *name) {
  // call system service
  if (name != 0) {
    if (!EmitIntrinsic(name)) {
      asm_.push(esp); // stackPtr
      asm_.push(ebp); // stackBase
      #if DEBUG
        asm_.push(index); // index
        asm_.call(sysreq_c_helper_label_);
      #else
        asm_.push(GetCurrentAmx().GetNativeAddress(index)); // address
        asm_.call(sysreq_d_helper_label_);
      #endif
    }
  }
}

void CompilerAsmjit::sysreq_d(cell address, const char *name) {
  // call system service
  if (name != 0) {
    if (!EmitIntrinsic(name)) {
      asm_.push(esp);     // stackPtr
      asm_.push(ebp);     // stackBase
      asm_.push(address); // address
      asm_.call(sysreq_d_helper_label_);
    }
  }
}

void CompilerAsmjit::switch_(const CaseTable &case_table) {
  // Compare PRI to the values in the case table (whose address
  // is passed as an offset from CIP) and jump to the associated
  // address in the matching record.

  if (case_table.num_cases() > 0) {
    // Get minimum and maximum values.
    cell min_value = case_table.FindMinValue();
    cell max_value = case_table.FindMaxValue();

    // Check if the value in eax is in the allowed range.
    // If not, jump to the default case (i.e. no match).
    asm_.cmp(eax, min_value);
    asm_.jl(GetLabel(case_table.GetDefaultAddress()));
    asm_.cmp(eax, max_value);
    asm_.jg(GetLabel(case_table.GetDefaultAddress()));

    // OK now sequentially compare eax with each value.
    // This is pretty slow so I probably should optimize
    // this in future...
    for (int i = 0; i < case_table.num_cases(); i++) {
      asm_.cmp(eax, case_table.GetCaseValue(i));
      asm_.je(GetLabel(case_table.GetCaseAddress(i)));
    }
  }

  // No match found - go for default case.
  asm_.jmp(GetLabel(case_table.GetDefaultAddress()));
}

void CompilerAsmjit::casetbl() {
  // A variable number of case records follows this opcode, where
  // each record takes two cells.
}

void CompilerAsmjit::swap_pri() {
  // [STK] = PRI and PRI = [STK]
  asm_.xchg(dword_ptr(esp), eax);
}

void CompilerAsmjit::swap_alt() {
  // [STK] = ALT and ALT = [STK]
  asm_.xchg(dword_ptr(esp), ecx);
}

void CompilerAsmjit::push_adr(cell offset) {
  // [STK] = FRM + offset, STK = STK - cell size
  asm_.lea(edx, dword_ptr(ebp, offset));
  asm_.sub(edx, ebx);
  asm_.push(edx);
}

void CompilerAsmjit::nop() {
  // no-operation, for code alignment
}

void CompilerAsmjit::break_() {
  // conditional breakpoint
}

void CompilerAsmjit::float_() {
  asm_.fild(dword_ptr(esp, 4));
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerAsmjit::floatabs() {
  asm_.fld(dword_ptr(esp, 4));
  asm_.fabs();
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerAsmjit::floatadd() {
  asm_.fld(dword_ptr(esp, 4));
  asm_.fadd(dword_ptr(esp, 8));
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerAsmjit::floatsub() {
  asm_.fld(dword_ptr(esp, 4));
  asm_.fsub(dword_ptr(esp, 8));
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerAsmjit::floatmul() {
  asm_.fld(dword_ptr(esp, 4));
  asm_.fmul(dword_ptr(esp, 8));
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerAsmjit::floatdiv() {
  asm_.fld(dword_ptr(esp, 4));
  asm_.fdiv(dword_ptr(esp, 8));
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerAsmjit::floatsqroot() {
  asm_.fld(dword_ptr(esp, 4));
  asm_.fsqrt();
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerAsmjit::floatlog() {
  asm_.fld1();
  asm_.fld(dword_ptr(esp, 8));
  asm_.fyl2x();
  asm_.fld1();
  asm_.fdivrp(st(1));
  asm_.fld(dword_ptr(esp, 4));
  asm_.fyl2x();
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

bool CompilerAsmjit::EmitIntrinsic(const char *name) {
  struct Intrinsic {
    const char         *name;
    EmitIntrinsicMethod emit;
  };
  
  static const Intrinsic intrinsics[] = {
    {"float",       &CompilerAsmjit::float_},
    {"floatabs",    &CompilerAsmjit::floatabs},
    {"floatadd",    &CompilerAsmjit::floatadd},
    {"floatsub",    &CompilerAsmjit::floatsub},
    {"floatmul",    &CompilerAsmjit::floatmul},
    {"floatdiv",    &CompilerAsmjit::floatdiv},
    {"floatsqroot", &CompilerAsmjit::floatsqroot},
    {"floatlog",    &CompilerAsmjit::floatlog}
  };

  for (std::size_t i = 0; i < sizeof(intrinsics) / sizeof(*intrinsics); i++) {
    if (std::strcmp(intrinsics[i].name, name) == 0) {
      (this->*intrinsics[i].emit)();
      return true;
    }
  }

  return false;
}

void CompilerAsmjit::EmitRuntimeInfo() {
  // This must have the same structure as RuntimeInfoBlock.
  asm_.bind(rib_start_label_);
  asm_.bind(exec_ptr_label_);
    asm_.dd(0);
  asm_.bind(amx_ptr_label_);
    asm_.dintptr(reinterpret_cast<intptr_t>(GetCurrentAmx().raw()));
  asm_.bind(ebp_ptr_label_);
    asm_.dd(0);
  asm_.bind(esp_ptr_label_);
    asm_.dd(0);
  asm_.bind(reset_ebp_ptr_label_);
    asm_.dd(0);
  asm_.bind(reset_esp_ptr_label_);
    asm_.dd(0);
  asm_.bind(instr_table_ptr_label_);
    asm_.dd(0);
  asm_.bind(instr_table_size_label_);
    asm_.dd(0);
}

void CompilerAsmjit::EmitInstrTable() {
  int num_entries = 0;

  Instruction instr;
  Disassembler disasm(GetCurrentAmx());
  while (disasm.Decode(instr)) {
    num_entries++;
  }

  RuntimeInfoBlock *rib = reinterpret_cast<RuntimeInfoBlock*>(asm_.getCode());
  rib->instr_table = asm_.getCodeSize();
  rib->instr_table_size = num_entries;

  InstrTableEntry dummy;
  for (int i = 0; i < num_entries; i++) {
    asm_.dstruct(dummy);
  }
}

// int AMXJIT_CDECL Exec(cell index, cell *retval);
void CompilerAsmjit::EmitExec() {
  RuntimeInfoBlock *rib = reinterpret_cast<RuntimeInfoBlock*>(asm_.getCode());
  rib->exec = asm_.getCodeSize();

  Label stack_heap_overflow_label = asm_.newLabel();
  Label heap_underflow_label = asm_.newLabel();
  Label stack_underflow_label = asm_.newLabel();
  Label native_not_found_label = asm_.newLabel();
  Label public_not_found_label = asm_.newLabel();
  Label finish_label = asm_.newLabel();
  Label return_label = asm_.newLabel();

  // Offsets of exec() arguments relative to ebp.
  int arg_index = 8;
  int arg_retval = 12;
  int var_address = -4;
  int var_reset_ebp = -8;
  int var_reset_esp = -12;

  asm_.bind(exec_label_);
    asm_.push(ebp);
    asm_.mov(ebp, esp);
    asm_.sub(esp, 12); // for locals

    asm_.push(esi);
    EmitAmxPtrMove(esi);

    // JIT code expects AMX data pointer to be in ebx.
    asm_.push(ebx);
    EmitAmxDataPtrMove(ebx);

    // Check for stack/heap collision (stack/heap overflow).
    asm_.mov(ecx, dword_ptr(esi, offsetof(AMX, hea)));
    asm_.mov(edx, dword_ptr(esi, offsetof(AMX, stk)));
    asm_.cmp(ecx, edx);
    asm_.jge(stack_heap_overflow_label);

    // Check for stack underflow.
    asm_.mov(ecx, dword_ptr(esi, offsetof(AMX, stk)));
    asm_.mov(edx, dword_ptr(esi, offsetof(AMX, stp)));
    asm_.cmp(ecx, edx);
    asm_.jg(stack_underflow_label);

    // Check for heap underflow.
    asm_.mov(ecx, dword_ptr(esi, offsetof(AMX, hea)));
    asm_.mov(edx, dword_ptr(esi, offsetof(AMX, hlw)));
    asm_.cmp(ecx, edx);
    asm_.jl(heap_underflow_label);

    // Make sure all natives are registered (the AMX_FLAG_NTVREG flag
    // must be set).
    asm_.mov(ecx, dword_ptr(esi, offsetof(AMX, flags)));
    asm_.test(ecx, AMX_FLAG_NTVREG);
    asm_.jz(native_not_found_label);

    // Reset the error code.
    asm_.mov(dword_ptr(esi, offsetof(AMX, error)), AMX_ERR_NONE);

    // Get address of the public function.
    asm_.push(dword_ptr(ebp, arg_index));
    EmitAmxPtrMove(eax);
    asm_.push(eax);
    asm_.call(asmjit_cast<void*>(&GetPublicAddress));
    asm_.add(esp, 8);

    // Check whether the function was found (address should be 0).
    asm_.test(eax, eax);
    asm_.jz(public_not_found_label);

    // Get pointer to the start of the function.
    asm_.lea(ecx, dword_ptr(rib_start_label_));
    asm_.push(ecx);
    asm_.push(eax);
    asm_.call(asmjit_cast<void*>(&GetInstrStartPtr));
    asm_.add(esp, 8);
    asm_.mov(dword_ptr(ebp, var_address), eax);

    // Push size of arguments and reset parameter count.
    // Pseudo code:
    //   stk = amx->stk - sizeof(cell);
    //   *(data + stk) = amx->paramcount;
    //   amx->stk = stk;
    //   amx->paramcount = 0;
    asm_.mov(eax, dword_ptr(esi, offsetof(AMX, paramcount)));
    asm_.imul(eax, eax, sizeof(cell));
    asm_.mov(ecx, dword_ptr(esi, offsetof(AMX, stk)));
    asm_.sub(ecx, sizeof(cell));
    asm_.mov(dword_ptr(ebx, ecx), eax);
    asm_.mov(dword_ptr(esi, offsetof(AMX, stk)), ecx);
    asm_.mov(dword_ptr(esi, offsetof(AMX, paramcount)), 0);

    // Keep a copy of the old resetEbp and resetEsp on the stack.
    asm_.mov(eax, dword_ptr(reset_ebp_ptr_label_));
    asm_.mov(dword_ptr(ebp, var_reset_ebp), eax);
    asm_.mov(eax, dword_ptr(reset_esp_ptr_label_));
    asm_.mov(dword_ptr(ebp, var_reset_esp), eax);

    // Call the function.
    asm_.push(dword_ptr(ebp, var_address));
    asm_.call(exec_helper_label_);
    asm_.add(esp, 4);

    // Copy return value to retval if it's not null.
    asm_.mov(ecx, dword_ptr(ebp, arg_retval));
    asm_.test(ecx, ecx);
    asm_.jz(finish_label);
    asm_.mov(dword_ptr(ecx), eax);

  asm_.bind(finish_label);
    // Restore resetEbp and resetEsp (remember they are kept in locals?).
    asm_.mov(eax, dword_ptr(ebp, var_reset_ebp));
    asm_.mov(dword_ptr(reset_ebp_ptr_label_), eax);
    asm_.mov(eax, dword_ptr(ebp, var_reset_esp));
    asm_.mov(dword_ptr(reset_esp_ptr_label_), eax);

    // Copy amx->error for return and reset it.
    asm_.mov(eax, AMX_ERR_NONE);
    asm_.xchg(eax, dword_ptr(esi, offsetof(AMX, error)));

  asm_.bind(return_label);
    asm_.pop(ebx);
    asm_.pop(esi);
    asm_.mov(esp, ebp);
    asm_.pop(ebp);
    asm_.ret();

  asm_.bind(stack_heap_overflow_label);
    asm_.mov(eax, AMX_ERR_STACKERR);
    asm_.jmp(return_label);

  asm_.bind(heap_underflow_label);
    asm_.mov(eax, AMX_ERR_HEAPLOW);
    asm_.jmp(return_label);

  asm_.bind(stack_underflow_label);
    asm_.mov(eax, AMX_ERR_STACKLOW);
    asm_.jmp(return_label);

  asm_.bind(native_not_found_label);
    asm_.mov(eax, AMX_ERR_NOTFOUND);
    asm_.jmp(return_label);

  asm_.bind(public_not_found_label);
    asm_.mov(eax, AMX_ERR_INDEX);
    asm_.jmp(return_label);
}

// cell AMXJIT_CDECL ExecHelper(void *address);
void CompilerAsmjit::EmitExecHelper() {
  asm_.bind(exec_helper_label_);
    // Store function address in eax.
    asm_.mov(eax, dword_ptr(esp, 4));

    // esi and edi are not saved across function bounds but generally
    // can be utilized in JIT code (for instance, in MOVS).
    asm_.push(esi);
    asm_.push(edi);

    // In JIT code these are caller-saved registers:
    //  eax - primary register (PRI)
    //  ecx - alternate register (ALT)
    //  ebx - data base pointer (DAT + amx->base)
    //  edx - temporary storage
    asm_.push(ebx);
    asm_.push(ecx);
    asm_.push(edx);

    // Store old ebp and esp on the stack.
    asm_.push(dword_ptr(ebp_ptr_label_));
    asm_.push(dword_ptr(esp_ptr_label_));

    // Most recent ebp and esp are stored in member variables.
    asm_.mov(dword_ptr(ebp_ptr_label_), ebp);
    asm_.mov(dword_ptr(esp_ptr_label_), esp);

    // Switch from native stack to AMX stack.
    EmitAmxPtrMove(ecx);
    asm_.mov(edx, dword_ptr(ecx, offsetof(AMX, frm)));
    asm_.lea(ebp, dword_ptr(ebx, edx)); // ebp = data + amx->frm
    asm_.mov(edx, dword_ptr(ecx, offsetof(AMX, stk)));
    asm_.lea(esp, dword_ptr(ebx, edx)); // esp = data + amx->stk

    // In order to make halt() work we have to be able to return to this
    // point somehow. The easiest way it to set the stack registers as
    // if we called the offending instruction directly from here.
    asm_.lea(ecx, dword_ptr(esp, - 4));
    asm_.mov(dword_ptr(reset_esp_ptr_label_), ecx);
    asm_.mov(dword_ptr(reset_ebp_ptr_label_), ebp);

    // Call the function. Prior to his point ebx should point to the
    // AMX data and the both stack pointers should point to somewhere
    // in the AMX stack.
    asm_.call(eax);

    // Keep AMX stack registers up-to-date. This wouldn't be necessary if
    // RETN didn't modify them (it pops all arguments off the stack).
    EmitAmxPtrMove(ecx);
    asm_.mov(edx, ebp);
    asm_.sub(edx, ebx);
    asm_.mov(dword_ptr(ecx, offsetof(AMX, frm)), edx); // amx->frm = ebp - data
    asm_.mov(edx, esp);
    asm_.sub(edx, ebx);
    asm_.mov(dword_ptr(ecx, offsetof(AMX, stk)), edx); // amx->stk = esp - data

    // Switch back to native stack.
    asm_.mov(ebp, dword_ptr(ebp_ptr_label_));
    asm_.mov(esp, dword_ptr(esp_ptr_label_));

    asm_.pop(dword_ptr(esp_ptr_label_));
    asm_.pop(dword_ptr(ebp_ptr_label_));

    asm_.pop(edx);
    asm_.pop(ecx);
    asm_.pop(ebx);
    asm_.pop(edi);
    asm_.pop(esi);

    asm_.ret();
}

// void HaltHelper(int error [edx]);
void CompilerAsmjit::EmitHaltHelper() {
  asm_.bind(halt_helper_label_);
    EmitAmxPtrMove(esi);
    asm_.mov(dword_ptr(esi, offsetof(AMX, error)), edx); // error code in edx

    // Reset stack so we can return right to call().
    asm_.mov(esp, dword_ptr(reset_esp_ptr_label_));
    asm_.mov(ebp, dword_ptr(reset_ebp_ptr_label_));

    // Pop public arguments as it would otherwise be done by RETN.
    asm_.pop(eax);
    asm_.add(esp, dword_ptr(esp));
    asm_.add(esp, 4);
    asm_.push(eax);

    asm_.ret();
}

// void JumpHelper(void *address [edx], void *stackBase [esi],
//                 void *stackPtr [edi]);
void CompilerAsmjit::EmitJumpHelper() {
  Label invalidAddressLabel = asm_.newLabel();

  asm_.bind(jump_helper_label_);
    asm_.push(eax);
    asm_.push(ecx);

    asm_.lea(ecx, dword_ptr(rib_start_label_));
    asm_.push(ecx);
    asm_.push(edx);
    asm_.call(asmjit_cast<void*>(&GetInstrStartPtr));
    asm_.add(esp, 8);
    asm_.mov(edx, eax); // address

    asm_.pop(ecx);
    asm_.pop(eax);

    asm_.test(edx, edx);
    asm_.jz(invalidAddressLabel);

    asm_.mov(ebp, esi);
    asm_.mov(esp, edi);
    asm_.jmp(edx);

  // Continue execution as if no jump was initiated (this is what AMX does).
  asm_.bind(invalidAddressLabel);
    asm_.ret();
}

// cell AMXJIT_CDECL SysreqCHelper(int index, void *stackBase, void *stackPtr);
void CompilerAsmjit::EmitSysreqCHelper() {
  Label native_not_found_label = asm_.newLabel();
  Label return_label = asm_.newLabel();

  int arg_index = 8;
  int arg_stack_base = 12;
  int arg_stack_ptr = 16;

  asm_.bind(sysreq_c_helper_label_);
    asm_.push(ebp);
    asm_.mov(ebp, esp);

    asm_.push(dword_ptr(ebp, arg_index));
    EmitAmxPtrMove(eax);
    asm_.push(eax);
    asm_.call(asmjit_cast<void*>(&GetNativeAddress));
    asm_.add(esp, 8);
    asm_.test(eax, eax);
    asm_.jz(native_not_found_label);

    asm_.push(dword_ptr(ebp, arg_stack_ptr));
    asm_.push(dword_ptr(ebp, arg_stack_base));
    asm_.push(eax); // address
    asm_.call(sysreq_d_helper_label_);
    asm_.add(esp, 12);

  asm_.bind(return_label);
    asm_.mov(esp, ebp);
    asm_.pop(ebp);
    asm_.ret();

  asm_.bind(native_not_found_label);
    asm_.mov(eax, AMX_ERR_NOTFOUND);
    asm_.jmp(return_label);
}

// cell AMXJIT_CDECL SysreqDHelper(void *address, void *stackBase, void *stackPtr);
void CompilerAsmjit::EmitSysreqDHelper() {
  asm_.bind(sysreq_d_helper_label_);
    asm_.mov(eax, dword_ptr(esp, 4));   // address
    asm_.mov(ebp, dword_ptr(esp, 8));   // stackBase
    asm_.mov(esp, dword_ptr(esp, 12));  // stackPtr
    asm_.mov(ecx, esp);                 // params
    asm_.mov(esi, dword_ptr(esp, -16)); // return address

    EmitAmxPtrMove(edx);

    // Switch to native stack.
    asm_.sub(ebp, ebx);
    asm_.mov(dword_ptr(edx, offsetof(AMX, frm)), ebp); // amx->frm = ebp - data
    asm_.mov(ebp, dword_ptr(ebp_ptr_label_));
    asm_.sub(esp, ebx);
    asm_.mov(dword_ptr(edx, offsetof(AMX, stk)), esp); // amx->stk = esp - data
    asm_.mov(esp, dword_ptr(esp_ptr_label_));

    // Call the native function.
    asm_.push(ecx); // params
    asm_.push(edx); // amx
    asm_.call(eax); // address
    asm_.add(esp, 8);

    // Switch back to AMX stack.
    EmitAmxPtrMove(edx);
    asm_.mov(dword_ptr(ebp_ptr_label_), ebp);
    asm_.mov(ecx, dword_ptr(edx, offsetof(AMX, frm)));
    asm_.lea(ebp, dword_ptr(ebx, ecx)); // ebp = data + amx->frm
    asm_.mov(dword_ptr(esp_ptr_label_), esp);
    asm_.mov(ecx, dword_ptr(edx, offsetof(AMX, stk)));
    asm_.lea(esp, dword_ptr(ebx, ecx)); // ebp = data + amx->stk

    // Modify the return address so we return next to the sysreq point.
    asm_.push(esi);
    asm_.ret();
}

void CompilerAsmjit::EmitAmxPtrMove(const GpReg &dest) {
  asm_.mov(dest, dword_ptr(amx_ptr_label_));
}

void CompilerAsmjit::EmitAmxDataPtrMove(const GpReg &dest) {
  Label exit_label = asm_.newLabel();

    EmitAmxPtrMove(eax);

    asm_.mov(dest, dword_ptr(eax, offsetof(AMX, data)));
    asm_.test(dest, dest);
    asm_.jnz(exit_label);

    asm_.mov(dest, dword_ptr(eax, offsetof(AMX, base)));
    asm_.mov(eax, dword_ptr(dest, offsetof(AMX_HEADER, dat)));
    asm_.add(dest, eax);

  asm_.bind(exit_label);
}

} // namespace amxjit
