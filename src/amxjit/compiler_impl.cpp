// Copyright (c) 2012-2019 Zeex
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
#include "compiler.h"
#include "compiler_impl.h"
#include "cstdint.h"
#include "disasm.h"
#include "logger.h"
#include "platform.h"

using asmjit::Label;
using asmjit::x86::byte_ptr;
using asmjit::x86::word_ptr;
using asmjit::x86::dword_ptr;
using asmjit::x86::dword_ptr_abs;
using asmjit::x86::ah;
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
using asmjit::x86::fp0;
using asmjit::x86::fp1;

namespace amxjit {
namespace {

struct RuntimeInfoBlock {
  intptr_t exec;
  intptr_t amx;
  intptr_t ebp;
  intptr_t esp;
  intptr_t amx_ebp;
  intptr_t amx_esp;
  intptr_t reset_ebp;
  intptr_t reset_esp;
  cell reset_stk;
  cell reset_hea;
  intptr_t instr_table;
  intptr_t instr_table_size;
};

asmjit::JitRuntime jit_runtime;

cell AMXJIT_CDECL GetPublicAddress(AMX *amx, int index) {
  return AMXRef(amx).GetPublicAddress(index);
}

cell AMXJIT_CDECL GetNativeAddress(AMX *amx, int index) {
  return AMXRef(amx).GetNativeAddress(index);
}

class InstrTableEntry {
 public:
  InstrTableEntry(): address(), ptr() {}
  InstrTableEntry(cell address, uintptr_t ptr): address(address), ptr(ptr) {}

  static bool CompareByAMXAddress(
    const InstrTableEntry &e1, const InstrTableEntry &e2)
  {
    return e1.address < e2.address;
  }

  static bool CompareByJITPtr(
    const InstrTableEntry &e1, const InstrTableEntry &e2)
  {
    return e1.ptr < e2.ptr;
  }

