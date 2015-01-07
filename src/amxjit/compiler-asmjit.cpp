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
#include "logger.h"

using asmjit::Label;
using asmjit::x86::byte_ptr;
using asmjit::x86::word_ptr;
using asmjit::x86::dword_ptr;
using asmjit::x86::dword_ptr_abs;
using asmjit::x86::al;
using asmjit::x86::cl;
using asmjit::x86::ax;
using asmjit::x86::cx;
using asmjit::x86::eax;
using asmjit::x86::ebx;
using asmjit::x86::ecx;
using asmjit::x86::edx;
using asmjit::x86::esi;
using asmjit::x86::edi;
using asmjit::x86::ebp;
using asmjit::x86::esp;
using asmjit::x86::fp1;

namespace amxjit {
namespace {

asmjit::JitRuntime jit_runtime;

struct RuntimeInfoBlock {
  intptr_t exec;
  intptr_t amx;
  intptr_t ebp;
  intptr_t esp;
  intptr_t reset_ebp;
  intptr_t reset_esp;
  intptr_t instr_table;
  intptr_t instr_table_size;
};

cell AMXJIT_CDECL GetPublicAddress(AMX *amx, int index) {
  return AMXPtr(amx).GetPublicAddress(index);
}

cell AMXJIT_CDECL GetNativeAddress(AMX *amx, int index) {
  return AMXPtr(amx).GetNativeAddress(index);
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

class AsmJitLoggerAdapter: public asmjit::Logger {
 public:
  AsmJitLoggerAdapter(amxjit::Logger *logger):
    logger_(logger) {}
  virtual void logString(uint32_t style,
                         const char* buf,
                         size_t len = asmjit::kInvalidIndex) {
    assert(logger_ != 0);
    if (len == asmjit::kInvalidIndex) {
      logger_->Write(buf);
    } else {
      std::string tmp_buf(buf, len);
      logger_->Write(tmp_buf.c_str());
    }
  }
 private:
  amxjit::Logger *logger_;
};

} // anonymous namespace

CompileOutputAsmjit::CompileOutputAsmjit(void *code):
  code_(code)
{
  assert(code_ != 0);
}

CompileOutputAsmjit::~CompileOutputAsmjit() {
  jit_runtime.release(code_);
}

void *CompileOutputAsmjit::GetCode() const {
  return code_;
}

EntryPoint CompileOutputAsmjit::GetEntryPoint() const {
  return (EntryPoint)*reinterpret_cast<void**>(GetCode());
}

void CompileOutputAsmjit::Delete() {
  delete this;
}

CompilerAsmjit::CompilerAsmjit():
  asm_(&jit_runtime),
  rib_start_label_(asm_.newLabel()),
  exec_ptr_label_(asm_.newLabel()),
  amx_ptr_label_(asm_.newLabel()),
  ebp_label_(asm_.newLabel()),
  esp_label_(asm_.newLabel()),
  reset_ebp_label_(asm_.newLabel()),
  reset_esp_label_(asm_.newLabel()),
  exec_label_(asm_.newLabel()),
  exec_helper_label_(asm_.newLabel()),
  halt_helper_label_(asm_.newLabel()),
  jump_helper_label_(asm_.newLabel()),
  sysreq_c_helper_label_(asm_.newLabel()),
  sysreq_d_helper_label_(asm_.newLabel()),
  logger_()
{
}

CompilerAsmjit::~CompilerAsmjit() {
}

bool CompilerAsmjit::Prepare(AMXPtr amx) {
  current_amx_ = amx;

  EmitRuntimeInfo();
  EmitInstrTable();
  EmitExec();
  EmitExecHelper();
  EmitHaltHelper();
  EmitJumpHelper();
  EmitSysreqCHelper();
  EmitSysreqDHelper();

  if (GetLogger() != 0) {
    logger_ = new AsmJitLoggerAdapter(GetLogger());
    logger_->setIndentation("\t");
    logger_->setOption(asmjit::kLoggerOptionHexImmediate, true);
    logger_->setOption(asmjit::kLoggerOptionHexDisplacement, true);
    asm_.setLogger(logger_);
  }

  return true;
}

bool CompilerAsmjit::Process(const Instruction &instr) {
  cell cip = instr.address();

  // Align functions on 16-byte boundary.
  if (instr.opcode().GetId() == OP_PROC) {
    asm_.align(asmjit::kAlignCode, 16);
  }

  asm_.bind(GetLabel(cip));
  instr_map_[cip] = asm_.getCodeSize();

  if (logger_ != 0) {
    logger_->logFormat(asmjit::kLoggerStyleComment,
                       "%s; +%08x: %08x: %s\n",
                       logger_->getIndentation(),
                       asm_.getCodeSize(),
                       instr.address(),
                       instr.ToString().c_str());
  }

  return true;
}

CompileOutput *CompilerAsmjit::Finish(bool error) {
  CompileOutput *output = 0;

  if (!error) {
    void *code = asm_.make();
    RuntimeInfoBlock *rib = reinterpret_cast<RuntimeInfoBlock*>(code);

    rib->amx = reinterpret_cast<intptr_t>(current_amx_.raw());
    rib->exec += reinterpret_cast<intptr_t>(code);
    rib->instr_table += reinterpret_cast<intptr_t>(code);

    InstrTableEntry *ite =
      reinterpret_cast<InstrTableEntry*>(rib->instr_table);
    for (InstrMap::const_iterator it = instr_map_.begin();
         it != instr_map_.end(); it++) {
      ite->address = it->first;
      ite->start = static_cast<unsigned char*>(code) + it->second;
      ite++;
    }

    output = new CompileOutputAsmjit(code);
  }

  current_amx_.Reset();

  if (asm_.getLogger() != 0) {
    delete logger_;
    asm_.setLogger(0);
  }

  return output;
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

void CompilerAsmjit::lctrl(cell index, cell cip) {
  // PRI is set to the current value of any of the special registers.
  // The index parameter must be:
  // 3=STP, 4=STK, 5=FRM, 6=CIP (of the next instruction)
  switch (index) {
    case 0:
    case 1:
    case 2:
    case 3:
      asm_.mov(eax, dword_ptr(amx_ptr_label_));
      switch (index) {
        case 0:
          asm_.mov(eax, dword_ptr(eax, offsetof(AMX, base)));
          asm_.mov(eax, dword_ptr(eax, offsetof(AMX_HEADER, cod)));
          break;
        case 1:
          asm_.mov(eax, dword_ptr(eax, offsetof(AMX, base)));
          asm_.mov(eax, dword_ptr(eax, offsetof(AMX_HEADER, dat)));
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
      asm_.mov(eax, cip);
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
      asm_.mov(edx, dword_ptr(amx_ptr_label_));
      asm_.mov(dword_ptr(edx, offsetof(AMX, hea)), eax);
      break;
    case 4:
      asm_.lea(esp, dword_ptr(ebx, eax));
      break;
    case 5:
      asm_.lea(ebp, dword_ptr(ebx, eax));
      break;
    case 6:
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
  asm_.mov(edx, dword_ptr(amx_ptr_label_));
  asm_.mov(ecx, dword_ptr(edx, offsetof(AMX, hea)));
  if (value >= 0) {
    asm_.add(dword_ptr(edx, offsetof(AMX, hea)), value);
  } else {
    asm_.sub(dword_ptr(edx, offsetof(AMX, hea)), -value);
  }
}

void CompilerAsmjit::proc() {
  // [STK] = FRM, STK = STK - cell size, FRM = STK
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
  asm_.mov(esi, eax);
  asm_.lea(eax, dword_ptr(edx, ecx));
  asm_.cdq();
  asm_.idiv(ecx);
  asm_.mov(ecx, edx);
  asm_.mov(eax, esi);
}

void CompilerAsmjit::sdiv_alt() {
  // PRI = ALT / PRI (signed divide), ALT = ALT mod PRI
  asm_.xchg(eax, ecx);
  sdiv();
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
  asm_.lea(edi, dword_ptr(ebx, ecx));
  asm_.push(ecx);
  asm_.mov(ecx, num_bytes / sizeof(cell));
  asm_.rep_stosd();
  asm_.pop(ecx);
}

void CompilerAsmjit::halt(cell error_code) {
  // Abort execution (exit value in PRI), parameters other than 0
  // have a special meaning.
  asm_.mov(edi, error_code);
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
    asm_.mov(edi, AMX_ERR_BOUNDS);
    asm_.jmp(halt_helper_label_);
  asm_.bind(exit_label);
}

void CompilerAsmjit::sysreq_pri() {
  // call system service, service number in PRI
  asm_.call(sysreq_c_helper_label_);
}

void CompilerAsmjit::sysreq_c(cell index, const char *name) {
  // call system service
  if (!EmitIntrinsic(name)) {
    asm_.mov(eax, index);
    asm_.call(sysreq_c_helper_label_);
  }
}

void CompilerAsmjit::sysreq_d(cell address, const char *name) {
  // call system service
  if (!EmitIntrinsic(name)) {
    asm_.mov(eax, address);
    asm_.call(sysreq_d_helper_label_);
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
  asm_.fdivrp(fp1);
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
  asm_.bind(rib_start_label_);
  asm_.bind(exec_ptr_label_);
    asm_.dd(0); // rib->exec
  asm_.bind(amx_ptr_label_);
    asm_.dd(0); // rib->amx_ptr
  asm_.bind(ebp_label_);
    asm_.dd(0); // rib->ebp
  asm_.bind(esp_label_);
    asm_.dd(0); // rib->esp
  asm_.bind(reset_ebp_label_);
    asm_.dd(0); // rib->reset_ebp
  asm_.bind(reset_esp_label_);
    asm_.dd(0); // rib->reset_esp
    asm_.dd(0); // rib->instr_map
    asm_.dd(0); // rib->instr_map_size
}

void CompilerAsmjit::EmitInstrTable() {
  int num_entries = 0;

  Instruction instr;
  Disassembler disasm(current_amx_);
  while (disasm.Decode(instr)) {
    num_entries++;
  }

  RuntimeInfoBlock *rib = reinterpret_cast<RuntimeInfoBlock*>(asm_.getBuffer());
  rib->instr_table = asm_.getCodeSize();
  rib->instr_table_size = num_entries;

  InstrTableEntry dummy;
  for (int i = 0; i < num_entries; i++) {
    asm_.dstruct(dummy);
  }
}

// int AMXJIT_CDECL Exec(cell index, cell *retval);
void CompilerAsmjit::EmitExec() {
  RuntimeInfoBlock *rib = reinterpret_cast<RuntimeInfoBlock*>(asm_.getBuffer());
  rib->exec = asm_.getCodeSize();

  Label null_data_label = asm_.newLabel();
  Label stack_heap_overflow_label = asm_.newLabel();
  Label heap_underflow_label = asm_.newLabel();
  Label stack_underflow_label = asm_.newLabel();
  Label native_not_found_label = asm_.newLabel();
  Label public_not_found_label = asm_.newLabel();
  Label finish_label = asm_.newLabel();
  Label return_label = asm_.newLabel();

  int arg_index = 8;
  int arg_retval = 12;
  int var_address = -4;
  int var_reset_ebp = -8;
  int var_reset_esp = -12;

  asm_.bind(exec_label_);
    asm_.push(ebp);
    asm_.mov(ebp, esp);

    // Allocate space for the local variables.
    asm_.sub(esp, 12);

    asm_.push(esi);
    asm_.mov(esi, dword_ptr(amx_ptr_label_));

    // Set ebx to point to the AMX data section.
    asm_.push(ebx);
    asm_.mov(ebx, dword_ptr(esi, offsetof(AMX, data)));
    asm_.test(ebx, ebx);
    asm_.jnz(null_data_label);
    asm_.mov(ebx, dword_ptr(esi, offsetof(AMX, base)));
    asm_.mov(eax, dword_ptr(ebx, offsetof(AMX_HEADER, dat)));
    asm_.add(ebx, eax);

  asm_.bind(null_data_label);

    // Check for a stack/heap collision (stack/heap overflow).
    asm_.mov(ecx, dword_ptr(esi, offsetof(AMX, hea)));
    asm_.mov(edx, dword_ptr(esi, offsetof(AMX, stk)));
    asm_.cmp(ecx, edx);
    asm_.jge(stack_heap_overflow_label);

    // Check for a stack underflow.
    asm_.mov(ecx, dword_ptr(esi, offsetof(AMX, stk)));
    asm_.mov(edx, dword_ptr(esi, offsetof(AMX, stp)));
    asm_.cmp(ecx, edx);
    asm_.jg(stack_underflow_label);

    // Check for a heap underflow.
    asm_.mov(ecx, dword_ptr(esi, offsetof(AMX, hea)));
    asm_.mov(edx, dword_ptr(esi, offsetof(AMX, hlw)));
    asm_.cmp(ecx, edx);
    asm_.jl(heap_underflow_label);

    // Make sure that all natives functions have been registered.
    asm_.mov(ecx, dword_ptr(esi, offsetof(AMX, flags)));
    asm_.test(ecx, AMX_FLAG_NTVREG);
    asm_.jz(native_not_found_label);

    // Reset the error code.
    asm_.mov(dword_ptr(esi, offsetof(AMX, error)), AMX_ERR_NONE);

    // Get the address of the public function.
    asm_.push(dword_ptr(ebp, arg_index));
    asm_.mov(eax, dword_ptr(amx_ptr_label_));
    asm_.push(eax);
    asm_.call(reinterpret_cast<asmjit::Ptr>(&GetPublicAddress));
    asm_.add(esp, 8);

    // Check if the function was actually found.
    asm_.test(eax, eax);
    asm_.jz(public_not_found_label);

    // Get the function's start address.
    asm_.lea(ecx, dword_ptr(rib_start_label_));
    asm_.push(ecx);
    asm_.push(eax);
    asm_.call(reinterpret_cast<asmjit::Ptr>(&GetInstrStartPtr));
    asm_.add(esp, 8);
    asm_.mov(dword_ptr(ebp, var_address), eax);

    // Push the size of the arguments and reset the parameter count.
    asm_.mov(eax, dword_ptr(esi, offsetof(AMX, paramcount)));
    asm_.imul(eax, eax, sizeof(cell));
    asm_.mov(ecx, dword_ptr(esi, offsetof(AMX, stk)));
    asm_.sub(ecx, sizeof(cell));
    asm_.mov(dword_ptr(ebx, ecx), eax);
    asm_.mov(dword_ptr(esi, offsetof(AMX, stk)), ecx);
    asm_.mov(dword_ptr(esi, offsetof(AMX, paramcount)), 0);

    // Save the old reset_ebp and reset_esp on the stack.
    asm_.mov(eax, dword_ptr(reset_ebp_label_));
    asm_.mov(dword_ptr(ebp, var_reset_ebp), eax);
    asm_.mov(eax, dword_ptr(reset_esp_label_));
    asm_.mov(dword_ptr(ebp, var_reset_esp), eax);

    // Call the function.
    asm_.push(dword_ptr(ebp, var_address));
    asm_.call(exec_helper_label_);
    asm_.add(esp, 4);

    // Copy the return value to retval (if it's not null).
    asm_.mov(ecx, dword_ptr(ebp, arg_retval));
    asm_.test(ecx, ecx);
    asm_.jz(finish_label);
    asm_.mov(dword_ptr(ecx), eax);

  asm_.bind(finish_label);
    // Restore reset_ebp and reset_esp from the stack.
    asm_.mov(eax, dword_ptr(ebp, var_reset_ebp));
    asm_.mov(dword_ptr(reset_ebp_label_), eax);
    asm_.mov(eax, dword_ptr(ebp, var_reset_esp));
    asm_.mov(dword_ptr(reset_esp_label_), eax);

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
    // Store the function address in eax.
    asm_.mov(eax, dword_ptr(esp, 4));

    // These registers are caller-saved in JIT code.
    asm_.push(esi);
    asm_.push(edi);

    // Store the old ebp and esp on the stack.
    asm_.push(dword_ptr(ebp_label_));
    asm_.push(dword_ptr(esp_label_));

    // The most recent ebp and esp are stored in RIB.
    asm_.mov(dword_ptr(ebp_label_), ebp);
    asm_.mov(dword_ptr(esp_label_), esp);

    // Switch from the native stack to the AMX stack.
    asm_.mov(ecx, dword_ptr(amx_ptr_label_));
    asm_.mov(edx, dword_ptr(esi, offsetof(AMX, frm)));
    asm_.lea(ebp, dword_ptr(ebx, edx)); // ebp = data + amx->frm
    asm_.mov(edx, dword_ptr(ecx, offsetof(AMX, stk)));
    asm_.lea(esp, dword_ptr(ebx, edx)); // esp = data + amx->stk

    // In order to make halt() work we must able to return to this place.
    asm_.lea(ecx, dword_ptr(esp, - 4));
    asm_.mov(dword_ptr(reset_esp_label_), ecx);
    asm_.mov(dword_ptr(reset_ebp_label_), ebp);

    // Call the function. Prior to this point ebx should point to the
    // AMX data and the both stack pointers should point to somewhere
    // in the AMX stack.
    asm_.call(eax);

    // Keep the AMX stack registers up-to-date. This wouldn't be necessary
    // if RETN didn't modify them (it pops all arguments off the stack).
    asm_.mov(ecx, dword_ptr(amx_ptr_label_));
    asm_.mov(edx, ebp);
    asm_.sub(edx, ebx);
    asm_.mov(dword_ptr(ecx, offsetof(AMX, frm)), edx); // amx->frm = ebp - data
    asm_.mov(edx, esp);
    asm_.sub(edx, ebx);
    asm_.mov(dword_ptr(ecx, offsetof(AMX, stk)), edx); // amx->stk = esp - data

    // Switch back to the native stack.
    asm_.mov(ebp, dword_ptr(ebp_label_));
    asm_.mov(esp, dword_ptr(esp_label_));

    asm_.pop(dword_ptr(esp_label_));
    asm_.pop(dword_ptr(ebp_label_));

    asm_.pop(edi);
    asm_.pop(esi);
    asm_.ret();
}

// void HaltHelper(int error [edi]);
void CompilerAsmjit::EmitHaltHelper() {
  asm_.bind(halt_helper_label_);
    asm_.mov(esi, dword_ptr(amx_ptr_label_));
    asm_.mov(dword_ptr(esi, offsetof(AMX, error)), edi); // error code in edi

    // Reset the stack so that we return to the instruction next to CALL.
    asm_.mov(esp, dword_ptr(reset_esp_label_));
    asm_.mov(ebp, dword_ptr(reset_ebp_label_));

    // Pop the public function's arguments as it done by RETN.
    asm_.pop(eax);
    asm_.add(esp, dword_ptr(esp));
    asm_.add(esp, 4);
    asm_.push(eax);

    asm_.ret();
}

// void JumpHelper(void *address [eax]);
void CompilerAsmjit::EmitJumpHelper() {
  Label invalid_address_label = asm_.newLabel();

  asm_.bind(jump_helper_label_);
    asm_.push(eax);
    asm_.push(ecx);

    asm_.lea(ecx, dword_ptr(rib_start_label_));
    asm_.push(ecx);
    asm_.push(eax);
    asm_.call(reinterpret_cast<asmjit::Ptr>(&GetInstrStartPtr));
    asm_.add(esp, 8);
    asm_.mov(edx, eax); // address

    asm_.pop(ecx);
    asm_.pop(eax);

    asm_.test(edx, edx);
    asm_.jz(invalid_address_label);

    asm_.lea(esp, dword_ptr(esp, 4));
    asm_.jmp(edx);

  // Continue execution as if there was no jump at all (this is what AMX does).
  asm_.bind(invalid_address_label);
    asm_.ret();
}

// cell SysreqCHelper(int index [eax]);
void CompilerAsmjit::EmitSysreqCHelper() {
  asm_.bind(sysreq_c_helper_label_);
    asm_.mov(esi, dword_ptr(esp));
    asm_.lea(esp, dword_ptr(esp, 4));
    asm_.mov(ecx, esp);

    asm_.mov(edx, dword_ptr(amx_ptr_label_));

    // Switch to the native stack.
    asm_.sub(ebp, ebx);
    asm_.mov(dword_ptr(edx, offsetof(AMX, frm)), ebp); // amx->frm = ebp - data
    asm_.mov(ebp, dword_ptr(ebp_label_));
    asm_.sub(esp, ebx);
    asm_.mov(dword_ptr(edx, offsetof(AMX, stk)), esp); // amx->stk = esp - data
    asm_.mov(esp, dword_ptr(esp_label_));

    // Call the native function.
    asm_.push(0);
    asm_.mov(edi, esp);
    asm_.push(ecx); // params
    asm_.push(edi); // result
    asm_.push(eax); // index
    asm_.push(edx); // amx
    asm_.call(dword_ptr(edx, offsetof(AMX, callback)));
    asm_.add(esp, 20);

    // Get the result.
    asm_.mov(edi, dword_ptr(esp, -4));
    asm_.xchg(eax, edi);

    // Switch back to the AMX stack.
    asm_.mov(edx, dword_ptr(amx_ptr_label_));
    asm_.mov(dword_ptr(ebp_label_), ebp);
    asm_.mov(ecx, dword_ptr(edx, offsetof(AMX, frm)));
    asm_.lea(ebp, dword_ptr(ebx, ecx)); // ebp = data + amx->frm
    asm_.mov(dword_ptr(esp_label_), esp);
    asm_.mov(ecx, dword_ptr(edx, offsetof(AMX, stk)));
    asm_.lea(esp, dword_ptr(ebx, ecx)); // ebp = data + amx->stk

    // Check for errors.
    asm_.cmp(edi, AMX_ERR_NONE);
    asm_.jne(halt_helper_label_);

    // Modify the return address so we return next to the sysreq point.
    asm_.push(esi);
    asm_.ret();
}

// cell SysreqDHelper(void *address [eax]);
void CompilerAsmjit::EmitSysreqDHelper() {
  asm_.bind(sysreq_d_helper_label_);
    asm_.mov(esi, dword_ptr(esp));
    asm_.lea(esp, dword_ptr(esp, 4));
    asm_.mov(ecx, esp);

    asm_.mov(edx, dword_ptr(amx_ptr_label_));

    // Switch to the native stack.
    asm_.sub(ebp, ebx);
    asm_.mov(dword_ptr(edx, offsetof(AMX, frm)), ebp); // amx->frm = ebp - data
    asm_.mov(ebp, dword_ptr(ebp_label_));
    asm_.sub(esp, ebx);
    asm_.mov(dword_ptr(edx, offsetof(AMX, stk)), esp); // amx->stk = esp - data
    asm_.mov(esp, dword_ptr(esp_label_));

    // Call the native function.
    asm_.push(ecx); // params
    asm_.push(edx); // amx
    asm_.call(eax); // address
    asm_.add(esp, 8);

    // Switch back to the AMX stack.
    asm_.mov(edx, dword_ptr(amx_ptr_label_));
    asm_.mov(dword_ptr(ebp_label_), ebp);
    asm_.mov(ecx, dword_ptr(edx, offsetof(AMX, frm)));
    asm_.lea(ebp, dword_ptr(ebx, ecx)); // ebp = data + amx->frm
    asm_.mov(dword_ptr(esp_label_), esp);
    asm_.mov(ecx, dword_ptr(edx, offsetof(AMX, stk)));
    asm_.lea(esp, dword_ptr(ebx, ecx)); // ebp = data + amx->stk

    // Modify the return address so we return next to the sysreq point.
    asm_.push(esi);
    asm_.ret();
}

const Label &CompilerAsmjit::GetLabel(cell address) {
  Label &label = label_map_[address];
  if (label.getId() == asmjit::kInvalidValue) {
    label = asm_.newLabel();
  }
  return label;
}

} // namespace amxjit
