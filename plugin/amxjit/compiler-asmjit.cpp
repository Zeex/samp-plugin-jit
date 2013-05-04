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
#include <string>
#include "compiler-asmjit.h"
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
  RuntimeDataInstrMapPtr
};

struct InstrMapEntry {
  cell  address;
  void *start;
};

class CompareInstrMapEntries
 : std::binary_function<const InstrMapEntry&, const InstrMapEntry&, bool> {
 public:
  bool operator()(const InstrMapEntry &lhs, const InstrMapEntry &rhs) const {
    return lhs.address < rhs.address;
  }
};

cell AMXJIT_CDECL GetPublicAddress(AMX *amx, int index) {
  return amxjit::AMXPtr(amx).GetPublicAddress(index);
}

cell AMXJIT_CDECL GetNativeAddress(AMX *amx, int index) {
  return amxjit::AMXPtr(amx).GetNativeAddress(index);
}

void *AMXJIT_CDECL GetInstrStartPtr(cell address, void *instrMap,
                               std::size_t instrMapSize) {
  assert(instrMap != 0);
  InstrMapEntry *begin = reinterpret_cast<InstrMapEntry*>(instrMap);
  InstrMapEntry target = {address, 0};

  std::pair<InstrMapEntry*, InstrMapEntry*> result;
  result = std::equal_range(begin, begin + instrMapSize, target,
                            CompareInstrMapEntries());

  if (result.first != result.second) {
    return result.first->start;
  }

  return 0;
}

} // anonymous namespace