 public:
  cell address;
  uintptr_t ptr;
};

uintptr_t AMXJIT_CDECL GetJITInstrPtr(cell address, RuntimeInfoBlock *rib) {
  assert(rib->instr_table != 0);
  assert(rib->instr_table_size > 0);

  InstrTableEntry *instr_table =
    reinterpret_cast<InstrTableEntry*>(rib->instr_table);
  InstrTableEntry target(address, 0);

  InstrTableEntry *first = instr_table;
  InstrTableEntry *last = instr_table + rib->instr_table_size;
  InstrTableEntry *result = std::lower_bound(
    first,
    last,
    target,
    InstrTableEntry::CompareByAMXAddress);
  if (result != last && result->address == address) {
    return result->ptr;
  }
  return 0;
}

cell AMXJIT_CDECL GetANXAddressByJITInstrPtr(uintptr_t ptr, RuntimeInfoBlock *rib) {
  assert(rib->instr_table != 0);
  assert(rib->instr_table_size > 0);

  InstrTableEntry *instr_table =
    reinterpret_cast<InstrTableEntry*>(rib->instr_table);
  InstrTableEntry target(0, ptr);

  InstrTableEntry *first = instr_table;
  InstrTableEntry *last = instr_table + rib->instr_table_size;
  InstrTableEntry *result = std::lower_bound(
    first,
    last,
    target,
    InstrTableEntry::CompareByJITPtr);
  if (result != last && result->ptr == ptr) {
    return result->address;
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

CodeBuffer::CodeBuffer(void *code):
  code_(code)
{
  assert(code_ != 0);
}

CodeBuffer::~CodeBuffer() {
  jit_runtime.release(code_);
}

CodeEntryPoint CodeBuffer::GetEntryPoint() const {
  return (CodeEntryPoint)*reinterpret_cast<void**>(code_);
}

void CodeBuffer::Delete() {
  delete this;
}

CompilerImpl::CompilerImpl():
  asmjit_logger_(),
  asm_(&jit_runtime),
  rib_start_label_(asm_.newLabel()),
  exec_ptr_label_(asm_.newLabel()),
  amx_ptr_label_(asm_.newLabel()),
  ebp_label_(asm_.newLabel()),
  esp_label_(asm_.newLabel()),
  amx_ebp_label_(asm_.newLabel()),
  amx_esp_label_(asm_.newLabel()),
  reset_ebp_label_(asm_.newLabel()),
  reset_esp_label_(asm_.newLabel()),
  reset_stk_label_(asm_.newLabel()),
  reset_hea_label_(asm_.newLabel()),
  exec_label_(asm_.newLabel()),
  exec_helper_label_(asm_.newLabel()),
  exec_exit_label_(asm_.newLabel()),
  exec_cont_helper_label_(asm_.newLabel()),
  halt_helper_label_(asm_.newLabel()),
  jump_helper_label_(asm_.newLabel()),
  jump_lookup_label_(asm_.newLabel()),
  reverse_jump_lookup_label_(asm_.newLabel()),
  sysreq_c_helper_label_(asm_.newLabel()),
  sysreq_d_helper_label_(asm_.newLabel()),
  logger_(),
  error_handler_(),
  enable_sysreq_d_(false),
  debug_flags_(0)
{
}

CodeBuffer *CompilerImpl::Compile(AMXRef amx) {
  amx_ = amx;

  EmitRuntimeInfo();
  EmitInstrTable();
  EmitExec();
  EmitExecHelper();
  if (enable_sleep_) {
    EmitExecContHelper();
  }
  EmitHaltHelper();
  EmitJumpLookup();
  EmitReverseJumpLookup();
  EmitJumpHelper();
  EmitSysreqCHelper();
  EmitSysreqDHelper();

  if (logger_ != 0) {
    asmjit_logger_ = new AsmJitLoggerAdapter(logger_);
    asmjit_logger_->setIndentation("\t");
    asmjit_logger_->setOption(asmjit::kLoggerOptionHexImmediate, true);
    asmjit_logger_->setOption(asmjit::kLoggerOptionHexDisplacement, true);
    asm_.setLogger(asmjit_logger_);
  }

  Disassembler disasm(amx);
  Instruction instr;
  bool error = false;

  while (!error && disasm.Decode(instr, error)) {
    cell cip = instr.address();

    // Align functions on 16-byte boundary.
    if (instr.opcode().GetId() == OP_PROC) {
      asm_.align(asmjit::kAlignCode, 16);
    }

    asm_.bind(GetLabel(cip));
    instr_map_[cip] = asm_.getCodeSize();

    if (asmjit_logger_ != 0) {
      asmjit_logger_->logFormat(asmjit::kLoggerStyleComment,
                                "%s; +%08x: %08x: %s\n",
                                asmjit_logger_->getIndentation(),
                                asm_.getCodeSize(),
                                instr.address(),
                                instr.ToString().c_str());
    }

    // eax = PRI
    // ecx = ALT
    // ebp = FRM
    // esp = STK
    // ebx = data (amx->data or amx->base + amxhdr->dat)

    switch (instr.opcode().GetId()) {
      case OP_LOAD_PRI:
        // PRI = [address]
        asm_.mov(eax, dword_ptr(ebx, instr.operand()));
        break;
      case OP_LOAD_ALT:
        // ALT = [address]
        asm_.mov(ecx, dword_ptr(ebx, instr.operand()));
        break;
      case OP_LOAD_S_PRI:
        // PRI = [FRM + offset]
        asm_.mov(eax, dword_ptr(ebp, instr.operand()));
        break;
      case OP_LOAD_S_ALT:
        // ALT = [FRM + offset]
        asm_.mov(ecx, dword_ptr(ebp, instr.operand()));
        break;
      case OP_LREF_PRI:
        // PRI = [ [address] ]
        asm_.mov(edx, dword_ptr(ebx, instr.operand()));
        asm_.mov(eax, dword_ptr(ebx, edx));
        break;
      case OP_LREF_ALT:
        // ALT = [ [address] ]
        asm_.mov(edx, dword_ptr(ebx, + instr.operand()));
        asm_.mov(ecx, dword_ptr(ebx, edx));
        break;
      case OP_LREF_S_PRI:
        // PRI = [ [FRM + offset] ]
        asm_.mov(edx, dword_ptr(ebp, instr.operand()));
        asm_.mov(eax, dword_ptr(ebx, edx));
        break;
      case OP_LREF_S_ALT:
        // PRI = [ [FRM + offset] ]
        asm_.mov(edx, dword_ptr(ebp, instr.operand()));
        asm_.mov(ecx, dword_ptr(ebx, edx));
        break;
      case OP_LOAD_I:
        // PRI = [PRI] (full cell)
        asm_.mov(eax, dword_ptr(ebx, eax));
        break;
      case OP_LODB_I:
        // PRI = "number" bytes from [PRI] (read 1/2/4 bytes)
        switch (instr.operand()) {
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
        break;
      case OP_CONST_PRI:
        // PRI = value
        if (instr.operand() == 0) {
          asm_.xor_(eax, eax);
        } else {
          asm_.mov(eax, instr.operand());
        }
        break;
      case OP_CONST_ALT:
        // ALT = value
        if (instr.operand() == 0) {
          asm_.xor_(ecx, ecx);
        } else {
          asm_.mov(ecx, instr.operand());
        }
        break;
      case OP_ADDR_PRI:
        // PRI = FRM + offset
        asm_.lea(eax, dword_ptr(ebp, instr.operand()));
        asm_.sub(eax, ebx);
        break;
      case OP_ADDR_ALT:
        // ALT = FRM + offset
        asm_.lea(ecx, dword_ptr(ebp, instr.operand()));
        asm_.sub(ecx, ebx);
        break;
      case OP_STOR_PRI:
        // [address] = PRI
        asm_.mov(dword_ptr(ebx, instr.operand()), eax);
        break;
      case OP_STOR_ALT:
        // [address] = ALT
        asm_.mov(dword_ptr(ebx, instr.operand()), ecx);
        break;
      case OP_STOR_S_PRI:
        // [FRM + offset] = ALT
        asm_.mov(dword_ptr(ebp, instr.operand()), eax);
        break;
      case OP_STOR_S_ALT:
        // [FRM + offset] = ALT
        asm_.mov(dword_ptr(ebp, instr.operand()), ecx);
        break;
      case OP_SREF_PRI:
        // [ [address] ] = PRI
        asm_.mov(edx, dword_ptr(ebx, instr.operand()));
        asm_.mov(dword_ptr(ebx, edx), eax);
        break;
      case OP_SREF_ALT:
        // [ [address] ] = ALT
        asm_.mov(edx, dword_ptr(ebx, instr.operand()));
        asm_.mov(dword_ptr(ebx, edx), ecx);
        break;
      case OP_SREF_S_PRI:
        // [ [FRM + offset] ] = PRI
        asm_.mov(edx, dword_ptr(ebp, instr.operand()));
        asm_.mov(dword_ptr(ebx, edx), eax);
        break;
      case OP_SREF_S_ALT:
        // [ [FRM + offset] ] = ALT
        asm_.mov(edx, dword_ptr(ebp, instr.operand()));
        asm_.mov(dword_ptr(ebx, edx), ecx);
        break;
      case OP_STOR_I:
        // [ALT] = PRI (full cell)
        asm_.mov(dword_ptr(ebx, ecx), eax);
        break;
      case OP_STRB_I:
        // "number" bytes at [ALT] = PRI (write 1/2/4 bytes)
        switch (instr.operand()) {
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
        break;
      case OP_LIDX:
        // PRI = [ ALT + (PRI x cell size) ]
        asm_.lea(edx, dword_ptr(ebx, ecx));
        asm_.mov(eax, dword_ptr(edx, eax, 2));
        break;
      case OP_LIDX_B:
        // PRI = [ ALT + (PRI << shift) ]
        asm_.lea(edx, dword_ptr(ebx, ecx));
        asm_.mov(eax, dword_ptr(edx, eax, instr.operand()));
        break;
      case OP_IDXADDR:
        // PRI = ALT + (PRI x cell size) (calculate indexed address)
        asm_.lea(eax, dword_ptr(ecx, eax, 2));
        break;
      case OP_IDXADDR_B:
        // PRI = ALT + (PRI << shift) (calculate indexed address)
        asm_.lea(eax, dword_ptr(ecx, eax, instr.operand()));
        break;
      case OP_ALIGN_PRI:
        // Little Endian: PRI ^= cell size - number
        #if BYTE_ORDER == LITTLE_ENDIAN
          if (static_cast<std::size_t>(instr.operand()) < sizeof(cell)) {
            asm_.xor_(eax, sizeof(cell) - instr.operand());
          }
        #endif
        break;
      case OP_ALIGN_ALT:
        // Little Endian: ALT ^= cell size - number
        #if BYTE_ORDER == LITTLE_ENDIAN
          if (static_cast<std::size_t>(instr.operand()) < sizeof(cell)) {
            asm_.xor_(ecx, sizeof(cell) - instr.operand());
          }
        #endif
        break;
      case OP_LCTRL:
        // PRI is set to the current value of any of the special registers.
        // The index parameter must be:
        // 3=STP, 4=STK, 5=FRM, 6=CIP (of the next instruction)
        switch (instr.operand()) {
          case 0:
          case 1:
          case 2:
          case 3:
            asm_.mov(eax, dword_ptr(amx_ptr_label_));
            switch (instr.operand()) {
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
            asm_.mov(eax, instr.address() + instr.size());
            break;
          case 7:
            asm_.mov(eax, 1);
            break;
          case 8:
            asm_.call(jump_lookup_label_);
            break;
        }
        break;
      case OP_SCTRL:
        // set the indexed special registers to the value in PRI.
        // The index parameter must be:
        // 6=CIP
        switch (instr.operand()) {
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
          case 8:
            asm_.jmp(eax);
            break;
        }
        break;
      case OP_MOVE_PRI:
        // PRI = ALT
        asm_.mov(eax, ecx);
        break;
      case OP_MOVE_ALT:
        // ALT = PRI
        asm_.mov(ecx, eax);
        break;
      case OP_XCHG:
        // Exchange PRI and ALT
        asm_.xchg(eax, ecx);
        break;
      case OP_PUSH_PRI:
        // [STK] = PRI, STK = STK - cell size
        asm_.push(eax);
        break;
      case OP_PUSH_ALT:
        // [STK] = ALT, STK = STK - cell size
        asm_.push(ecx);
        break;
      case OP_PUSH_C:
        // [STK] = value, STK = STK - cell size
        asm_.push(instr.operand());
        break;
      case OP_PUSH:
        // [STK] = [address], STK = STK - cell size
        asm_.push(dword_ptr(ebx, instr.operand()));
        break;
      case OP_PUSH_S:
        // [STK] = [FRM + offset], STK = STK - cell size
        asm_.push(dword_ptr(ebp, instr.operand()));
        break;
      case OP_POP_PRI:
        // STK = STK + cell size, PRI = [STK]
        asm_.pop(eax);
        break;
      case OP_POP_ALT:
        // STK = STK + cell size, ALT = [STK]
        asm_.pop(ecx);
        break;
      case OP_STACK:
        // ALT = STK, STK = STK + value
        asm_.mov(ecx, esp);
        asm_.sub(ecx, ebx);
        if (instr.operand() >= 0) {
          asm_.add(esp, instr.operand());
        } else {
          asm_.sub(esp, -instr.operand());
        }
        break;
      case OP_HEAP:
        // ALT = HEA, HEA = HEA + value
        asm_.mov(edx, dword_ptr(amx_ptr_label_));
        asm_.mov(ecx, dword_ptr(edx, offsetof(AMX, hea)));
        if (instr.operand() >= 0) {
          asm_.add(dword_ptr(edx, offsetof(AMX, hea)), instr.operand());
        } else {
          asm_.sub(dword_ptr(edx, offsetof(AMX, hea)), -instr.operand());
        }
        break;
      case OP_PROC:
        // [STK] = FRM, STK = STK - cell size, FRM = STK
        asm_.push(ebp);
        asm_.mov(ebp, esp);
        asm_.sub(dword_ptr(esp), ebx);
        break;
      case OP_RET:
        // STK = STK + cell size, FRM = [STK],
        // CIP = [STK], STK = STK + cell size
        asm_.pop(ebp);
        asm_.add(ebp, ebx);
        asm_.ret();
        break;
      case OP_RETN:
        // STK = STK + cell size, FRM = [STK],
        // CIP = [STK], STK = STK + cell size
        // The RETN instruction removes a specified number of bytes
        // from the stack. The value to adjust STK with must be
        // pushed prior to the call.
        asm_.pop(ebp);
        asm_.add(ebp, ebx);
        asm_.pop(edx);
        asm_.add(esp, dword_ptr(esp));
        asm_.add(esp, 4);
        asm_.mov(esi, dword_ptr(amx_ptr_label_));
        asm_.mov(edi, esp);
        asm_.sub(edi, ebx);
        asm_.mov(dword_ptr(esi, offsetof(AMX, stk)), edi);
        asm_.push(edx);
        asm_.ret();
        break;
      case OP_JUMP_PRI:
        // CIP = PRI (indirect jump)
        asm_.call(jump_helper_label_);
        break;
      case OP_CALL:
      case OP_JUMP:
      case OP_JZER:
      case OP_JNZ:
      case OP_JEQ:
      case OP_JNEQ:
      case OP_JLESS:
      case OP_JLEQ:
      case OP_JGRTR:
      case OP_JGEQ:
      case OP_JSLESS:
      case OP_JSLEQ:
      case OP_JSGRTR:
      case OP_JSGEQ: {
        cell dest = instr.operand() - reinterpret_cast<cell>(amx.code());
        switch (instr.opcode().GetId()) {
          case OP_CALL:
            // [STK] = CIP + 5, STK = STK - cell size
            // CIP = CIP + offset
            // The CALL instruction jumps to an address after storing the
            // address of the next sequential instruction on the stack.
            // The address jumped to is relative to the current CIP,
            // but the address on the stack is an absolute address.
            asm_.call(GetLabel(dest));
            break;
          case OP_JUMP:
            // CIP = CIP + offset (jump to the address relative from
            // the current position)
            asm_.jmp(GetLabel(dest));
            break;
          case OP_JZER:
            // if PRI == 0 then CIP = CIP + offset
            asm_.test(eax, eax);
            asm_.jz(GetLabel(dest));
            break;
          case OP_JNZ:
            // if PRI != 0 then CIP = CIP + offset
            asm_.test(eax, eax);
            asm_.jnz(GetLabel(dest));
            break;
          case OP_JEQ:
            // if PRI == ALT then CIP = CIP + offset
            asm_.cmp(eax, ecx);
            asm_.je(GetLabel(dest));
            break;
          case OP_JNEQ:
            // if PRI != ALT then CIP = CIP + offset
            asm_.cmp(eax, ecx);
            asm_.jne(GetLabel(dest));
            break;
          case OP_JLESS:
            // if PRI < ALT then CIP = CIP + offset (unsigned)
            asm_.cmp(eax, ecx);
            asm_.jb(GetLabel(dest));
            break;
          case OP_JLEQ:
            // if PRI <= ALT then CIP = CIP + offset (unsigned)
            asm_.cmp(eax, ecx);
            asm_.jbe(GetLabel(dest));
            break;
          case OP_JGRTR:
            // if PRI > ALT then CIP = CIP + offset (unsigned)
            asm_.cmp(eax, ecx);
            asm_.ja(GetLabel(dest));
            break;
          case OP_JGEQ:
            // if PRI >= ALT then CIP = CIP + offset (unsigned)
            asm_.cmp(eax, ecx);
            asm_.jae(GetLabel(dest));
            break;
          case OP_JSLESS:
            // if PRI < ALT then CIP = CIP + offset (signed)
            asm_.cmp(eax, ecx);
            asm_.jl(GetLabel(dest));
            break;
          case OP_JSLEQ:
            // if PRI <= ALT then CIP = CIP + offset (signed)
            asm_.cmp(eax, ecx);
            asm_.jle(GetLabel(dest));
            break;
          case OP_JSGRTR:
            // if PRI > ALT then CIP = CIP + offset (signed)
            asm_.cmp(eax, ecx);
            asm_.jg(GetLabel(dest));
            break;
          case OP_JSGEQ:
            // if PRI >= ALT then CIP = CIP + offset (signed)
            asm_.cmp(eax, ecx);
            asm_.jge(GetLabel(dest));
            break;
        }
        break;
      }
      case OP_SHL:
        // PRI = PRI << ALT
        asm_.shl(eax, cl);
        break;
      case OP_SHR:
        // PRI = PRI >> ALT (without sign extension)
        asm_.shr(eax, cl);
        break;
      case OP_SSHR:
        // PRI = PRI >> ALT with sign extension
        asm_.sar(eax, cl);
        break;
      case OP_SHL_C_PRI:
        // PRI = PRI << value
        asm_.shl(eax, static_cast<unsigned char>(instr.operand()));
        break;
      case OP_SHL_C_ALT:
        // ALT = ALT << value
        asm_.shl(ecx, static_cast<unsigned char>(instr.operand()));
        break;
      case OP_SHR_C_PRI:
        // PRI = PRI >> value (without sign extension)
        asm_.shr(eax, static_cast<unsigned char>(instr.operand()));
        break;
      case OP_SHR_C_ALT:
        // ALT = ALT >> value (without sign extension)
        asm_.shr(ecx, static_cast<unsigned char>(instr.operand()));
        break;
      case OP_SMUL:
        // PRI = PRI * ALT (signed multiply)
        asm_.imul(ecx);
        break;
      case OP_SDIV:
        // PRI = PRI / ALT (signed divide), ALT = PRI mod ALT
        asm_.cdq();
        asm_.idiv(ecx);
        asm_.mov(esi, eax);
        asm_.lea(eax, dword_ptr(edx, ecx));
        asm_.cdq();
        asm_.idiv(ecx);
        asm_.mov(ecx, edx);
        asm_.mov(eax, esi);
        break;
      case OP_SDIV_ALT:
        // PRI = ALT / PRI (signed divide), ALT = ALT mod PRI
        asm_.xchg(eax, ecx);
        asm_.cdq();
        asm_.idiv(ecx);
        asm_.mov(esi, eax);
        asm_.lea(eax, dword_ptr(edx, ecx));
        asm_.cdq();
        asm_.idiv(ecx);
        asm_.mov(ecx, edx);
        asm_.mov(eax, esi);
        break;
      case OP_UMUL:
        // PRI = PRI * ALT (unsigned multiply)
        asm_.mul(ecx);
        break;
      case OP_UDIV:
        // PRI = PRI / ALT (unsigned divide), ALT = PRI mod ALT
        asm_.xor_(edx, edx);
        asm_.div(ecx);
        asm_.mov(ecx, edx);
        break;
      case OP_UDIV_ALT:
        // PRI = ALT / PRI (unsigned divide), ALT = ALT mod PRI
        asm_.xchg(eax, ecx);
        asm_.xor_(edx, edx);
        asm_.div(ecx);
        asm_.mov(ecx, edx);
        break;
      case OP_ADD:
        // PRI = PRI + ALT
        asm_.add(eax, ecx);
        break;
      case OP_SUB:
        // PRI = PRI - ALT
        asm_.sub(eax, ecx);
        break;
      case OP_SUB_ALT:
        // PRI = ALT - PRI
        // or:
        // PRI = -(PRI - ALT)
        asm_.sub(eax, ecx);
        asm_.neg(eax);
        break;
      case OP_AND:
        // PRI = PRI & ALT
        asm_.and_(eax, ecx);
        break;
      case OP_OR:
        // PRI = PRI | ALT
        asm_.or_(eax, ecx);
        break;
      case OP_XOR:
        // PRI = PRI ^ ALT
        asm_.xor_(eax, ecx);
        break;
      case OP_NOT:
        // PRI = !PRI
        asm_.test(eax, eax);
        asm_.setz(al);
        asm_.movzx(eax, al);
        break;
      case OP_NEG:
        // PRI = -PRI
        asm_.neg(eax);
        break;
      case OP_INVERT:
        // PRI = ~PRI
        asm_.not_(eax);
        break;
      case OP_ADD_C:
        // PRI = PRI + value
        if (instr.operand() >= 0) {
          asm_.add(eax, instr.operand());
        } else {
          asm_.sub(eax, -instr.operand());
        }
        break;
      case OP_SMUL_C:
        // PRI = PRI * value
        asm_.imul(eax, instr.operand());
        break;
      case OP_ZERO_PRI:
        // PRI = 0
        asm_.xor_(eax, eax);
        break;
      case OP_ZERO_ALT:
        // ALT = 0
        asm_.xor_(ecx, ecx);
        break;
      case OP_ZERO:
        // [address] = 0
        asm_.mov(dword_ptr(ebx, instr.operand()), 0);
        break;
      case OP_ZERO_S:
        // [FRM + offset] = 0
        asm_.mov(dword_ptr(ebp, instr.operand()), 0);
        break;
      case OP_SIGN_PRI:
        // sign extent the byte in PRI to a cell
        asm_.movsx(eax, al);
        break;
      case OP_SIGN_ALT:
        // sign extent the byte in ALT to a cell
        asm_.movsx(ecx, cl);
        break;
      case OP_EQ:
        // PRI = PRI == ALT ? 1 :
        asm_.cmp(eax, ecx);
        asm_.sete(al);
        asm_.movzx(eax, al);
        break;
      case OP_NEQ:
        // PRI = PRI != ALT ? 1 :
        asm_.cmp(eax, ecx);
        asm_.setne(al);
        asm_.movzx(eax, al);
        break;
      case OP_LESS:
        // PRI = PRI < ALT ? 1 :
        asm_.cmp(eax, ecx);
        asm_.setb(al);
        asm_.movzx(eax, al);
        break;
      case OP_LEQ:
        // PRI = PRI <= ALT ? 1 :
        asm_.cmp(eax, ecx);
        asm_.setbe(al);
        asm_.movzx(eax, al);
        break;
      case OP_GRTR:
        // PRI = PRI > ALT ? 1 :
        asm_.cmp(eax, ecx);
        asm_.seta(al);
        asm_.movzx(eax, al);
        break;
      case OP_GEQ:
        // PRI = PRI >= ALT ? 1 :
        asm_.cmp(eax, ecx);
        asm_.setae(al);
        asm_.movzx(eax, al);
        break;
      case OP_SLESS:
        // PRI = PRI < ALT ? 1 :
        asm_.cmp(eax, ecx);
        asm_.setl(al);
        asm_.movzx(eax, al);
        break;
      case OP_SLEQ:
        // PRI = PRI <= ALT ? 1 :
        asm_.cmp(eax, ecx);
        asm_.setle(al);
        asm_.movzx(eax, al);
        break;
      case OP_SGRTR:
        // PRI = PRI > ALT ? 1 :
        asm_.cmp(eax, ecx);
        asm_.setg(al);
        asm_.movzx(eax, al);
        break;
      case OP_SGEQ:
        // PRI = PRI >= ALT ? 1 :
        asm_.cmp(eax, ecx);
        asm_.setge(al);
        asm_.movzx(eax, al);
        break;
      case OP_EQ_C_PRI:
        // PRI = PRI == value ? 1 :
        asm_.cmp(eax, instr.operand());
        asm_.sete(al);
        asm_.movzx(eax, al);
        break;
      case OP_EQ_C_ALT:
        // PRI = ALT == value ? 1 :
        asm_.cmp(ecx, instr.operand());
        asm_.sete(al);
        asm_.movzx(eax, al);
        break;
      case OP_INC_PRI:
        // PRI = PRI + 1
        asm_.inc(eax);
        break;
      case OP_INC_ALT:
        // ALT = ALT + 1
        asm_.inc(ecx);
        break;
      case OP_INC:
        // [address] = [address] + 1
        asm_.inc(dword_ptr(ebx, instr.operand()));
        break;
      case OP_INC_S:
        // [FRM + offset] = [FRM + offset] + 1
        asm_.inc(dword_ptr(ebp, instr.operand()));
        break;
      case OP_INC_I:
        // [PRI] = [PRI] + 1
        asm_.inc(dword_ptr(ebx, eax));
        break;
      case OP_DEC_PRI:
        // PRI = PRI - 1
        asm_.dec(eax);
        break;
      case OP_DEC_ALT:
        // ALT = ALT - 1
        asm_.dec(ecx);
        break;
      case OP_DEC:
        // [address] = [address] - 1
        asm_.dec(dword_ptr(ebx, instr.operand()));
        break;
      case OP_DEC_S:
        // [FRM + offset] = [FRM + offset] - 1
        asm_.dec(dword_ptr(ebp, instr.operand()));
        break;
      case OP_DEC_I:
        // [PRI] = [PRI] - 1
        asm_.dec(dword_ptr(ebx, eax));
        break;
      case OP_MOVS: {
        // Copy memory from [PRI] to [ALT]. The parameter
        // specifies the number of bytes. The blocks should not
        // overlap.
        cell num_bytes = instr.operand();
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
        break;
      }
      case OP_CMPS: {
        // Compare memory blocks at [PRI] and [ALT]. The parameter
        // specifies the number of bytes. The blocks should not
        // overlap.
        cell num_bytes = instr.operand();
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
        break;
      }
      case OP_FILL: {
        // Fill memory at [ALT] with value in [PRI]. The parameter
        // specifies the number of bytes, which must be a multiple
        // of the cell size.
        cell num_bytes = instr.operand();
        asm_.lea(edi, dword_ptr(ebx, ecx));
        asm_.push(ecx);
        asm_.mov(ecx, num_bytes / sizeof(cell));
        asm_.rep_stosd();
        asm_.pop(ecx);
        break;
      }
      case OP_HALT:
        // Abort execution (exit value in PRI), parameters other than 0
        // have a special meaning.
        asm_.mov(edi, instr.operand());
        asm_.call(halt_helper_label_);
        break;
      case OP_BOUNDS: {
        // Abort execution if PRI > value or if PRI < 0.
        Label halt_label = asm_.newLabel();
        Label exit_label = asm_.newLabel();
          asm_.cmp(eax, instr.operand());
          asm_.jg(halt_label);
          asm_.test(eax, eax);
          asm_.jl(halt_label);
          asm_.jmp(exit_label);
        asm_.bind(halt_label);
          EmitDebugBreakpoint();
          asm_.mov(edi, AMX_ERR_BOUNDS);
          asm_.call(halt_helper_label_);
        asm_.bind(exit_label);
        break;
      }
      case OP_SYSREQ_PRI:
        // Call system service, service number in PRI.
        asm_.push(eax);
        asm_.call(sysreq_c_helper_label_);
        break;
      case OP_SYSREQ_C: {
        // Call system service.
        const char *name = amx.GetNativeName(instr.operand());
        if (name == 0) {
          error = true;
        } else {
          bool handled = EmitIntrinsic(name);
          if (!handled && amx->sysreq_d && enable_sysreq_d_) {
            // Optimization: if we already know the address we can call this
            // native function directly.
            cell address = amx.GetNativeAddress(instr.operand());
            if (address != 0) {
              // Sometimes the address can be 0: for example, when a function
              // is registered _after_ JIT compilation (could be a plugin).
              asm_.push(address);
              asm_.call(sysreq_d_helper_label_);
              handled = true;
            }
          }
          if (!handled) {
            asm_.push(instr.operand());
            asm_.call(sysreq_c_helper_label_);
          }
        }
        break;
      }
      case OP_SYSREQ_D: {
        // Call system service.
        const char *name = amx.GetNativeName(amx.FindNative(instr.operand()));
        if (name == 0) {
          error = true;
        } else {
          if (!EmitIntrinsic(name)) {
            asm_.push(instr.operand());
            asm_.call(sysreq_d_helper_label_);
          }
        }
        break;
      }
      case OP_SWITCH: {
        // Compare PRI to the values in the case table (whose address
        // is passed as an offset from CIP) and jump to the associated
        // address in the matching record.
        CaseTable case_table(amx, instr.operand());

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
        break;
      }
      case OP_CASETBL:
        // A variable number of case records follows this opcode, where
        // each record takes two cells.
        break;
      case OP_SWAP_PRI:
        // [STK] = PRI and PRI = [STK]
        asm_.xchg(dword_ptr(esp), eax);
        break;
      case OP_SWAP_ALT:
        // [STK] = ALT and ALT = [STK]
        asm_.xchg(dword_ptr(esp), ecx);
        break;
      case OP_PUSH_ADR:
        // [STK] = FRM + offset, STK = STK - cell size
        asm_.lea(edx, dword_ptr(ebp, instr.operand()));
        asm_.sub(edx, ebx);
        asm_.push(edx);
        break;
      case OP_NOP:
        // No-operation, for code alignment.
        break;
      case OP_BREAK:
        // Conditional breakpoint.
        EmitDebugBreakpoint();
        break;
    default:
      error = true;
    }
  }

  if (error && error_handler_ != 0) {
    error_handler_->Execute(instr);
  }

  CodeBuffer *code_buffer = 0;

  if (!error) {
    void *code_blob = asm_.make();
    code_buffer = new CodeBuffer(code_blob);

    RuntimeInfoBlock *rib = reinterpret_cast<RuntimeInfoBlock*>(code_blob);
    rib->amx = reinterpret_cast<intptr_t>(amx_.raw());
    rib->exec += reinterpret_cast<intptr_t>(code_blob);
    rib->instr_table += reinterpret_cast<intptr_t>(code_blob);

    InstrTableEntry *ite =
      reinterpret_cast<InstrTableEntry*>(rib->instr_table);
    for (std::map<cell, std::ptrdiff_t>::const_iterator it = instr_map_.begin();
         it != instr_map_.end(); it++) {
      ite->address = it->first;
      ite->ptr = reinterpret_cast<uintptr_t>(code_blob) + it->second;
      ite++;
    }
  }

  amx_.Reset();

  if (asmjit_logger_ != 0) {
    asm_.setLogger(0);
    delete asmjit_logger_;
  }

  return code_buffer;
}

void CompilerImpl::float_() {
  // Float:float(value)
  asm_.fild(dword_ptr(esp, 4));
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerImpl::floatabs() {
  // Float:floatabs(Float:value)
  asm_.fld(dword_ptr(esp, 4));
  asm_.fabs();
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerImpl::floatadd() {
  // Float:floatadd(Float:oper1, Float:oper2)
  asm_.fld(dword_ptr(esp, 4));
  asm_.fadd(dword_ptr(esp, 8));
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerImpl::floatsub() {
  // Float:floatsub(Float:oper1, Float:oper2)
  asm_.fld(dword_ptr(esp, 4));
  asm_.fsub(dword_ptr(esp, 8));
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerImpl::floatmul() {
  // Float:floatmul(Float:oper1, Float:oper2)
  asm_.fld(dword_ptr(esp, 4));
  asm_.fmul(dword_ptr(esp, 8));
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerImpl::floatdiv() {
  // Float:floatdiv(Float:dividend, Float:divisor)
  asm_.fld(dword_ptr(esp, 4));
  asm_.fdiv(dword_ptr(esp, 8));
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerImpl::floatsqroot() {
  // Float:floatsqroot(Float:value)
  asm_.fld(dword_ptr(esp, 4));
  asm_.fsqrt();
  asm_.sub(esp, 4);
  asm_.fstp(dword_ptr(esp));
  asm_.mov(eax, dword_ptr(esp));
  asm_.add(esp, 4);
}

void CompilerImpl::floatlog() {
  // Float:floatlog(Float:value, Float:base=10.0)
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

void CompilerImpl::floatcmp() {
  // floatcmp(Float:oper1, Float:oper2)
  asmjit::Label less_or_greater = asm_.newLabel();
  asmjit::Label less = asm_.newLabel();
  asmjit::Label exit = asm_.newLabel();

    asm_.fld(dword_ptr(esp, 8));
    asm_.fld(dword_ptr(esp, 4));
    asm_.fcompp();
    asm_.fnstsw(ax);

    asm_.test(ah, 0x44); // C2 + C3
    asm_.jp(less_or_greater);
    asm_.xor_(eax, eax);
    asm_.jmp(exit);

  asm_.bind(less_or_greater);
    asm_.test(ah, 0x01); // C0
    asm_.jnz(less);
    asm_.mov(eax, 1);
    asm_.jmp(exit);

  asm_.bind(less);
    asm_.mov(eax, -1);

  asm_.bind(exit);
}

void CompilerImpl::heapspace() {
  // PRI = STL - HEA
  asm_.mov(edx, dword_ptr(amx_ptr_label_));
  asm_.mov(edx, dword_ptr(edx, offsetof(AMX, hea)));
  asm_.mov(eax, esp);
  asm_.sub(eax, ebx);
  asm_.sub(eax, edx);
}

void CompilerImpl::clamp() {
  asmjit::Label exit = asm_.newLabel();
    // Load the value being compared.
    asm_.mov(edx, dword_ptr(esp, 4));

    // If it is lower than the lower bound, return the lower bound.
    asm_.mov(eax, dword_ptr(esp, 8));
    asm_.cmp(edx, eax);
    asm_.jle(exit);

    // If it is higher than the higher bound, return the higher bound.
    asm_.mov(eax, dword_ptr(esp, 12));
    asm_.cmp(edx, eax);
    asm_.jge(exit);

    // Otherwise, return the number.
    asm_.mov(eax, edx);
  asm_.bind(exit);
}

void CompilerImpl::numargs() {
  asm_.mov(eax, dword_ptr(ebp, 8));
  asm_.shr(eax, 2);
}

void CompilerImpl::min() {
  asmjit::Label exit = asm_.newLabel();
    asm_.mov(eax, dword_ptr(esp, 4));
    asm_.mov(edx, dword_ptr(esp, 8));
    asm_.cmp(edx, eax);
    asm_.jge(exit);
    asm_.mov(eax, edx);
  asm_.bind(exit);
}

void CompilerImpl::max() {
  asmjit::Label exit = asm_.newLabel();
    asm_.mov(eax, dword_ptr(esp, 4));
    asm_.mov(edx, dword_ptr(esp, 8));
    asm_.cmp(edx, eax);
    asm_.jle(exit);
    asm_.mov(eax, edx);
  asm_.bind(exit);
}

void CompilerImpl::swapchars() {
  asm_.mov(eax, dword_ptr(esp, 4));
  asm_.xchg(ah, al);
  asm_.ror(eax, 16);
  asm_.xchg(ah, al);
}

bool CompilerImpl::EmitIntrinsic(const char *name) {
  typedef void (CompilerImpl::*EmitIntrinsicMethod)();

  struct Intrinsic {
    const char *name;
    EmitIntrinsicMethod emit;
  };

  static const Intrinsic intrinsics[] = {
    // float.inc
    {"float",       &CompilerImpl::float_},
    {"floatabs",    &CompilerImpl::floatabs},
    {"floatadd",    &CompilerImpl::floatadd},
    {"floatsub",    &CompilerImpl::floatsub},
    {"floatmul",    &CompilerImpl::floatmul},
    {"floatdiv",    &CompilerImpl::floatdiv},
    {"floatsqroot", &CompilerImpl::floatsqroot},
    {"floatcmp",    &CompilerImpl::floatcmp},
    // core.inc
    {"clamp",       &CompilerImpl::clamp},
    {"heapspace",   &CompilerImpl::heapspace},
    {"numargs",     &CompilerImpl::numargs},
    {"min",         &CompilerImpl::min},
    {"max",         &CompilerImpl::max},
    {"swapchars",   &CompilerImpl::swapchars}
  };

  for (std::size_t i = 0; i < sizeof(intrinsics) / sizeof(*intrinsics); i++) {
    if (std::strcmp(intrinsics[i].name, name) == 0) {
      (this->*intrinsics[i].emit)();
      return true;
    }
  }

  return false;
}

void CompilerImpl::EmitRuntimeInfo() {
  asm_.bind(rib_start_label_);
  asm_.bind(exec_ptr_label_);
    asm_.dd(0); // rib->exec
  asm_.bind(amx_ptr_label_);
    asm_.dd(0); // rib->amx_ptr
  asm_.bind(ebp_label_);
    asm_.dd(0); // rib->ebp
  asm_.bind(esp_label_);
    asm_.dd(0); // rib->esp
  asm_.bind(amx_ebp_label_);
    asm_.dd(0); // rib->amx_ebp
  asm_.bind(amx_esp_label_);
    asm_.dd(0); // rib->amx_esp
  asm_.bind(reset_ebp_label_);
    asm_.dd(0); // rib->reset_ebp
  asm_.bind(reset_esp_label_);
    asm_.dd(0); // rib->reset_esp
  asm_.bind(reset_stk_label_);
    asm_.dd(0); // rib->reset_stk
  asm_.bind(reset_hea_label_);
    asm_.dd(0); // rib->reset_hea
    asm_.dd(0); // rib->instr_map
    asm_.dd(0); // rib->instr_map_size
}

void CompilerImpl::EmitInstrTable() {
  int num_entries = 0;

  Instruction instr;
  Disassembler disasm(amx_);
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
void CompilerImpl::EmitExec() {
  RuntimeInfoBlock *rib = reinterpret_cast<RuntimeInfoBlock*>(asm_.getBuffer());
  rib->exec = asm_.getCodeSize();

  Label null_data_label = asm_.newLabel();
  Label stack_heap_overflow_label = asm_.newLabel();
  Label heap_underflow_label = asm_.newLabel();
  Label stack_underflow_label = asm_.newLabel();
  Label native_not_found_label = asm_.newLabel();
  Label public_not_found_label = asm_.newLabel();
  Label after_call_label_ = asm_.newLabel();
  Label finish_label = asm_.newLabel();
  Label return_label = asm_.newLabel();
  Label continue_from_sleep_label = asm_.newLabel();

  int arg_index = 8;
  int arg_retval = 12;
  int var_address = -4;
  int var_reset_ebp = -8;
  int var_reset_esp = -12;
  int var_reset_stk = -16;
  int var_reset_hea = -20;

  asm_.bind(exec_label_);
    asm_.push(ebp);
    asm_.mov(ebp, esp);

    // Allocate space for the local variables.
    asm_.sub(esp, 20);

    asm_.push(esi);
    asm_.mov(esi, dword_ptr(amx_ptr_label_));

    // Set ebx = address of the AMX data section.
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

    if (enable_sleep_) {
      // Handle AMX_EXEC_CONT case.
      asm_.mov(ecx, dword_ptr(ebp, arg_index));
      asm_.mov(edx, AMX_EXEC_CONT);
      asm_.cmp(ecx, edx);
      asm_.je(continue_from_sleep_label);
    }

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
    asm_.call(reinterpret_cast<asmjit::Ptr>(&GetJITInstrPtr));
    asm_.add(esp, 8);
    asm_.mov(dword_ptr(ebp, var_address), eax);

    // Save the old reset_ebp and reset_esp on the stack.
    asm_.mov(eax, dword_ptr(reset_ebp_label_));
    asm_.mov(dword_ptr(ebp, var_reset_ebp), eax);
    asm_.mov(eax, dword_ptr(reset_esp_label_));
    asm_.mov(dword_ptr(ebp, var_reset_esp), eax);

    // Now do the same for reset_stk and reset_hea.
    asm_.mov(eax, dword_ptr(reset_stk_label_));
    asm_.mov(dword_ptr(ebp, var_reset_stk), eax);
    asm_.mov(eax, dword_ptr(reset_hea_label_));
    asm_.mov(dword_ptr(ebp, var_reset_hea), eax);

    // Call the function.
    asm_.push(dword_ptr(ebp, var_address));
    asm_.call(exec_helper_label_);
    asm_.add(esp, 4);

  asm_.bind(after_call_label_);
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

    // Again, do the same for reset_stk and reset_hea.
    asm_.mov(eax, dword_ptr(ebp, var_reset_stk));
    asm_.mov(dword_ptr(reset_stk_label_), eax);
    asm_.mov(eax, dword_ptr(ebp, var_reset_hea));
    asm_.mov(dword_ptr(reset_hea_label_), eax);

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

    if (enable_sleep_) {
    asm_.bind(continue_from_sleep_label);
      asm_.call(exec_cont_helper_label_);
      asm_.jmp(after_call_label_);
    }
}

// cell AMXJIT_CDECL ExecHelper(void *address);
void CompilerImpl::EmitExecHelper() {
  Label call_return_label = asm_.newLabel();

  asm_.bind(exec_helper_label_);
    EmitDebugPrint("ExecHelper()");

    // Store the address in eax.
    asm_.mov(eax, dword_ptr(esp, 4));

    // We must preserve these registers because they are callee-saved in x86
    // code (cdecl).
    //
    // https://en.wikipedia.org/wiki/X86_calling_conventions#Caller_clean-up
    asm_.push(esi);
    asm_.push(edi);

    // Save current stack pointers on the stack.
    asm_.push(dword_ptr(ebp_label_));
    asm_.push(dword_ptr(esp_label_));
    asm_.push(dword_ptr(amx_ebp_label_));
    asm_.push(dword_ptr(amx_esp_label_));

    // Push parameters size to the AMX stack and reset the parameter count.
    //
    // cell new_stk = amx->stk - sizeof(cell);
    // *(data + new_stk) = amx->paramcount * sizeof(cell);
    // amx->paramcount = 0;
    // reset_stk += amx->paramcount * sizeof(cell);
    // reset_hea = amx->hea;
    //
    asm_.mov(ecx, dword_ptr(amx_ptr_label_));
    asm_.mov(edx, dword_ptr(ecx, offsetof(AMX, hea)));
    asm_.mov(dword_ptr(reset_hea_label_), edx);
    asm_.mov(esi, dword_ptr(ecx, offsetof(AMX, paramcount)));
    asm_.imul(esi, esi, sizeof(cell));
    asm_.mov(edx, dword_ptr(ecx, offsetof(AMX, stk)));
    asm_.lea(edi, dword_ptr(edx, esi));
    asm_.mov(dword_ptr(reset_stk_label_), edi);
    asm_.sub(edx, sizeof(cell));
    asm_.mov(dword_ptr(ebx, edx), esi);
    asm_.mov(dword_ptr(ecx, offsetof(AMX, paramcount)), 0);

    // Switch to the AMX stack.
    asm_.mov(dword_ptr(esp_label_), esp);
    asm_.add(edx, ebx);
    asm_.mov(dword_ptr(amx_esp_label_), edx);
    asm_.mov(esp, edx); // esp = data + new_stk
    asm_.mov(dword_ptr(ebp_label_), ebp);
    asm_.mov(edx, dword_ptr(ecx, offsetof(AMX, frm)));
    asm_.add(edx, ebx);
    asm_.mov(dword_ptr(amx_ebp_label_), edx);
    asm_.mov(ebp, edx); // ebp = data + amx->frm

    // To make "halt" work we must be able to return here later.
    // Subtract 4 bytes to keep the return address on the stack after return.
    asm_.lea(ecx, dword_ptr(esp, -4));
    asm_.mov(dword_ptr(reset_esp_label_), ecx);
    asm_.mov(dword_ptr(reset_ebp_label_), ebp);

    // Call the function. Prior to this point ebx should point to the data
    // section and both stack pointers should point somewhere on the AMX stack.
    // asm_.lea(ecx, dword_ptr(halt_helper_label_));
    // asm_.push(ecx);
    asm_.call(eax);

  asm_.bind(call_return_label);
    asm_.mov(esi, dword_ptr(amx_ptr_label_));
    asm_.mov(edi, dword_ptr(esi, offsetof(AMX, error)));

    if (enable_sleep_) {
      asm_.cmp(edi, AMX_ERR_SLEEP);
      asm_.jnz(exec_exit_label_);
    }

    // Sync AMX registers on successful exit. If there was an error, they have
    // been already synced by HaltHelper.
    asm_.mov(dword_ptr(esi, offsetof(AMX, pri)), eax);
    asm_.mov(dword_ptr(esi, offsetof(AMX, alt)), ecx);
    asm_.mov(edx, ebp);
    asm_.sub(edx, ebx);
    asm_.mov(dword_ptr(esi, offsetof(AMX, frm)), edx);
    asm_.mov(edx, esp);
    asm_.sub(edx, ebx);
    asm_.mov(dword_ptr(esi, offsetof(AMX, stk)), edx);
    // HEA is already in sync.

  asm_.bind(exec_exit_label_);
    // Switch back to the native stack.
    asm_.mov(dword_ptr(amx_ebp_label_), ebp);
    asm_.mov(ebp, dword_ptr(ebp_label_));
    asm_.mov(dword_ptr(amx_esp_label_), esp);
    asm_.mov(esp, dword_ptr(esp_label_));

    asm_.pop(dword_ptr(amx_esp_label_));
    asm_.pop(dword_ptr(amx_ebp_label_));
    asm_.pop(dword_ptr(esp_label_));
    asm_.pop(dword_ptr(ebp_label_));

    EmitDebugPrint("ExecHelper: returned from entry point");

    asm_.pop(edi);
    asm_.pop(esi);
    asm_.ret();
}

// cell AMXJIT_CDECL ExecContHelper();
void CompilerImpl::EmitExecContHelper() {
  // Execute code after returning from sleep (index = AMX_EXEC_CONT).
  //
  // frm = amx->frm;
  // stk = amx->stk;
  // hea = amx->hea;
  // pri = amx->pri;
  // alt = amx->alt;
  // reset_stk = amx->reset_stk;
  // reset_hea = amx->reset_hea;
  // cip = (cell *)(code + (int)amx->cip);

  asm_.bind(exec_cont_helper_label_);
    EmitDebugPrint("ExecContHelper()");

    // Store the address in eax.
    asm_.mov(eax, dword_ptr(esp, 4));

    asm_.push(esi);
    asm_.push(edi);

    // Save current stack pointers on the stack.
    asm_.push(dword_ptr(ebp_label_));
    asm_.push(dword_ptr(esp_label_));
    asm_.push(dword_ptr(amx_ebp_label_));
    asm_.push(dword_ptr(amx_esp_label_));

    // Switch to the AMX stack.
    asm_.mov(ecx, dword_ptr(amx_ptr_label_));
    asm_.mov(dword_ptr(ebp_label_), ebp);
    asm_.mov(edx, dword_ptr(ecx, offsetof(AMX, frm)));
    asm_.add(edx, ebx);
    asm_.mov(dword_ptr(amx_ebp_label_), edx);
    asm_.mov(ebp, edx); // ebp = data + amx->frm
    asm_.mov(dword_ptr(esp_label_), esp);
    asm_.mov(edx, dword_ptr(ecx, offsetof(AMX, stk)));
    asm_.add(edx, ebx);
    asm_.mov(dword_ptr(amx_esp_label_), edx);
    asm_.mov(esp, edx); // esp = data + amx->stk

    // To make "halt" work we must be able to return here later.
    // Subtract 4 bytes to keep the return address on the stack after return.
    asm_.lea(ecx, dword_ptr(esp, -4));
    asm_.mov(dword_ptr(reset_esp_label_), ecx);
    asm_.mov(dword_ptr(reset_ebp_label_), ebp);

    // Restore registers and jump to the address saved before entering sleep
    // mode (amx->cip). Stack registers - STK and FRM - have already been
    // restored by the preceding stack switching code above at this point, so
    // we don't touch them here.
    asm_.mov(eax, dword_ptr(esi, offsetof(AMX, reset_stk)));
    asm_.mov(dword_ptr(reset_stk_label_), eax);
    asm_.mov(eax, dword_ptr(esi, offsetof(AMX, reset_hea)));
    asm_.mov(dword_ptr(reset_hea_label_), eax);
    asm_.mov(eax, dword_ptr(esi, offsetof(AMX, cip)));
    asm_.call(jump_lookup_label_);
    asm_.mov(edx, eax); // address
    asm_.mov(eax, dword_ptr(esi, offsetof(AMX, pri)));
    asm_.mov(ecx, dword_ptr(esi, offsetof(AMX, alt)));
    asm_.jmp(edx);
}

// void HaltHelper(int error [edi]);
void CompilerImpl::EmitHaltHelper() {
  Label sleep_label = asm_.newLabel();
  Label exit_label = asm_.newLabel();

  asm_.bind(halt_helper_label_);
    EmitDebugPrint("HaltHelper()");

    // amx->error = error (edi);
    // amx->pri = pri;
    // amx->alt = alt;
    // amx->frm = frm;
    // amx->stk = stk;
    asm_.mov(esi, dword_ptr(amx_ptr_label_));
    asm_.mov(dword_ptr(esi, offsetof(AMX, error)), edi);
    asm_.mov(dword_ptr(esi, offsetof(AMX, pri)), eax);
    asm_.mov(dword_ptr(esi, offsetof(AMX, alt)), ecx);
    asm_.pop(ecx); // return address
    asm_.mov(edx, ebp);
    asm_.sub(edx, ebx);
    asm_.mov(dword_ptr(esi, offsetof(AMX, frm)), edx);
    asm_.mov(edx, esp);
    asm_.sub(edx, ebx);
    asm_.mov(dword_ptr(esi, offsetof(AMX, stk)), edx);
    // HEA is already in sync.

    if (enable_sleep_) {
      // Handle error == AMX_ERR_SLEEP case.
      asm_.cmp(edi, AMX_ERR_SLEEP);
      asm_.je(sleep_label);
    }

  asm_.bind(exit_label);
    // Reset the stack and return back to ExecHelper().
    asm_.mov(esp, dword_ptr(reset_esp_label_));
    asm_.mov(ebp, dword_ptr(reset_ebp_label_));
    asm_.jmp(exec_exit_label_);

    if (enable_sleep_) {
    asm_.bind(sleep_label);
      // amx->cip = ReverseJumpLookup(return_address);
      // amx->reset_stk = reset_stk;
      // amx->reset_hea = reset_hea;
      asm_.mov(eax, ecx);
      asm_.call(reverse_jump_lookup_label_);
      asm_.mov(dword_ptr(esi, offsetof(AMX, cip)), eax);
      asm_.mov(edx, dword_ptr(reset_stk_label_));
      asm_.mov(dword_ptr(esi, offsetof(AMX, reset_stk)), edx);
      asm_.mov(edx, dword_ptr(reset_hea_label_));
      asm_.mov(dword_ptr(esi, offsetof(AMX, reset_hea)), edx);
      asm_.jmp(exit_label);
    }
}

// void JumpHelper(void *address [eax]);
void CompilerImpl::EmitJumpHelper() {
  Label invalid_address_label = asm_.newLabel();

  asm_.bind(jump_helper_label_);
    asm_.push(eax);
    asm_.call(jump_lookup_label_);
    asm_.mov(edx, eax); // address
    asm_.pop(eax);

    asm_.test(edx, edx);
    asm_.jz(invalid_address_label);

    asm_.lea(esp, dword_ptr(esp, 4));
    asm_.jmp(edx);

  // Continue execution as if there was no jump at all (this is what AMX does).
  asm_.bind(invalid_address_label);
    asm_.ret();
}

// void JumpLookup(void *address [eax]);
void CompilerImpl::EmitJumpLookup() {
  asm_.bind(jump_lookup_label_);
    asm_.push(ecx);
    asm_.push(edx);

    asm_.lea(ecx, dword_ptr(rib_start_label_));
    asm_.push(ecx);
    asm_.push(eax);
    asm_.call(reinterpret_cast<asmjit::Ptr>(&GetJITInstrPtr));
    asm_.add(esp, 8);

    asm_.pop(edx);
    asm_.pop(ecx);
    asm_.ret();
}

// void ReverseJumpLookup(void *address [eax]);
void CompilerImpl::EmitReverseJumpLookup() {
  asm_.bind(reverse_jump_lookup_label_);
    asm_.push(ecx);
    asm_.push(edx);

    asm_.lea(ecx, dword_ptr(rib_start_label_));
    asm_.push(ecx);
    asm_.push(eax);
    asm_.call(reinterpret_cast<asmjit::Ptr>(&GetANXAddressByJITInstrPtr));
    asm_.add(esp, 8);

    asm_.pop(edx);
    asm_.pop(ecx);
    asm_.ret();
}

// cell AMXJIT_STDCALL SysreqCHelper(int index);
void CompilerImpl::EmitSysreqCHelper() {
  Label error_label = asm_.newLabel();
  Label sleep_error_label = asm_.newLabel();

  // Can modify registers: eax, edx, esi, edi
  // eax = return value (PRI)

  asm_.bind(sysreq_c_helper_label_);
    EmitDebugPrint("sysreq.c");

    asm_.mov(edx, dword_ptr(amx_ptr_label_));

    if (enable_sleep_) {
      // Copy CIP to amx->cip (needed for "sleep" to work).
      asm_.mov(eax, dword_ptr(esp));  // return address
      asm_.call(reverse_jump_lookup_label_);
      asm_.mov(dword_ptr(edx, offsetof(AMX, cip)), eax);
    }

    asm_.pop(esi); // return address
    asm_.pop(eax); // index
    asm_.lea(edi, dword_ptr(esp)); // params

    // Switch to the native stack.
    asm_.mov(dword_ptr(amx_ebp_label_), ebp);
    asm_.sub(ebp, ebx);
    asm_.mov(dword_ptr(edx, offsetof(AMX, frm)), ebp); // amx->frm = ebp - data
    asm_.mov(ebp, dword_ptr(ebp_label_));
    asm_.mov(dword_ptr(amx_esp_label_), esp);
    asm_.sub(esp, ebx);
    asm_.mov(dword_ptr(edx, offsetof(AMX, stk)), esp); // amx->stk = esp - data
    asm_.mov(esp, dword_ptr(esp_label_));
    asm_.push(dword_ptr(amx_esp_label_));
    asm_.push(dword_ptr(amx_ebp_label_));

    // Push ALT to the stack (not enough registers to keep it around).
    asm_.push(ecx);

    // Allocate space for the result parameter of amx_Callback().
    asm_.push(0);
    asm_.mov(ecx, esp);

    // Call the native function via amx->callback.
    asm_.push(edi); // params
    asm_.push(ecx); // result
    asm_.push(eax); // index
    asm_.push(edx); // amx
    asm_.call(dword_ptr(edx, offsetof(AMX, callback)));
    asm_.mov(edi, eax);
    asm_.mov(eax, dword_ptr(ecx)); // eax = *result
    asm_.add(esp, 20);

    asm_.pop(ecx); // ALT

    // Switch back to the AMX stack.
    asm_.pop(edx);
    asm_.mov(ebp, edx);
    asm_.pop(edx);
    asm_.mov(esp, edx);

    // Check return value for errors and leave.
    if (enable_sleep_) {
      asm_.cmp(edi, AMX_ERR_SLEEP);
      asm_.je(sleep_error_label);
    }
    asm_.cmp(edi, AMX_ERR_NONE);
    asm_.jne(error_label);
    asm_.push(esi); // return address
    asm_.ret();

  asm_.bind(error_label);
    asm_.call(halt_helper_label_);

    if (enable_sleep_) {
    asm_.bind(sleep_error_label);
      // Enter sleep mode (return value = AMX_ERR_SLEEP).
      //
      // amx->cip = (cell)((unsigned char *)cip-code);
      // amx->hea = hea;
      // amx->frm = frm;
      // amx->stk = stk;
      // amx->pri = pri;
      // amx->alt = alt;
      // amx->reset_stk = reset_stk;
      // amx->reset_hea = reset_hea;
      EmitDebugPrint("AMX_ERR_SLEEP");

      asm_.mov(edx, dword_ptr(amx_ptr_label_));
      asm_.mov(dword_ptr(edx, offsetof(AMX, pri)), eax);
      asm_.mov(dword_ptr(edx, offsetof(AMX, alt)), ecx);
      asm_.mov(ecx, dword_ptr(reset_stk_label_));
      asm_.mov(dword_ptr(edx, offsetof(AMX, reset_stk)), ecx);
      asm_.mov(ecx, dword_ptr(reset_hea_label_));
      asm_.mov(dword_ptr(edx, offsetof(AMX, reset_hea)), ecx);

      asm_.jmp(exec_exit_label_);
    }
}

// cell AMXJIT_STDCALL SysreqDHelper(void *address);
void CompilerImpl::EmitSysreqDHelper() {
  Label error_label = asm_.newLabel();
  Label sleep_error_label = asm_.newLabel();

  asm_.bind(sysreq_d_helper_label_);
    EmitDebugPrint("sysreq.d");

    asm_.mov(edx, dword_ptr(amx_ptr_label_));

    if (enable_sleep_) {
      // Save CIP to amx->cip (needed for sleep to work).
      asm_.mov(eax, dword_ptr(esp));  // return address
      asm_.call(reverse_jump_lookup_label_);
      asm_.mov(dword_ptr(edx, offsetof(AMX, cip)), eax);
    }

    asm_.pop(esi); // return address
    asm_.pop(eax); // address
    asm_.lea(edi, dword_ptr(esp)); // params

    // Switch to the native stack.
    asm_.mov(dword_ptr(amx_ebp_label_), ebp);
    asm_.sub(ebp, ebx);
    asm_.mov(dword_ptr(edx, offsetof(AMX, frm)), ebp); // amx->frm = ebp - data
    asm_.mov(ebp, dword_ptr(ebp_label_));
    asm_.mov(dword_ptr(amx_esp_label_), esp);
    asm_.sub(esp, ebx);
    asm_.mov(dword_ptr(edx, offsetof(AMX, stk)), esp); // amx->stk = esp - data
    asm_.mov(esp, dword_ptr(esp_label_));
    asm_.push(dword_ptr(amx_esp_label_));
    asm_.push(dword_ptr(amx_ebp_label_));

    // Push ALT to the stack (not enough registers to keep it around).
    asm_.push(ecx); // ALT

    // Call the native function.
    asm_.push(edi); // params
    asm_.push(edx); // amx
    asm_.call(eax); // address
    asm_.add(esp, 8);
    // At this point the return value is stored in eax.

    asm_.pop(ecx); // ALT

    // Switch back to the AMX stack.
    asm_.pop(edx);
    asm_.mov(ebp, edx);
    asm_.pop(edx);
    asm_.mov(esp, edx);

    asm_.mov(edx, dword_ptr(amx_ptr_label_));
    asm_.mov(edi, dword_ptr(edx, offsetof(AMX, error)));

    // Check for errors and leave.
    if (enable_sleep_) {
      asm_.cmp(edi, AMX_ERR_SLEEP);
      asm_.je(sleep_error_label);
    }
    asm_.cmp(edi, AMX_ERR_NONE);
    asm_.jne(error_label);
    asm_.push(esi); // return address
    asm_.ret();

  asm_.bind(error_label);
    asm_.call(halt_helper_label_);

    if (enable_sleep_) {
    asm_.bind(sleep_error_label);
      // Enter sleep mode (amx->error = AMX_ERR_SLEEP).
      //
      // amx->cip = (cell)((unsigned char *)cip-code);
      // amx->hea = hea;
      // amx->frm = frm;
      // amx->stk = stk;
      // amx->pri = pri;
      // amx->alt = alt;
      // amx->reset_stk = reset_stk;
      // amx->reset_hea = reset_hea;
      EmitDebugPrint("AMX_ERR_SLEEP");

      asm_.mov(edx, dword_ptr(amx_ptr_label_));
      asm_.mov(dword_ptr(edx, offsetof(AMX, pri)), eax);
      asm_.mov(dword_ptr(edx, offsetof(AMX, alt)), ecx);
      asm_.mov(ecx, dword_ptr(reset_stk_label_));
      asm_.mov(dword_ptr(edx, offsetof(AMX, reset_stk)), ecx);
      asm_.mov(ecx, dword_ptr(reset_hea_label_));
      asm_.mov(dword_ptr(edx, offsetof(AMX, reset_hea)), ecx);

      asm_.jmp(exec_exit_label_);
    }
}

void CompilerImpl::EmitDebugPrint(const char *message) {
  if (debug_flags_ & DEBUG_LOGGING) {
    asm_.push(eax);
    asm_.push(edx);
    asm_.push(ecx);
    asm_.push((intptr_t)message);
    asm_.call((intptr_t)puts);
    asm_.add(esp, 4);
    asm_.pop(ecx);
    asm_.pop(edx);
    asm_.pop(eax);
  }
}

void CompilerImpl::EmitDebugBreakpoint() {
  if ((debug_flags_ & DEBUG_BREAKPOINTS) && IsDebuggerPresent()) {
    asm_.int3();
  }
}

const Label &CompilerImpl::GetLabel(cell address) {
  Label &label = label_map_[address];
  if (label.getId() == asmjit::kInvalidValue) {
    label = asm_.newLabel();
  }
  return label;
}

}  // namespace amxjit