namespace amxjit {

CompilerOutputAsmjit::CompilerOutputAsmjit(void *code, std::size_t codeSize)
 : code(code),
   codeSize(codeSize)
{
}

CompilerOutputAsmjit::~CompilerOutputAsmjit() {
  AsmJit::MemoryManager::getGlobal()->free(code);
}

CompilerAsmjit::CompilerAsmjit()
 : as(),
   execPtrLabel(as.newLabel()),
   amxPtrLabel(as.newLabel()),
   ebpPtrLabel(as.newLabel()),
   espPtrLabel(as.newLabel()),
   resetEbpPtrLabel(as.newLabel()),
   resetEspPtrLabel(as.newLabel()),
   instrMapPtrLabel(as.newLabel()),
   instrMapSizeLabel(as.newLabel()),
   execLabel(as.newLabel()),
   execHelperLabel(as.newLabel()),
   haltHelperLabel(as.newLabel()),
   jumpHelperLabel(as.newLabel()),
   sysreqCHelperLabel(as.newLabel()),
   sysreqDHelperLabel(as.newLabel()),
   breakHelperLabel(as.newLabel())
{
}

CompilerAsmjit::~CompilerAsmjit()
{
}

bool CompilerAsmjit::Setup() { 
  EmitRuntimeData();
  EmitInstrMap();
  EmitExec();
  EmitExecHelper();
  EmitHaltHelper();
  EmitJumpHelper();
  EmitSysreqCHelper();
  EmitSysreqDHelper();
  EmitBreakHelper();
  return true;
}

bool CompilerAsmjit::Process(const Instruction &instr) {
  cell cip = instr.GetAddress();

  as.bind(GetLabel(cip));
  instrMap[cip] = as.getCodeSize();

  // Align functions on 16-byte boundary.
  if (instr.GetOpcode().GetId() == OP_PROC) {
    as.align(16);
  }

  return true;
}

void CompilerAsmjit::Abort() {
  // do nothing
}

CompilerOutput *CompilerAsmjit::Finish() {
  intptr_t codePtr = reinterpret_cast<intptr_t>(as.make());
  size_t codeSize = as.getCodeSize();

  intptr_t *runtimeData = reinterpret_cast<intptr_t*>(codePtr);

  runtimeData[RuntimeDataExecPtr] += codePtr;
  runtimeData[RuntimeDataInstrMapPtr] += codePtr;

  intptr_t instrMapPtr = runtimeData[RuntimeDataInstrMapPtr];
  InstrMapEntry *entry = reinterpret_cast<InstrMapEntry*>(instrMapPtr);

  for (std::map<cell, std::ptrdiff_t>::const_iterator iterator = instrMap.begin();
       iterator != instrMap.end(); iterator++) {
    entry->address = iterator->first;
    entry->start = reinterpret_cast<void*>(iterator->second + codePtr);
    entry++;
  }

  return new CompilerOutputAsmjit(reinterpret_cast<void*>(codePtr), codeSize);
}

void CompilerAsmjit::load_pri(cell address) {
  // PRI = [address]
  as.mov(eax, dword_ptr(ebx, address));
}

void CompilerAsmjit::load_alt(cell address) {
  // ALT = [address]
  as.mov(ecx, dword_ptr(ebx, address));
}

void CompilerAsmjit::load_s_pri(cell offset) {
  // PRI = [FRM + offset]
  as.mov(eax, dword_ptr(ebp, offset));
}

void CompilerAsmjit::load_s_alt(cell offset) {
  // ALT = [FRM + offset]
  as.mov(ecx, dword_ptr(ebp, offset));
}

void CompilerAsmjit::lref_pri(cell address) {
  // PRI = [ [address] ]
  as.mov(edx, dword_ptr(ebx, address));
  as.mov(eax, dword_ptr(ebx, edx));
}

void CompilerAsmjit::lref_alt(cell address) {
  // ALT = [ [address] ]
  as.mov(edx, dword_ptr(ebx, + address));
  as.mov(ecx, dword_ptr(ebx, edx));
}

void CompilerAsmjit::lref_s_pri(cell offset) {
  // PRI = [ [FRM + offset] ]
  as.mov(edx, dword_ptr(ebp, offset));
  as.mov(eax, dword_ptr(ebx, edx));
}

void CompilerAsmjit::lref_s_alt(cell offset) {
  // PRI = [ [FRM + offset] ]
  as.mov(edx, dword_ptr(ebp, offset));
  as.mov(ecx, dword_ptr(ebx, edx));
}

void CompilerAsmjit::load_i() {
  // PRI = [PRI] (full cell)
  as.mov(eax, dword_ptr(ebx, eax));
}

void CompilerAsmjit::lodb_i(cell number) {
  // PRI = "number" bytes from [PRI] (read 1/2/4 bytes)
  switch (number) {
    case 1:
      as.movzx(eax, byte_ptr(ebx, eax));
      break;
    case 2:
      as.movzx(eax, word_ptr(ebx, eax));
      break;
    case 4:
      as.mov(eax, dword_ptr(ebx, eax));
      break;
  }
}

void CompilerAsmjit::const_pri(cell value) {
  // PRI = value
  if (value == 0) {
    as.xor_(eax, eax);
  } else {
    as.mov(eax, value);
  }
}

void CompilerAsmjit::const_alt(cell value) {
  // ALT = value
  if (value == 0) {
    as.xor_(ecx, ecx);
  } else {
    as.mov(ecx, value);
  }
}

void CompilerAsmjit::addr_pri(cell offset) {
  // PRI = FRM + offset
  as.lea(eax, dword_ptr(ebp, offset));
  as.sub(eax, ebx);
}

void CompilerAsmjit::addr_alt(cell offset) {
  // ALT = FRM + offset
  as.lea(ecx, dword_ptr(ebp, offset));
  as.sub(ecx, ebx);
}

void CompilerAsmjit::stor_pri(cell address) {
  // [address] = PRI
  as.mov(dword_ptr(ebx, address), eax);
}

void CompilerAsmjit::stor_alt(cell address) {
  // [address] = ALT
  as.mov(dword_ptr(ebx, address), ecx);
}

void CompilerAsmjit::stor_s_pri(cell offset) {
  // [FRM + offset] = ALT
  as.mov(dword_ptr(ebp, offset), eax);
}

void CompilerAsmjit::stor_s_alt(cell offset) {
  // [FRM + offset] = ALT
  as.mov(dword_ptr(ebp, offset), ecx);
}

void CompilerAsmjit::sref_pri(cell address) {
  // [ [address] ] = PRI
  as.mov(edx, dword_ptr(ebx, address));
  as.mov(dword_ptr(ebx, edx), eax);
}

void CompilerAsmjit::sref_alt(cell address) {
  // [ [address] ] = ALT
  as.mov(edx, dword_ptr(ebx, address));
  as.mov(dword_ptr(ebx, edx), ecx);
}

void CompilerAsmjit::sref_s_pri(cell offset) {
  // [ [FRM + offset] ] = PRI
  as.mov(edx, dword_ptr(ebp, offset));
  as.mov(dword_ptr(ebx, edx), eax);
}

void CompilerAsmjit::sref_s_alt(cell offset) {
  // [ [FRM + offset] ] = ALT
  as.mov(edx, dword_ptr(ebp, offset));
  as.mov(dword_ptr(ebx, edx), ecx);
}

void CompilerAsmjit::stor_i() {
  // [ALT] = PRI (full cell)
  as.mov(dword_ptr(ebx, ecx), eax);
}

void CompilerAsmjit::strb_i(cell number) {
  // "number" bytes at [ALT] = PRI (write 1/2/4 bytes)
  switch (number) {
    case 1:
      as.mov(byte_ptr(ebx, ecx), al);
      break;
    case 2:
      as.mov(word_ptr(ebx, ecx), ax);
      break;
    case 4:
      as.mov(dword_ptr(ebx, ecx), eax);
      break;
  }
}

void CompilerAsmjit::lidx() {
  // PRI = [ ALT + (PRI x cell size) ]
  as.lea(edx, dword_ptr(ebx, ecx));
  as.mov(eax, dword_ptr(edx, eax, 2));
}

void CompilerAsmjit::lidx_b(cell shift) {
  // PRI = [ ALT + (PRI << shift) ]
  as.lea(edx, dword_ptr(ebx, ecx));
  as.mov(eax, dword_ptr(edx, eax, shift));
}

void CompilerAsmjit::idxaddr() {
  // PRI = ALT + (PRI x cell size) (calculate indexed address)
  as.lea(eax, dword_ptr(ecx, eax, 2));
}

void CompilerAsmjit::idxaddr_b(cell shift) {
  // PRI = ALT + (PRI << shift) (calculate indexed address)
  as.lea(eax, dword_ptr(ecx, eax, shift));
}

void CompilerAsmjit::align_pri(cell number) {
  // Little Endian: PRI ^= cell size - number
  #if BYTE_ORDER == LITTLE_ENDIAN
    if (static_cast<std::size_t>(number) < sizeof(cell)) {
      as.xor_(eax, sizeof(cell) - number);
    }
  #endif
}

void CompilerAsmjit::align_alt(cell number) {
  // Little Endian: ALT ^= cell size - number
  #if BYTE_ORDER == LITTLE_ENDIAN
    if (static_cast<std::size_t>(number) < sizeof(cell)) {
      as.xor_(ecx, sizeof(cell) - number);
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
      EmitGetAmxPtr(eax);
      switch (index) {
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
      as.mov(eax, GetCurrentInstr().GetAddress() + GetCurrentInstr().GetSize());
      break;
    case 7:
      as.mov(eax, 1);
      break;
  }
}

void CompilerAsmjit::sctrl(cell index) {
  // set the indexed special registers to the value in PRI.
  // The index parameter must be:
  // 6=CIP
  switch (index) {
    case 2:
      EmitGetAmxPtr(edx);
      as.mov(dword_ptr(edx, offsetof(AMX, hea)), eax);
      break;
    case 4:
      as.lea(esp, dword_ptr(ebx, eax));
      break;
    case 5:
      as.lea(ebp, dword_ptr(ebx, eax));
      break;
    case 6:
      as.mov(edi, esp);
      as.mov(esi, ebp);
      as.mov(edx, eax);
      as.call(jumpHelperLabel);
      break;
  }
}

void CompilerAsmjit::move_pri() {
  // PRI = ALT
  as.mov(eax, ecx);
}

void CompilerAsmjit::move_alt() {
  // ALT = PRI
  as.mov(ecx, eax);
}

void CompilerAsmjit::xchg() {
  // Exchange PRI and ALT
  as.xchg(eax, ecx);
}

void CompilerAsmjit::push_pri() {
  // [STK] = PRI, STK = STK - cell size
  as.push(eax);
}

void CompilerAsmjit::push_alt() {
  // [STK] = ALT, STK = STK - cell size
  as.push(ecx);
}

void CompilerAsmjit::push_c(cell value) {
  // [STK] = value, STK = STK - cell size
  as.push(value);
}

void CompilerAsmjit::push(cell address) {
  // [STK] = [address], STK = STK - cell size
  as.push(dword_ptr(ebx, address));
}

void CompilerAsmjit::push_s(cell offset) {
  // [STK] = [FRM + offset], STK = STK - cell size
  as.push(dword_ptr(ebp, offset));
}

void CompilerAsmjit::pop_pri() {
  // STK = STK + cell size, PRI = [STK]
  as.pop(eax);
}

void CompilerAsmjit::pop_alt() {
  // STK = STK + cell size, ALT = [STK]
  as.pop(ecx);
}

void CompilerAsmjit::stack(cell value) {
  // ALT = STK, STK = STK + value
  as.mov(ecx, esp);
  as.sub(ecx, ebx);
  if (value >= 0) {
    as.add(esp, value);
  } else {
    as.sub(esp, -value);
  }
}

void CompilerAsmjit::heap(cell value) {
  // ALT = HEA, HEA = HEA + value
  EmitGetAmxPtr(edx);
  as.mov(ecx, dword_ptr(edx, offsetof(AMX, hea)));
  if (value >= 0) {
    as.add(dword_ptr(edx, offsetof(AMX, hea)), value);
  } else {
    as.sub(dword_ptr(edx, offsetof(AMX, hea)), -value);
  }
}

void CompilerAsmjit::proc() {
  // [STK] = FRM, STK = STK - cell size, FRM = STK
  as.push(ebp);
  as.mov(ebp, esp);
  as.sub(dword_ptr(esp), ebx);
}

void CompilerAsmjit::ret() {
  // STK = STK + cell size, FRM = [STK],
  // CIP = [STK], STK = STK + cell size
  as.pop(ebp);
  as.add(ebp, ebx);
  as.ret();
}

void CompilerAsmjit::retn() {
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

void CompilerAsmjit::call(cell address) {
  // [STK] = CIP + 5, STK = STK - cell size
  // CIP = CIP + offset
  // The CALL instruction jumps to an address after storing the
  // address of the next sequential instruction on the stack.
  // The address jumped to is relative to the current CIP,
  // but the address on the stack is an absolute address.
  as.call(GetLabel(address));
}

void CompilerAsmjit::jump_pri() {
  // CIP = PRI (indirect jump)
  as.mov(edi, esp);
  as.mov(esi, ebp);
  as.mov(edx, eax);
  as.call(jumpHelperLabel);
}

void CompilerAsmjit::jump(cell address) {
  // CIP = CIP + offset (jump to the address relative from
  // the current position)
  as.jmp(GetLabel(address));
}

void CompilerAsmjit::jzer(cell address) {
  // if PRI == 0 then CIP = CIP + offset
  as.test(eax, eax);
  as.jz(GetLabel(address));
}

void CompilerAsmjit::jnz(cell address) {
  // if PRI != 0 then CIP = CIP + offset
  as.test(eax, eax);
  as.jnz(GetLabel(address));
}

void CompilerAsmjit::jeq(cell address) {
  // if PRI == ALT then CIP = CIP + offset
  as.cmp(eax, ecx);
  as.je(GetLabel(address));
}

void CompilerAsmjit::jneq(cell address) {
  // if PRI != ALT then CIP = CIP + offset
  as.cmp(eax, ecx);
  as.jne(GetLabel(address));
}

void CompilerAsmjit::jless(cell address) {
  // if PRI < ALT then CIP = CIP + offset (unsigned)
  as.cmp(eax, ecx);
  as.jb(GetLabel(address));
}

void CompilerAsmjit::jleq(cell address) {
  // if PRI <= ALT then CIP = CIP + offset (unsigned)
  as.cmp(eax, ecx);
  as.jbe(GetLabel(address));
}

void CompilerAsmjit::jgrtr(cell address) {
  // if PRI > ALT then CIP = CIP + offset (unsigned)
  as.cmp(eax, ecx);
  as.ja(GetLabel(address));
}

void CompilerAsmjit::jgeq(cell address) {
  // if PRI >= ALT then CIP = CIP + offset (unsigned)
  as.cmp(eax, ecx);
  as.jae(GetLabel(address));
}

void CompilerAsmjit::jsless(cell address) {
  // if PRI < ALT then CIP = CIP + offset (signed)
  as.cmp(eax, ecx);
  as.jl(GetLabel(address));
}

void CompilerAsmjit::jsleq(cell address) {
  // if PRI <= ALT then CIP = CIP + offset (signed)
  as.cmp(eax, ecx);
  as.jle(GetLabel(address));
}

void CompilerAsmjit::jsgrtr(cell address) {
  // if PRI > ALT then CIP = CIP + offset (signed)
  as.cmp(eax, ecx);
  as.jg(GetLabel(address));
}

void CompilerAsmjit::jsgeq(cell address) {
  // if PRI >= ALT then CIP = CIP + offset (signed)
  as.cmp(eax, ecx);
  as.jge(GetLabel(address));
}

void CompilerAsmjit::shl() {
  // PRI = PRI << ALT
  as.shl(eax, cl);
}

void CompilerAsmjit::shr() {
  // PRI = PRI >> ALT (without sign extension)
  as.shr(eax, cl);
}

void CompilerAsmjit::sshr() {
  // PRI = PRI >> ALT with sign extension
  as.sar(eax, cl);
}

void CompilerAsmjit::shl_c_pri(cell value) {
  // PRI = PRI << value
  as.shl(eax, static_cast<unsigned char>(value));
}

void CompilerAsmjit::shl_c_alt(cell value) {
  // ALT = ALT << value
  as.shl(ecx, static_cast<unsigned char>(value));
}

void CompilerAsmjit::shr_c_pri(cell value) {
  // PRI = PRI >> value (without sign extension)
  as.shr(eax, static_cast<unsigned char>(value));
}

void CompilerAsmjit::shr_c_alt(cell value) {
  // PRI = PRI >> value (without sign extension)
  as.shl(ecx, static_cast<unsigned char>(value));
}

void CompilerAsmjit::smul() {
  // PRI = PRI * ALT (signed multiply)
  as.imul(ecx);
}

void CompilerAsmjit::sdiv() {
  // PRI = PRI / ALT (signed divide), ALT = PRI mod ALT
  as.cdq();
  as.idiv(ecx);
  as.mov(ecx, edx);
}

void CompilerAsmjit::sdiv_alt() {
  // PRI = ALT / PRI (signed divide), ALT = ALT mod PRI
  as.xchg(eax, ecx);
  as.cdq();
  as.idiv(ecx);
  as.mov(ecx, edx);
}

void CompilerAsmjit::umul() {
  // PRI = PRI * ALT (unsigned multiply)
  as.mul(ecx);
}

void CompilerAsmjit::udiv() {
  // PRI = PRI / ALT (unsigned divide), ALT = PRI mod ALT
  as.xor_(edx, edx);
  as.div(ecx);
  as.mov(ecx, edx);
}

void CompilerAsmjit::udiv_alt() {
  // PRI = ALT / PRI (unsigned divide), ALT = ALT mod PRI
  as.xchg(eax, ecx);
  as.xor_(edx, edx);
  as.div(ecx);
  as.mov(ecx, edx);
}

void CompilerAsmjit::add() {
  // PRI = PRI + ALT
  as.add(eax, ecx);
}

void CompilerAsmjit::sub() {
  // PRI = PRI - ALT
  as.sub(eax, ecx);
}

void CompilerAsmjit::sub_alt() {
  // PRI = ALT - PRI
  // or:
  // PRI = -(PRI - ALT)
  as.sub(eax, ecx);
  as.neg(eax);
}

void CompilerAsmjit::and_() {
  // PRI = PRI & ALT
  as.and_(eax, ecx);
}

void CompilerAsmjit::or_() {
  // PRI = PRI | ALT
  as.or_(eax, ecx);
}

void CompilerAsmjit::xor_() {
  // PRI = PRI ^ ALT
  as.xor_(eax, ecx);
}

void CompilerAsmjit::not_() {
  // PRI = !PRI
  as.test(eax, eax);
  as.setz(al);
  as.movzx(eax, al);
}

void CompilerAsmjit::neg() {
  // PRI = -PRI
  as.neg(eax);
}

void CompilerAsmjit::invert() {
  // PRI = ~PRI
  as.not_(eax);
}

void CompilerAsmjit::add_c(cell value) {
  // PRI = PRI + value
  if (value >= 0) {
    as.add(eax, value);
  } else {
    as.sub(eax, -value);
  }
}

void CompilerAsmjit::smul_c(cell value) {
  // PRI = PRI * value
  as.imul(eax, value);
}

void CompilerAsmjit::zero_pri() {
  // PRI = 0
  as.xor_(eax, eax);
}

void CompilerAsmjit::zero_alt() {
  // ALT = 0
  as.xor_(ecx, ecx);
}

void CompilerAsmjit::zero(cell address) {
  // [address] = 0
  as.mov(dword_ptr(ebx, address), 0);
}

void CompilerAsmjit::zero_s(cell offset) {
  // [FRM + offset] = 0
  as.mov(dword_ptr(ebp, offset), 0);
}

void CompilerAsmjit::sign_pri() {
  // sign extent the byte in PRI to a cell
  as.movsx(eax, al);
}

void CompilerAsmjit::sign_alt() {
  // sign extent the byte in ALT to a cell
  as.movsx(ecx, cl);
}

void CompilerAsmjit::eq() {
  // PRI = PRI == ALT ? 1 :
  as.cmp(eax, ecx);
  as.sete(al);
  as.movzx(eax, al);
}

void CompilerAsmjit::neq() {
  // PRI = PRI != ALT ? 1 :
  as.cmp(eax, ecx);
  as.setne(al);
  as.movzx(eax, al);
}

void CompilerAsmjit::less() {
  // PRI = PRI < ALT ? 1 :
  as.cmp(eax, ecx);
  as.setb(al);
  as.movzx(eax, al);
}

void CompilerAsmjit::leq() {
  // PRI = PRI <= ALT ? 1 :
  as.cmp(eax, ecx);
  as.setbe(al);
  as.movzx(eax, al);
}

void CompilerAsmjit::grtr() {
  // PRI = PRI > ALT ? 1 :
  as.cmp(eax, ecx);
  as.seta(al);
  as.movzx(eax, al);
}

void CompilerAsmjit::geq() {
  // PRI = PRI >= ALT ? 1 :
  as.cmp(eax, ecx);
  as.setae(al);
  as.movzx(eax, al);
}

void CompilerAsmjit::sless() {
  // PRI = PRI < ALT ? 1 :
  as.cmp(eax, ecx);
  as.setl(al);
  as.movzx(eax, al);
}

void CompilerAsmjit::sleq() {
  // PRI = PRI <= ALT ? 1 :
  as.cmp(eax, ecx);
  as.setle(al);
  as.movzx(eax, al);
}

void CompilerAsmjit::sgrtr() {
  // PRI = PRI > ALT ? 1 :
  as.cmp(eax, ecx);
  as.setg(al);
  as.movzx(eax, al);
}

void CompilerAsmjit::sgeq() {
  // PRI = PRI >= ALT ? 1 :
  as.cmp(eax, ecx);
  as.setge(al);
  as.movzx(eax, al);
}

void CompilerAsmjit::eq_c_pri(cell value) {
  // PRI = PRI == value ? 1 :
  as.cmp(eax, value);
  as.sete(al);
  as.movzx(eax, al);
}

void CompilerAsmjit::eq_c_alt(cell value) {
  // PRI = ALT == value ? 1 :
  as.cmp(ecx, value);
  as.sete(al);
  as.movzx(eax, al);
}

void CompilerAsmjit::inc_pri() {
  // PRI = PRI + 1
  as.inc(eax);
}

void CompilerAsmjit::inc_alt() {
  // ALT = ALT + 1
  as.inc(ecx);
}

void CompilerAsmjit::inc(cell address) {
  // [address] = [address] + 1
  as.inc(dword_ptr(ebx, address));
}

void CompilerAsmjit::inc_s(cell offset) {
  // [FRM + offset] = [FRM + offset] + 1
  as.inc(dword_ptr(ebp, offset));
}

void CompilerAsmjit::inc_i() {
  // [PRI] = [PRI] + 1
  as.inc(dword_ptr(ebx, eax));
}

void CompilerAsmjit::dec_pri() {
  // PRI = PRI - 1
  as.dec(eax);
}

void CompilerAsmjit::dec_alt() {
  // ALT = ALT - 1
  as.dec(ecx);
}

void CompilerAsmjit::dec(cell address) {
  // [address] = [address] - 1
  as.dec(dword_ptr(ebx, address));
}

void CompilerAsmjit::dec_s(cell offset) {
  // [FRM + offset] = [FRM + offset] - 1
  as.dec(dword_ptr(ebp, offset));
}

void CompilerAsmjit::dec_i() {
  // [PRI] = [PRI] - 1
  as.dec(dword_ptr(ebx, eax));
}

void CompilerAsmjit::movs(cell numBytes) {
  // Copy memory from [PRI] to [ALT]. The parameter
  // specifies the number of bytes. The blocks should not
  // overlap.
  as.cld();
  as.lea(esi, dword_ptr(ebx, eax));
  as.lea(edi, dword_ptr(ebx, ecx));
  as.push(ecx);
  if (numBytes % 4 == 0) {
    as.mov(ecx, numBytes / 4);
    as.rep_movsd();
  } else if (numBytes % 2 == 0) {
    as.mov(ecx, numBytes / 2);
    as.rep_movsw();
  } else {
    as.mov(ecx, numBytes);
    as.rep_movsb();
  }
  as.pop(ecx);
}

void CompilerAsmjit::cmps(cell numBytes) {
  // Compare memory blocks at [PRI] and [ALT]. The parameter
  // specifies the number of bytes. The blocks should not
  // overlap.
  Label aboveLabel = as.newLabel();
  Label belowLabel = as.newLabel();
  Label equalLabel = as.newLabel();
  Label continueLabel = as.newLabel();
    as.cld();
    as.lea(edi, dword_ptr(ebx, eax));
    as.lea(esi, dword_ptr(ebx, ecx));
    as.push(ecx);
    as.mov(ecx, numBytes);
    as.repe_cmpsb();
    as.pop(ecx);
    as.ja(aboveLabel);
    as.jb(belowLabel);
    as.jz(equalLabel);
  as.bind(aboveLabel);
    as.mov(eax, 1);
    as.jmp(continueLabel);
  as.bind(belowLabel);
    as.mov(eax, -1);
    as.jmp(continueLabel);
  as.bind(equalLabel);
    as.xor_(eax, eax);
  as.bind(continueLabel);
}

void CompilerAsmjit::fill(cell numBytes) {
  // Fill memory at [ALT] with value in [PRI]. The parameter
  // specifies the number of bytes, which must be a multiple
  // of the cell size.
  as.cld();
  as.lea(edi, dword_ptr(ebx, ecx));
  as.push(ecx);
  as.mov(ecx, numBytes / sizeof(cell));
  as.rep_stosd();
  as.pop(ecx);
}

void CompilerAsmjit::halt(cell errorCode) {
  // Abort execution (exit value in PRI), parameters other than 0
  // have a special meaning.
  as.mov(edx, errorCode);
  as.jmp(haltHelperLabel);
}

void CompilerAsmjit::bounds(cell value) {
  // Abort execution if PRI > value or if PRI < 0.
  Label haltLabel = as.newLabel();
  Label exitLabel = as.newLabel();

    as.cmp(eax, value);
    as.jg(haltLabel);

    as.test(eax, eax);
    as.jl(haltLabel);

    as.jmp(exitLabel);

  as.bind(haltLabel);
    as.mov(edx, AMX_ERR_BOUNDS);
    as.jmp(haltHelperLabel);

  as.bind(exitLabel);
}

void CompilerAsmjit::sysreq_pri() {
  // call system service, service number in PRI
  as.push(esp); // stackPtr
  as.push(ebp); // stackBase
  as.push(eax); // index
  as.call(sysreqCHelperLabel);
}

void CompilerAsmjit::sysreq_c(cell index, const char *name) {
  // call system service
  if (name != 0) {
    if (!EmitIntrinsic(name)) {
      as.push(esp); // stackPtr
      as.push(ebp); // stackBase
      #if DEBUG
        as.push(index); // index
        as.call(sysreqCHelperLabel);
      #else
        as.push(GetCurrentAmx().GetNativeAddress(index)); // address
        as.call(sysreqDHelperLabel);
      #endif
    }
  }
}

void CompilerAsmjit::sysreq_d(cell address, const char *name) {
  // call system service
  if (name != 0) {
    if (!EmitIntrinsic(name)) {
      as.push(esp);     // stackPtr
      as.push(ebp);     // stackBase
      as.push(address); // address
      as.call(sysreqDHelperLabel);
    }
  }
}

void CompilerAsmjit::switch_(const CaseTable &caseTable) {
  // Compare PRI to the values in the case table (whose address
  // is passed as an offset from CIP) and jump to the associated
  // address in the matching record.

  if (caseTable.GetNumCases() > 0) {
    // Get minimum and maximum values.
    cell minValue = caseTable.FindMinValue();
    cell maxValue = caseTable.FindMaxValue();

    // Check if the value in eax is in the allowed range.
    // If not, jump to the default case (i.e. no match).
    as.cmp(eax, minValue);
    as.jl(GetLabel(caseTable.GetDefaultAddress()));
    as.cmp(eax, maxValue);
    as.jg(GetLabel(caseTable.GetDefaultAddress()));

    // OK now sequentially compare eax with each value.
    // This is pretty slow so I probably should optimize
    // this in future...
    for (int i = 0; i < caseTable.GetNumCases(); i++) {
      as.cmp(eax, caseTable.GetValue(i));
      as.je(GetLabel(caseTable.GetAddress(i)));
    }
  }

  // No match found - go for default case.
  as.jmp(GetLabel(caseTable.GetDefaultAddress()));
}

void CompilerAsmjit::casetbl() {
  // A variable number of case records follows this opcode, where
  // each record takes two cells.
}

void CompilerAsmjit::swap_pri() {
  // [STK] = PRI and PRI = [STK]
  as.xchg(dword_ptr(esp), eax);
}

void CompilerAsmjit::swap_alt() {
  // [STK] = ALT and ALT = [STK]
  as.xchg(dword_ptr(esp), ecx);
}

void CompilerAsmjit::push_adr(cell offset) {
  // [STK] = FRM + offset, STK = STK - cell size
  as.lea(edx, dword_ptr(ebp, offset));
  as.sub(edx, ebx);
  as.push(edx);
}

void CompilerAsmjit::nop() {
  // no-operation, for code alignment
}

void CompilerAsmjit::break_() {
  // conditional breakpoint
  #ifdef DEBUG
    as.call(breakHelperLabel);
  #endif
}

void CompilerAsmjit::float_() {
  as.fild(dword_ptr(esp, 4));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void CompilerAsmjit::floatabs() {
  as.fld(dword_ptr(esp, 4));
  as.fabs();
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void CompilerAsmjit::floatadd() {
  as.fld(dword_ptr(esp, 4));
  as.fadd(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void CompilerAsmjit::floatsub() {
  as.fld(dword_ptr(esp, 4));
  as.fsub(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void CompilerAsmjit::floatmul() {
  as.fld(dword_ptr(esp, 4));
  as.fmul(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void CompilerAsmjit::floatdiv() {
  as.fld(dword_ptr(esp, 4));
  as.fdiv(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void CompilerAsmjit::floatsqroot() {
  as.fld(dword_ptr(esp, 4));
  as.fsqrt();
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}

void CompilerAsmjit::floatlog() {
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

bool CompilerAsmjit::EmitIntrinsic(const char *name) {
  struct Intrinsic {
    const char          *name;
    EmitIntrinsicMethod  emit;
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

void CompilerAsmjit::EmitRuntimeData() {
  as.bind(execPtrLabel);
    as.dd(0);
  as.bind(amxPtrLabel);
    as.dintptr(reinterpret_cast<intptr_t>(GetCurrentAmx().GetStruct()));
  as.bind(ebpPtrLabel);
    as.dd(0);
  as.bind(espPtrLabel);
    as.dd(0);
  as.bind(resetEbpPtrLabel);
    as.dd(0);
  as.bind(resetEspPtrLabel);
    as.dd(0);
  as.bind(instrMapSizeLabel);
    as.dd(0);
  as.bind(instrMapPtrLabel);
    as.dd(0);
}

void CompilerAsmjit::EmitInstrMap() {
  Disassembler disas(GetCurrentAmx());
  Instruction instr;
  int size = 0;
  while (disas.Decode(instr)) {
    size++;
  }

  SetRuntimeData(RuntimeDataInstrMapSize, size);
  SetRuntimeData(RuntimeDataInstrMapPtr, as.getCodeSize());

  InstrMapEntry dummy = {0};
  for (int i = 0; i < size; i++) {
    as.dstruct(dummy);
  }
}

// int AMXJIT_CDECL Exec(cell index, cell *retval);
void CompilerAsmjit::EmitExec() {
  SetRuntimeData(RuntimeDataExecPtr, as.getCodeSize());

  Label stackHepOverrflowLabel = as.newLabel();
  Label heapUnderflowLabel = as.newLabel();
  Label stackUnderflowLabel = as.newLabel();
  Label nativeNotFoundLabel = as.newLabel();
  Label publicNotFoundLabel = as.newLabel();
  Label finishLabel = as.newLabel();
  Label returnLabel = as.newLabel();

  // Offsets of exec() arguments relative to ebp.
  int argIndex = 8;
  int argRetval = 12;
  int varAddress = -4;
  int varResetEbp = -8;
  int varResetEsp = -12;

  as.bind(execLabel);
    as.push(ebp);
    as.mov(ebp, esp);
    as.sub(esp, 12); // for locals

    as.push(esi);
    EmitGetAmxPtr(esi);

    // JIT code expects AMX data pointer to be in ebx.
    as.push(ebx);
    EmitGetAmxDataPtr(ebx);

    // Check for stack/heap collision (stack/heap overflow).
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, hea)));
    as.mov(edx, dword_ptr(esi, offsetof(AMX, stk)));
    as.cmp(ecx, edx);
    as.jge(stackHepOverrflowLabel);

    // Check for stack underflow.
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, stk)));
    as.mov(edx, dword_ptr(esi, offsetof(AMX, stp)));
    as.cmp(ecx, edx);
    as.jg(stackUnderflowLabel);

    // Check for heap underflow.
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, hea)));
    as.mov(edx, dword_ptr(esi, offsetof(AMX, hlw)));
    as.cmp(ecx, edx);
    as.jl(heapUnderflowLabel);

    // Make sure all natives are registered (the AMX_FLAG_NTVREG flag
    // must be set).
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, flags)));
    as.test(ecx, AMX_FLAG_NTVREG);
    as.jz(nativeNotFoundLabel);

    // Reset the error code.
    as.mov(dword_ptr(esi, offsetof(AMX, error)), AMX_ERR_NONE);

    // Get address of the public function.
    as.push(dword_ptr(ebp, argIndex));
    EmitGetAmxPtr(eax);
    as.push(eax);
    as.call(asmjit_cast<void*>(&GetPublicAddress));
    as.add(esp, 8);

    // Check whether the function was found (address should be 0).
    as.test(eax, eax);
    as.jz(publicNotFoundLabel);

    // Get pointer to the start of the function.
    as.push(dword_ptr(instrMapSizeLabel));
    as.push(dword_ptr(instrMapPtrLabel));
    as.push(eax);
    as.call(asmjit_cast<void*>(&GetInstrStartPtr));
    as.add(esp, 12);
    as.mov(dword_ptr(ebp, varAddress), eax);

    // Push size of arguments and reset parameter count.
    // Pseudo code:
    //   stk = amx->stk - sizeof(cell);
    //   *(data + stk) = amx->paramcount;
    //   amx->stk = stk;
    //   amx->paramcount = 0;
    as.mov(eax, dword_ptr(esi, offsetof(AMX, paramcount)));
    as.imul(eax, eax, sizeof(cell));
    as.mov(ecx, dword_ptr(esi, offsetof(AMX, stk)));
    as.sub(ecx, sizeof(cell));
    as.mov(dword_ptr(ebx, ecx), eax);
    as.mov(dword_ptr(esi, offsetof(AMX, stk)), ecx);
    as.mov(dword_ptr(esi, offsetof(AMX, paramcount)), 0);

    // Keep a copy of the old resetEbp and resetEsp on the stack.
    as.mov(eax, dword_ptr(resetEbpPtrLabel));
    as.mov(dword_ptr(ebp, varResetEbp), eax);
    as.mov(eax, dword_ptr(resetEspPtrLabel));
    as.mov(dword_ptr(ebp, varResetEsp), eax);

    // Call the function.
    as.push(dword_ptr(ebp, varAddress));
    as.call(execHelperLabel);
    as.add(esp, 4);

    // Copy return value to retval if it's not null.
    as.mov(ecx, dword_ptr(ebp, argRetval));
    as.test(ecx, ecx);
    as.jz(finishLabel);
    as.mov(dword_ptr(ecx), eax);

  as.bind(finishLabel);
    // Restore resetEbp and resetEsp (remember they are kept in locals?).
    as.mov(eax, dword_ptr(ebp, varResetEbp));
    as.mov(dword_ptr(resetEbpPtrLabel), eax);
    as.mov(eax, dword_ptr(ebp, varResetEsp));
    as.mov(dword_ptr(resetEspPtrLabel), eax);

    // Copy amx->error for return and reset it.
    as.mov(eax, AMX_ERR_NONE);
    as.xchg(eax, dword_ptr(esi, offsetof(AMX, error)));

  as.bind(returnLabel);
    as.pop(ebx);
    as.pop(esi);
    as.mov(esp, ebp);
    as.pop(ebp);
    as.ret();

  as.bind(stackHepOverrflowLabel);
    as.mov(eax, AMX_ERR_STACKERR);
    as.jmp(returnLabel);

  as.bind(heapUnderflowLabel);
    as.mov(eax, AMX_ERR_HEAPLOW);
    as.jmp(returnLabel);

  as.bind(stackUnderflowLabel);
    as.mov(eax, AMX_ERR_STACKLOW);
    as.jmp(returnLabel);

  as.bind(nativeNotFoundLabel);
    as.mov(eax, AMX_ERR_NOTFOUND);
    as.jmp(returnLabel);

  as.bind(publicNotFoundLabel);
    as.mov(eax, AMX_ERR_INDEX);
    as.jmp(returnLabel);
}

// cell AMXJIT_CDECL ExecHelper(void *address);
void CompilerAsmjit::EmitExecHelper() {
  as.bind(execHelperLabel);
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
    as.push(dword_ptr(ebpPtrLabel));
    as.push(dword_ptr(espPtrLabel));

    // Most recent ebp and esp are stored in member variables.
    as.mov(dword_ptr(ebpPtrLabel), ebp);
    as.mov(dword_ptr(espPtrLabel), esp);

    // Switch from native stack to AMX stack.
    EmitGetAmxPtr(ecx);
    as.mov(edx, dword_ptr(ecx, offsetof(AMX, frm)));
    as.lea(ebp, dword_ptr(ebx, edx)); // ebp = data + amx->frm
    as.mov(edx, dword_ptr(ecx, offsetof(AMX, stk)));
    as.lea(esp, dword_ptr(ebx, edx)); // esp = data + amx->stk

    // In order to make halt() work we have to be able to return to this
    // point somehow. The easiest way it to set the stack registers as
    // if we called the offending instruction directly from here.
    as.lea(ecx, dword_ptr(esp, - 4));
    as.mov(dword_ptr(resetEspPtrLabel), ecx);
    as.mov(dword_ptr(resetEbpPtrLabel), ebp);

    // Call the function. Prior to his point ebx should point to the
    // AMX data and the both stack pointers should point to somewhere
    // in the AMX stack.
    as.call(eax);

    // Keep AMX stack registers up-to-date. This wouldn't be necessary if
    // RETN didn't modify them (it pops all arguments off the stack).
    EmitGetAmxPtr(ecx);
    as.mov(edx, ebp);
    as.sub(edx, ebx);
    as.mov(dword_ptr(ecx, offsetof(AMX, frm)), edx); // amx->frm = ebp - data
    as.mov(edx, esp);
    as.sub(edx, ebx);
    as.mov(dword_ptr(ecx, offsetof(AMX, stk)), edx); // amx->stk = esp - data

    // Switch back to native stack.
    as.mov(ebp, dword_ptr(ebpPtrLabel));
    as.mov(esp, dword_ptr(espPtrLabel));

    as.pop(dword_ptr(espPtrLabel));
    as.pop(dword_ptr(ebpPtrLabel));

    as.pop(edx);
    as.pop(ecx);
    as.pop(ebx);
    as.pop(edi);
    as.pop(esi);

    as.ret();
}

// void HaltHelper(int error [edx]);
void CompilerAsmjit::EmitHaltHelper() {
  as.bind(haltHelperLabel);
    EmitGetAmxPtr(esi);
    as.mov(dword_ptr(esi, offsetof(AMX, error)), edx); // error code in edx

    // Reset stack so we can return right to call().
    as.mov(esp, dword_ptr(resetEspPtrLabel));
    as.mov(ebp, dword_ptr(resetEbpPtrLabel));

    // Pop public arguments as it would otherwise be done by RETN.
    as.pop(eax);
    as.add(esp, dword_ptr(esp));
    as.add(esp, 4);
    as.push(eax);

    as.ret();
}

// void JumpHelper(void *address [edx], void *stackBase [esi],
//                 void *stackPtr [edi]);
void CompilerAsmjit::EmitJumpHelper() {
  Label invalidAddressLabel = as.newLabel();

  as.bind(jumpHelperLabel);
    as.push(eax);
    as.push(ecx);

    as.push(dword_ptr(instrMapSizeLabel));
    as.push(dword_ptr(instrMapPtrLabel));
    as.push(edx);
    as.call(asmjit_cast<void*>(&GetInstrStartPtr));
    as.add(esp, 12);
    as.mov(edx, eax); // address

    as.pop(ecx);
    as.pop(eax);

    as.test(edx, edx);
    as.jz(invalidAddressLabel);

    as.mov(ebp, esi);
    as.mov(esp, edi);
    as.jmp(edx);

  // Continue execution as if no jump was initiated (this is what AMX does).
  as.bind(invalidAddressLabel);
    as.ret();
}

// cell AMXJIT_CDECL SysreqCHelper(int index, void *stackBase, void *stackPtr);
void CompilerAsmjit::EmitSysreqCHelper() {
  Label nativeNotFoundLabel = as.newLabel();
  Label returnLabel = as.newLabel();

  int argIndex = 8;
  int argStackbase = 12;
  int argStackPtr = 16;

  as.bind(sysreqCHelperLabel);
    as.push(ebp);
    as.mov(ebp, esp);

    as.push(dword_ptr(ebp, argIndex));
    EmitGetAmxPtr(eax);
    as.push(eax);
    as.call(asmjit_cast<void*>(&GetNativeAddress));
    as.add(esp, 8);
    as.test(eax, eax);
    as.jz(nativeNotFoundLabel);

    as.push(dword_ptr(ebp, argStackPtr));
    as.push(dword_ptr(ebp, argStackbase));
    as.push(eax); // address
    as.call(sysreqDHelperLabel);
    as.add(esp, 12);

  as.bind(returnLabel);
    as.mov(esp, ebp);
    as.pop(ebp);
    as.ret();

  as.bind(nativeNotFoundLabel);
    as.mov(eax, AMX_ERR_NOTFOUND);
    as.jmp(returnLabel);
}

// cell AMXJIT_CDECL SysreqDHelper(void *address, void *stackBase, void *stackPtr);
void CompilerAsmjit::EmitSysreqDHelper() {
  as.bind(sysreqDHelperLabel);
    as.mov(eax, dword_ptr(esp, 4));   // address
    as.mov(ebp, dword_ptr(esp, 8));   // stackBase
    as.mov(esp, dword_ptr(esp, 12));  // stackPtr
    as.mov(ecx, esp);                 // params
    as.mov(esi, dword_ptr(esp, -16)); // return address

    EmitGetAmxPtr(edx);

    // Switch to native stack.
    as.sub(ebp, ebx);
    as.mov(dword_ptr(edx, offsetof(AMX, frm)), ebp); // amx->frm = ebp - data
    as.mov(ebp, dword_ptr(ebpPtrLabel));
    as.sub(esp, ebx);
    as.mov(dword_ptr(edx, offsetof(AMX, stk)), esp); // amx->stk = esp - data
    as.mov(esp, dword_ptr(espPtrLabel));

    // Call the native function.
    as.push(ecx); // params
    as.push(edx); // amx
    as.call(eax); // address
    as.add(esp, 8);

    // Switch back to AMX stack.
    EmitGetAmxPtr(edx);
    as.mov(dword_ptr(ebpPtrLabel), ebp);
    as.mov(ecx, dword_ptr(edx, offsetof(AMX, frm)));
    as.lea(ebp, dword_ptr(ebx, ecx)); // ebp = data + amx->frm
    as.mov(dword_ptr(espPtrLabel), esp);
    as.mov(ecx, dword_ptr(edx, offsetof(AMX, stk)));
    as.lea(esp, dword_ptr(ebx, ecx)); // ebp = data + amx->stk

    // Modify the return address so we return next to the sysreq point.
    as.push(esi);
    as.ret();
}

// void BreakHelper();
void CompilerAsmjit::EmitBreakHelper() {
  Label returnLabel = as.newLabel();

  as.bind(breakHelperLabel);
    EmitGetAmxPtr(edx);
    as.mov(esi, dword_ptr(edx, offsetof(AMX, debug)));
    as.test(esi, esi);
    as.jz(returnLabel);

    as.push(eax);
    as.push(ecx);

    as.push(edx);
    as.call(esi);
    as.add(esp, 4);

    as.pop(ecx);
    as.pop(eax);

  as.bind(returnLabel);
    as.ret();
}

void CompilerAsmjit::EmitGetAmxPtr(const GpReg &reg) {
  as.mov(reg, dword_ptr(amxPtrLabel));
}

void CompilerAsmjit::EmitGetAmxDataPtr(const GpReg &reg) {
  Label exitLabel = as.newLabel();

    EmitGetAmxPtr(eax);

    as.mov(reg, dword_ptr(eax, offsetof(AMX, data)));
    as.test(reg, reg);
    as.jnz(exitLabel);

    as.mov(reg, dword_ptr(eax, offsetof(AMX, base)));
    as.mov(eax, dword_ptr(reg, offsetof(AMX_HEADER, dat)));
    as.add(reg, eax);

  as.bind(exitLabel);
}

} // namespace amxjit
