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
#include <map>
#include <set>
#include <string>
#include <vector>
#include <amx/amx.h>
#include <asmjit/core.h>
#include <asmjit/x86.h>
#include "amxptr.h"
#include "amxdisasm.h"
#include "backend-asmjit.h"
#include "callconv.h"
#include "compiler.h"

using namespace AsmJit;

#define DEFINE_INTRINSIC(Name)                                      \
  class Name : public jit::AsmjitBackend::Intrinsic {               \
   public:                                                          \
    Name(const char *name) : jit::AsmjitBackend::Intrinsic(name) {} \
    virtual ~Name() {}                                              \
    virtual void emit(X86Assembler &as)

DEFINE_INTRINSIC(native_float) {
  as.fild(dword_ptr(esp, 4));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}};

DEFINE_INTRINSIC(native_floatabs) {
  as.fld(dword_ptr(esp, 4));
  as.fabs();
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}};

DEFINE_INTRINSIC(native_floatadd) {
  as.fld(dword_ptr(esp, 4));
  as.fadd(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}};

DEFINE_INTRINSIC(native_floatsub) {
  as.fld(dword_ptr(esp, 4));
  as.fsub(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}};

DEFINE_INTRINSIC(native_floatmul) {
  as.fld(dword_ptr(esp, 4));
  as.fmul(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}};

DEFINE_INTRINSIC(native_floatdiv) {
  as.fld(dword_ptr(esp, 4));
  as.fdiv(dword_ptr(esp, 8));
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}};

DEFINE_INTRINSIC(native_floatsqroot) {
  as.fld(dword_ptr(esp, 4));
  as.fsqrt();
  as.sub(esp, 4);
  as.fstp(dword_ptr(esp));
  as.mov(eax, dword_ptr(esp));
  as.add(esp, 4);
}};

DEFINE_INTRINSIC(native_floatlog) {
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
}};

#undef DEFINE_INTRINSIC

namespace jit {

struct AsmjitBackend::Labels {
  Label exec_ptr;
  Label amx_ptr;
  Label amx_data_ptr;
  Label exec;
  Label exec_helper;
  Label halt_helper;
  Label jump_helper;
  Label sysreq_c_helper;
  Label sysreq_d_helper;
  Label ebp;
  Label esp;
  Label reset_ebp;
  Label reset_esp;
  Label instr_map_size;
  Label instr_map_ptr;
};

AsmjitBackendOutput::AsmjitBackendOutput(void *code, std::size_t code_size)
  : code_(code), code_size_(code_size)
{
}

AsmjitBackendOutput::~AsmjitBackendOutput() {
  AsmJit::MemoryManager::getGlobal()->free(code_);
}

AsmjitBackend::AsmjitBackend()
  : labels_(new Labels)
{
  intrinsics_.push_back(new native_float("float")),
  intrinsics_.push_back(new native_floatabs("floatabs"));
  intrinsics_.push_back(new native_floatadd("floatadd"));
  intrinsics_.push_back(new native_floatsub("floatsub"));
  intrinsics_.push_back(new native_floatmul("floatmul"));
  intrinsics_.push_back(new native_floatdiv("floatdiv"));
  intrinsics_.push_back(new native_floatsqroot("floatsqroot"));
  intrinsics_.push_back(new native_floatlog("floatlog"));
}

AsmjitBackend::~AsmjitBackend() {
  for (std::vector<Intrinsic*>::const_iterator it = intrinsics_.begin();
       it != intrinsics_.end(); ++it) {
    delete *it;
  }
  for (LabelMap::const_iterator it = label_map_.begin();
       it != label_map_.end(); ++it) {
    delete it->second;
  }
  delete labels_;
}

BackendOutput *AsmjitBackend::compile(AMXPtr amx,
                                      CompileErrorHandler *error_handler)
{ 
  X86Assembler as;
  emit_runtime_data(amx, as);

  Label L_do_halt = as.newLabel();

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
      as.bind(amx_label(as, cip));
    }

    // Add this instruction to the opcode map.
    instr_map.push_back(std::make_pair(instr.address(), as.getCodeSize()));

    switch (instr.opcode().id()) {
    case OP_LOAD_PRI: // address
      // PRI = [address]
      as.mov(eax, dword_ptr(ebx, instr.operand()));
      break;
    case OP_LOAD_ALT: // address
      // ALT = [address]
      as.mov(ecx, dword_ptr(ebx, instr.operand()));
      break;
    case OP_LOAD_S_PRI: // offset
      // PRI = [FRM + offset]
      as.mov(eax, dword_ptr(ebp, instr.operand()));
      break;
    case OP_LOAD_S_ALT: // offset
      // ALT = [FRM + offset]
      as.mov(ecx, dword_ptr(ebp, instr.operand()));
      break;
    case OP_LREF_PRI: // address
      // PRI = [ [address] ]
      as.mov(edx, dword_ptr(ebx, instr.operand()));
      as.mov(eax, dword_ptr(ebx, edx));
      break;
    case OP_LREF_ALT: // address
      // ALT = [ [address] ]
      as.mov(edx, dword_ptr(ebx, + instr.operand()));
      as.mov(ecx, dword_ptr(ebx, edx));
      break;
    case OP_LREF_S_PRI: // offset
      // PRI = [ [FRM + offset] ]
      as.mov(edx, dword_ptr(ebp, instr.operand()));
      as.mov(eax, dword_ptr(ebx, edx));
      break;
    case OP_LREF_S_ALT: // offset
      // PRI = [ [FRM + offset] ]
      as.mov(edx, dword_ptr(ebp, instr.operand()));
      as.mov(ecx, dword_ptr(ebx, edx));
      break;
    case OP_LOAD_I:
      // PRI = [PRI] (full cell)
      as.mov(eax, dword_ptr(ebx, eax));
      break;
    case OP_LODB_I: // number
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
        error = true;
      }
      break;
    case OP_CONST_PRI: // value
      // PRI = value
      if (instr.operand() == 0) {
        as.xor_(eax, eax);
      } else {
        as.mov(eax, instr.operand());
      }
      break;
    case OP_CONST_ALT: // value
      // ALT = value
      if (instr.operand() == 0) {
        as.xor_(ecx, ecx);
      } else {
        as.mov(ecx, instr.operand());
      }
      break;
    case OP_ADDR_PRI: // offset
      // PRI = FRM + offset
      as.lea(eax, dword_ptr(ebp, instr.operand()));
      as.sub(eax, ebx);
      break;
    case OP_ADDR_ALT: // offset
      // ALT = FRM + offset
      as.lea(ecx, dword_ptr(ebp, instr.operand()));
      as.sub(ecx, ebx);
      break;
    case OP_STOR_PRI: // address
      // [address] = PRI
      as.mov(dword_ptr(ebx, instr.operand()), eax);
      break;
    case OP_STOR_ALT: // address
      // [address] = ALT
      as.mov(dword_ptr(ebx, instr.operand()), ecx);
      break;
    case OP_STOR_S_PRI: // offset
      // [FRM + offset] = ALT
      as.mov(dword_ptr(ebp, instr.operand()), eax);
      break;
    case OP_STOR_S_ALT: // offset
      // [FRM + offset] = ALT
      as.mov(dword_ptr(ebp, instr.operand()), ecx);
      break;
    case OP_SREF_PRI: // address
      // [ [address] ] = PRI
      as.mov(edx, dword_ptr(ebx, instr.operand()));
      as.mov(dword_ptr(ebx, edx), eax);
      break;
    case OP_SREF_ALT: // address
      // [ [address] ] = ALT
      as.mov(edx, dword_ptr(ebx, instr.operand()));
      as.mov(dword_ptr(ebx, edx), ecx);
      break;
    case OP_SREF_S_PRI: // offset
      // [ [FRM + offset] ] = PRI
      as.mov(edx, dword_ptr(ebp, instr.operand()));
      as.mov(dword_ptr(ebx, edx), eax);
      break;
    case OP_SREF_S_ALT: // offset
      // [ [FRM + offset] ] = ALT
      as.mov(edx, dword_ptr(ebp, instr.operand()));
      as.mov(dword_ptr(ebx, edx), ecx);
      break;
    case OP_STOR_I:
      // [ALT] = PRI (full cell)
      as.mov(dword_ptr(ebx, ecx), eax);
      break;
    case OP_STRB_I: // number
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
        error = true;
      }
      break;
    case OP_LIDX:
      // PRI = [ ALT + (PRI x cell size) ]
      as.lea(edx, dword_ptr(ebx, ecx));
      as.mov(eax, dword_ptr(edx, eax, 2));
      break;
    case OP_LIDX_B: // shift
      // PRI = [ ALT + (PRI << shift) ]
      as.lea(edx, dword_ptr(ebx, ecx));
      as.mov(eax, dword_ptr(edx, eax, instr.operand()));
      break;
    case OP_IDXADDR:
      // PRI = ALT + (PRI x cell size) (calculate indexed address)
      as.lea(eax, dword_ptr(ecx, eax, 2));
      break;
    case OP_IDXADDR_B: // shift
      // PRI = ALT + (PRI << shift) (calculate indexed address)
      as.lea(eax, dword_ptr(ecx, eax, instr.operand()));
      break;
    case OP_ALIGN_PRI: // number
      // Little Endian: PRI ^= cell size - number
      #if BYTE_ORDER == LITTLE_ENDIAN
        if (instr.operand() < sizeof(cell)) {
          as.xor_(eax, sizeof(cell) - instr.operand());
        }
      #endif
      break;
    case OP_ALIGN_ALT: // number
      // Little Endian: ALT ^= cell size - number
      #if BYTE_ORDER == LITTLE_ENDIAN
        if (instr.operand() < sizeof(cell)) {
          as.xor_(ecx, sizeof(cell) - instr.operand());
        }
      #endif
      break;
    case OP_LCTRL: // index
      // PRI is set to the current value of any of the special registers.
      // The index parameter must be: 0=COD, 1=DAT, 2=HEA,
      // 3=STP, 4=STK, 5=FRM, 6=CIP (of the next instruction)
      switch (instr.operand()) {
      case 0:
        as.mov(eax, dword_ptr_abs(reinterpret_cast<void*>(&amx.hdr()->cod)));
        break;
      case 1:
        as.mov(eax, dword_ptr_abs(reinterpret_cast<void*>(&amx.hdr()->dat)));
        break;
      case 2:
        as.mov(eax, dword_ptr_abs(reinterpret_cast<void*>(&amx->hea)));
        break;
      case 3:
        as.mov(eax, dword_ptr_abs(reinterpret_cast<void*>(&amx->stp)));
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
        error = true;
      }
      break;
    case OP_SCTRL: // index
      // set the indexed special registers to the value in PRI.
      // The index parameter must be: 2=HEA, 4=STK, 5=FRM,
      // 6=CIP
      switch (instr.operand()) {
      case 2:
        as.mov(dword_ptr_abs(reinterpret_cast<void*>(&amx->hea)), eax);
        break;
      case 4:
        as.lea(esp, dword_ptr(ebx, eax));
        break;
      case 5:
        as.lea(ebp, dword_ptr(ebx, eax));
        break;
      case 6:
        as.push(esp);
        as.push(eax);
        as.push(reinterpret_cast<intptr_t>(this));
        as.call(labels_->jump_helper);
        break;
      default:
        error = true;
      }
      break;
    case OP_MOVE_PRI:
      // PRI = ALT
      as.mov(eax, ecx);
      break;
    case OP_MOVE_ALT:
      // ALT = PRI
      as.mov(ecx, eax);
      break;
    case OP_XCHG:
      // Exchange PRI and ALT
      as.xchg(eax, ecx);
      break;
    case OP_PUSH_PRI:
      // [STK] = PRI, STK = STK - cell size
      as.push(eax);
      break;
    case OP_PUSH_ALT:
      // [STK] = ALT, STK = STK - cell size
      as.push(ecx);
      break;
    case OP_PUSH_C: // value
      // [STK] = value, STK = STK - cell size
      as.push(instr.operand());
      break;
    case OP_PUSH: // address
      // [STK] = [address], STK = STK - cell size
      as.push(dword_ptr(ebx, instr.operand()));
      break;
    case OP_PUSH_S: // offset
      // [STK] = [FRM + offset], STK = STK - cell size
      as.push(dword_ptr(ebp, instr.operand()));
      break;
    case OP_POP_PRI:
      // STK = STK + cell size, PRI = [STK]
      as.pop(eax);
      break;
    case OP_POP_ALT:
      // STK = STK + cell size, ALT = [STK]
      as.pop(ecx);
      break;
    case OP_STACK: // value
      // ALT = STK, STK = STK + value
      as.mov(ecx, esp);
      as.sub(ecx, ebx);
      if (instr.operand() >= 0) {
        as.add(esp, instr.operand());
      } else {
        as.sub(esp, -instr.operand());
      }
      break;
    case OP_HEAP: // value
      // ALT = HEA, HEA = HEA + value
      as.mov(ecx, dword_ptr_abs(reinterpret_cast<void*>(&amx->hea)));
      if (instr.operand() >= 0) {
        as.add(dword_ptr_abs(reinterpret_cast<void*>(&amx->hea)), instr.operand());
      } else {
        as.sub(dword_ptr_abs(reinterpret_cast<void*>(&amx->hea)), -instr.operand());
      }
      break;
    case OP_PROC:
      // [STK] = FRM, STK = STK - cell size, FRM = STK
      as.push(ebp);
      as.mov(ebp, esp);
      as.sub(dword_ptr(esp), ebx);
      break;
    case OP_RET:
      // STK = STK + cell size, FRM = [STK],
      // CIP = [STK], STK = STK + cell size
      as.pop(ebp);
      as.add(ebp, ebx);
      as.ret();
      break;
    case OP_RETN:
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
      break;
    case OP_CALL: // offset
      // [STK] = CIP + 5, STK = STK - cell size
      // CIP = CIP + offset
      // The CALL instruction jumps to an address after storing the
      // address of the next sequential instruction on the stack.
      // The address jumped to is relative to the current CIP,
      // but the address on the stack is an absolute address.
      as.call(amx_label(as, instr.operand() - reinterpret_cast<cell>(amx.code())));
      break;
    case OP_JUMP_PRI:
      // CIP = PRI (indirect jump)
      as.push(ebp);                         // stack_base
      as.lea(edx, dword_ptr(esp, -12));
      as.push(edx);                         // stack_ptr
      as.push(eax);                         // address
      as.push(reinterpret_cast<intptr_t>(this)); // jit
      as.call(labels_->jump_helper);
      break;
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
      Label &L_dest = amx_label(as, instr.operand() - reinterpret_cast<cell>(amx.code()));
      switch (instr.opcode().id()) {
        case OP_JUMP: // offset
          // CIP = CIP + offset (jump to the address relative from
          // the current position)
          as.jmp(L_dest);
          break;
        case OP_JZER: // offset
          // if PRI == 0 then CIP = CIP + offset
          as.cmp(eax, 0);
          as.jz(L_dest);
          break;
        case OP_JNZ: // offset
          // if PRI != 0 then CIP = CIP + offset
          as.cmp(eax, 0);
          as.jnz(L_dest);
          break;
        case OP_JEQ: // offset
          // if PRI == ALT then CIP = CIP + offset
          as.cmp(eax, ecx);
          as.je(L_dest);
          break;
        case OP_JNEQ: // offset
          // if PRI != ALT then CIP = CIP + offset
          as.cmp(eax, ecx);
          as.jne(L_dest);
          break;
        case OP_JLESS: // offset
          // if PRI < ALT then CIP = CIP + offset (unsigned)
          as.cmp(eax, ecx);
          as.jb(L_dest);
          break;
        case OP_JLEQ: // offset
          // if PRI <= ALT then CIP = CIP + offset (unsigned)
          as.cmp(eax, ecx);
          as.jbe(L_dest);
          break;
        case OP_JGRTR: // offset
          // if PRI > ALT then CIP = CIP + offset (unsigned)
          as.cmp(eax, ecx);
          as.ja(L_dest);
          break;
        case OP_JGEQ: // offset
          // if PRI >= ALT then CIP = CIP + offset (unsigned)
          as.cmp(eax, ecx);
          as.jae(L_dest);
          break;
        case OP_JSLESS: // offset
          // if PRI < ALT then CIP = CIP + offset (signed)
          as.cmp(eax, ecx);
          as.jl(L_dest);
          break;
        case OP_JSLEQ: // offset
          // if PRI <= ALT then CIP = CIP + offset (signed)
          as.cmp(eax, ecx);
          as.jle(L_dest);
          break;
        case OP_JSGRTR: // offset
          // if PRI > ALT then CIP = CIP + offset (signed)
          as.cmp(eax, ecx);
          as.jg(L_dest);
          break;
        case OP_JSGEQ: // offset
          // if PRI >= ALT then CIP = CIP + offset (signed)
          as.cmp(eax, ecx);
          as.jge(L_dest);
          break;
      }
      break;
    }
    case OP_SHL:
      // PRI = PRI << ALT
      as.shl(eax, cl);
      break;
    case OP_SHR:
      // PRI = PRI >> ALT (without sign extension)
      as.shr(eax, cl);
      break;
    case OP_SSHR:
      // PRI = PRI >> ALT with sign extension
      as.sar(eax, cl);
      break;
    case OP_SHL_C_PRI: // value
      // PRI = PRI << value
      as.shl(eax, static_cast<unsigned char>(instr.operand()));
      break;
    case OP_SHL_C_ALT: // value
      // ALT = ALT << value
      as.shl(ecx, static_cast<unsigned char>(instr.operand()));
      break;
    case OP_SHR_C_PRI: // value
      // PRI = PRI >> value (without sign extension)
      as.shr(eax, static_cast<unsigned char>(instr.operand()));
      break;
    case OP_SHR_C_ALT: // value
      // PRI = PRI >> value (without sign extension)
      as.shl(ecx, static_cast<unsigned char>(instr.operand()));
      break;
    case OP_SMUL:
      // PRI = PRI * ALT (signed multiply)
      as.xor_(edx, edx);
      as.imul(ecx);
      break;
    case OP_SDIV:
      // PRI = PRI / ALT (signed divide), ALT = PRI mod ALT
      as.xor_(edx, edx);
      as.idiv(ecx);
      as.mov(ecx, edx);
      break;
    case OP_SDIV_ALT:
      // PRI = ALT / PRI (signed divide), ALT = ALT mod PRI
      as.xchg(eax, ecx);
      as.xor_(edx, edx);
      as.idiv(ecx);
      as.mov(ecx, edx);
      break;
    case OP_UMUL:
      // PRI = PRI * ALT (unsigned multiply)
      as.xor_(edx, edx);
      as.mul(ecx);
      break;
    case OP_UDIV:
      // PRI = PRI / ALT (unsigned divide), ALT = PRI mod ALT
      as.xor_(edx, edx);
      as.div(ecx);
      as.mov(ecx, edx);
      break;
    case OP_UDIV_ALT:
      // PRI = ALT / PRI (unsigned divide), ALT = ALT mod PRI
      as.xchg(eax, ecx);
      as.xor_(edx, edx);
      as.div(ecx);
      as.mov(ecx, edx);
      break;
    case OP_ADD:
      // PRI = PRI + ALT
      as.add(eax, ecx);
      break;
    case OP_SUB:
      // PRI = PRI - ALT
      as.sub(eax, ecx);
      break;
    case OP_SUB_ALT:
      // PRI = ALT - PRI
      // or:
      // PRI = -(PRI - ALT)
      as.sub(eax, ecx);
      as.neg(eax);
      break;
    case OP_AND:
      // PRI = PRI & ALT
      as.and_(eax, ecx);
      break;
    case OP_OR:
      // PRI = PRI | ALT
      as.or_(eax, ecx);
      break;
    case OP_XOR:
      // PRI = PRI ^ ALT
      as.xor_(eax, ecx);
      break;
    case OP_NOT:
      // PRI = !PRI
      as.test(eax, eax);
      as.setz(al);
      as.movzx(eax, al);
      break;
    case OP_NEG:
      // PRI = -PRI
      as.neg(eax);
      break;
    case OP_INVERT:
      // PRI = ~PRI
      as.not_(eax);
      break;
    case OP_ADD_C: // value
      // PRI = PRI + value
      if (instr.operand() >= 0) {
        as.add(eax, instr.operand());
      } else {
        as.sub(eax, -instr.operand());
      }
      break;
    case OP_SMUL_C: // value
      // PRI = PRI * value
      as.imul(eax, instr.operand());
      break;
    case OP_ZERO_PRI:
      // PRI = 0
      as.xor_(eax, eax);
      break;
    case OP_ZERO_ALT:
      // ALT = 0
      as.xor_(ecx, ecx);
      break;
    case OP_ZERO: // address
      // [address] = 0
      as.mov(dword_ptr(ebx, instr.operand()), 0);
      break;
    case OP_ZERO_S: // offset
      // [FRM + offset] = 0
      as.mov(dword_ptr(ebp, instr.operand()), 0);
      break;
    case OP_SIGN_PRI:
      // sign extent the byte in PRI to a cell
      as.movsx(eax, al);
      break;
    case OP_SIGN_ALT:
      // sign extent the byte in ALT to a cell
      as.movsx(ecx, cl);
      break;
    case OP_EQ:
      // PRI = PRI == ALT ? 1 : 0
      as.cmp(eax, ecx);
      as.sete(al);
      as.movzx(eax, al);
      break;
    case OP_NEQ:
      // PRI = PRI != ALT ? 1 : 0
      as.cmp(eax, ecx);
      as.setne(al);
      as.movzx(eax, al);
      break;
    case OP_LESS:
      // PRI = PRI < ALT ? 1 : 0 (unsigned)
      as.cmp(eax, ecx);
      as.setb(al);
      as.movzx(eax, al);
      break;
    case OP_LEQ:
      // PRI = PRI <= ALT ? 1 : 0 (unsigned)
      as.cmp(eax, ecx);
      as.setbe(al);
      as.movzx(eax, al);
      break;
    case OP_GRTR:
      // PRI = PRI > ALT ? 1 : 0 (unsigned)
      as.cmp(eax, ecx);
      as.seta(al);
      as.movzx(eax, al);
      break;
    case OP_GEQ:
      // PRI = PRI >= ALT ? 1 : 0 (unsigned)
      as.cmp(eax, ecx);
      as.setae(al);
      as.movzx(eax, al);
      break;
    case OP_SLESS:
      // PRI = PRI < ALT ? 1 : 0 (signed)
      as.cmp(eax, ecx);
      as.setl(al);
      as.movzx(eax, al);
      break;
    case OP_SLEQ:
      // PRI = PRI <= ALT ? 1 : 0 (signed)
      as.cmp(eax, ecx);
      as.setle(al);
      as.movzx(eax, al);
      break;
    case OP_SGRTR:
      // PRI = PRI > ALT ? 1 : 0 (signed)
      as.cmp(eax, ecx);
      as.setg(al);
      as.movzx(eax, al);
      break;
    case OP_SGEQ:
      // PRI = PRI >= ALT ? 1 : 0 (signed)
      as.cmp(eax, ecx);
      as.setge(al);
      as.movzx(eax, al);
      break;
    case OP_EQ_C_PRI: // value
      // PRI = PRI == value ? 1 : 0
      as.cmp(eax, instr.operand());
      as.sete(al);
      as.movzx(eax, al);
      break;
    case OP_EQ_C_ALT: // value
      // PRI = ALT == value ? 1 : 0
      as.cmp(ecx, instr.operand());
      as.sete(al);
      as.movzx(eax, al);
      break;
    case OP_INC_PRI:
      // PRI = PRI + 1
      as.inc(eax);
      break;
    case OP_INC_ALT:
      // ALT = ALT + 1
      as.inc(ecx);
      break;
    case OP_INC: // address
      // [address] = [address] + 1
      as.inc(dword_ptr(ebx, instr.operand()));
      break;
    case OP_INC_S: // offset
      // [FRM + offset] = [FRM + offset] + 1
      as.inc(dword_ptr(ebp, instr.operand()));
      break;
    case OP_INC_I:
      // [PRI] = [PRI] + 1
      as.inc(dword_ptr(ebx, eax));
      break;
    case OP_DEC_PRI:
      // PRI = PRI - 1
      as.dec(eax);
      break;
    case OP_DEC_ALT:
      // ALT = ALT - 1
      as.dec(ecx);
      break;
    case OP_DEC: // address
      // [address] = [address] - 1
      as.dec(dword_ptr(ebx, instr.operand()));
      break;
    case OP_DEC_S: // offset
      // [FRM + offset] = [FRM + offset] - 1
      as.dec(dword_ptr(ebp, instr.operand()));
      break;
    case OP_DEC_I:
      // [PRI] = [PRI] - 1
      as.dec(dword_ptr(ebx, eax));
      break;
    case OP_MOVS: // number
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
      break;
    case OP_CMPS: { // number
      // Compare memory blocks at [PRI] and [ALT]. The parameter
      // specifies the number of bytes. The blocks should not
      // overlap.
      Label L_above(amx_label(as, cip, "above"));
      Label L_below(amx_label(as, cip, "below"));
      Label L_equal(amx_label(as, cip, "equal"));
      Label L_continue(amx_label(as, cip, "continue"));
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
      break;
    }
    case OP_FILL: // number
      // Fill memory at [ALT] with value in [PRI]. The parameter
      // specifies the number of bytes, which must be a multiple
      // of the cell size.
      as.cld();
      as.lea(edi, dword_ptr(ebx, ecx));
      as.push(ecx);
      as.mov(ecx, instr.operand() / sizeof(cell));
      as.rep_stosd();
      as.pop(ecx);
      break;
    case OP_HALT: // number
      // Abort execution (exit value in PRI), parameters other than 0
      // have a special meaning.
      as.mov(ecx, instr.operand());
      as.jmp(L_do_halt);
      break;
    case OP_BOUNDS: { // value
      // Abort execution if PRI > value or if PRI < 0.
      Label &L_halt = amx_label(as, cip, "halt");
      Label &L_good = amx_label(as, cip, "good");
        as.cmp(eax, instr.operand());
      as.jg(L_halt);
        as.cmp(eax, 0);
        as.jl(L_halt);
        as.jmp(L_good);
      as.bind(L_halt);
        as.mov(ecx, AMX_ERR_BOUNDS);
        as.jmp(L_do_halt);
      as.bind(L_good);
      break;
    }
    case OP_SYSREQ_PRI: {
      // call system service, service number in PRI
      as.push(esp); // stack_ptr
      as.push(ebp); // stack_base
      as.push(eax); // index
      as.call(labels_->sysreq_c_helper);
      break;
    }
    case OP_SYSREQ_C:   // index
    case OP_SYSREQ_D: { // address
      // call system service
      const char *native_name = 0;
      switch (instr.opcode().id()) {
        case OP_SYSREQ_C:
          native_name = amx.get_native_name(instr.operand());
          break;
        case OP_SYSREQ_D: {
          native_name = amx.get_native_name(amx.find_native(instr.operand()));
          break;
        }
      }
      if (native_name != 0) {
        for (std::vector<Intrinsic*>::const_iterator it = intrinsics_.begin();
             it != intrinsics_.end(); ++it) {
          Intrinsic *intr = *it;
          if (intr->name() == native_name) {
            intr->emit(as);
            goto special_native;
          }
        }
      } else {
        error = true;
        break;
      }

      switch (instr.opcode().id()) {
        case OP_SYSREQ_C: {
          as.push(esp); // stack_ptr
          as.push(ebp); // stack_base
          as.push(instr.operand()); // index
          as.call(labels_->sysreq_c_helper);
          break;
        }
        case OP_SYSREQ_D:
          as.push(esp); // stack_ptr
          as.push(ebp); // stack_base
          as.push(instr.operand()); // address
          as.call(labels_->sysreq_d_helper);
          break;
      }
      break;

    special_native:
      // Already processed above.
      break;
    }
    case OP_SWITCH: { // offset
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
      cell default_case = case_table[0].address - reinterpret_cast<cell>(amx.code());

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
        as.jl(amx_label(as, default_case));
        as.cmp(eax, *max_value);
        as.jg(amx_label(as, default_case));

        // OK now sequentially compare eax with each value.
        // This is pretty slow so I probably should optimize
        // this in future...
        for (int i = 0; i < num_records; i++) {
          as.cmp(eax, case_table[i + 1].value);
          as.je(amx_label(as, case_table[i + 1].address - reinterpret_cast<cell>(amx.code())));
        }
      }

      // No match found - go for default case.
      as.jmp(amx_label(as, default_case));
      break;
    }
    case OP_CASETBL: // ...
      // A variable number of case records follows this opcode, where
      // each record takes two cells.
      break;
    case OP_SWAP_PRI:
      // [STK] = PRI and PRI = [STK]
      as.xchg(dword_ptr(esp), eax);
      break;
    case OP_SWAP_ALT:
      // [STK] = ALT and ALT = [STK]
      as.xchg(dword_ptr(esp), ecx);
      break;
    case OP_PUSH_ADR: // offset
      // [STK] = FRM + offset, STK = STK - cell size
      as.lea(edx, dword_ptr(ebp, instr.operand()));
      as.sub(edx, ebx);
      as.push(edx);
      break;
    case OP_NOP:
      // no-operation, for code alignment
      break;
    case OP_BREAK:
      // conditional breakpoint
      break;
    default:
      error = true;
    }
  }

  as.bind(L_do_halt);
    as.push(ecx);
    as.call(labels_->halt_helper);

  // If compilation fails the code is incomplete and thus not runnable.
  if (error) {
    if (error_handler != 0) {
      error_handler->execute(instr);
    }
    return 0;
  }

  intptr_t code_ptr = reinterpret_cast<intptr_t>(as.make());
  size_t code_size = as.getCodeSize();

  intptr_t *runtime_data = reinterpret_cast<intptr_t*>(code_ptr);

  // Relocate address of exec() stored in the runtime data block.
  runtime_data[RuntimeDataExecPtr] += code_ptr;

  // Relocate address of the instruction map stored in the runtime data block.
  runtime_data[RuntimeDataInstrMapPtr] += code_ptr;
  intptr_t instr_map_ptr = runtime_data[RuntimeDataInstrMapPtr];

  // Fill the instruction map.
  for (std::size_t i = 0; i < instr_map.size(); i++) {
    InstrMapEntry *entry = &reinterpret_cast<InstrMapEntry*>(instr_map_ptr)[i];
    entry->amx_addr = instr_map[i].first;
    entry->jit_addr = reinterpret_cast<void*>(instr_map[i].second + code_ptr);
  }

  return new AsmjitBackendOutput(reinterpret_cast<void*>(code_ptr), code_size);
}

void AsmjitBackend::emit_runtime_data(AMXPtr amx, AsmJit::X86Assembler &as) {
  // Create and initialize labels.
  labels_->exec_ptr = as.newLabel();
  labels_->amx_ptr = as.newLabel();
  labels_->amx_data_ptr = as.newLabel();
  labels_->exec = as.newLabel();
  labels_->exec_helper = as.newLabel();
  labels_->halt_helper = as.newLabel();
  labels_->jump_helper = as.newLabel();
  labels_->sysreq_c_helper = as.newLabel();
  labels_->sysreq_d_helper = as.newLabel();
  labels_->ebp = as.newLabel();
  labels_->esp = as.newLabel();
  labels_->reset_ebp = as.newLabel();
  labels_->reset_esp = as.newLabel();
  labels_->instr_map_size = as.newLabel();
  labels_->instr_map_ptr = as.newLabel();

  // Write the header. Some fields will be filled later.
  as.bind(labels_->exec_ptr);
    as.dd(0);
  as.bind(labels_->amx_ptr);
    as.dintptr(reinterpret_cast<intptr_t>(amx.amx()));
  as.bind(labels_->amx_data_ptr);
    as.dintptr(reinterpret_cast<intptr_t>(amx.data()));
  as.bind(labels_->ebp);
    as.dd(0);
  as.bind(labels_->esp);
    as.dd(0);
  as.bind(labels_->reset_ebp);
    as.dd(0);
  as.bind(labels_->reset_esp);
    as.dd(0);
  as.bind(labels_->instr_map_size);
    as.dd(0);
  as.bind(labels_->instr_map_ptr);
    as.dd(0);

  // Reserve space for the instruction map.
  emit_instr_map(as, amx);

  // Write the entry point and some auxilary functions.
  emit_exec(as);
  emit_exec_helper(as);
  emit_halt_helper(as);
  emit_jump_helper(as);
  emit_sysreq_c_helper(as);
  emit_sysreq_d_helper(as);
}

void AsmjitBackend::emit_instr_map(AsmJit::X86Assembler &as,
                                   AMXPtr amx) const 
{
  AMXDisassembler disas(amx);
  AMXInstruction instr;
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

void AsmjitBackend::emit_exec(AsmJit::X86Assembler &as) const {
  set_runtime_data(as, RuntimeDataExecPtr, as.getCodeSize());
  as.bind(labels_->exec);

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

    as.push(ebp);
    as.mov(ebp, esp);
    as.sub(esp, 12); // for locals

    as.push(esi);
    as.mov(esi, dword_ptr(labels_->amx_ptr));

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
    as.push(dword_ptr(labels_->amx_ptr));
    as.call((void*)&get_public_addr);
    as.add(esp, 8);
  
    // If the function was not found, exit with error.
    as.cmp(eax, 0);
    as.jne(L_do_call);
    as.mov(eax, AMX_ERR_INDEX);
    as.jmp(L_return);

  as.bind(L_do_call);

    // Get pointer to the start of the function.
    as.push(dword_ptr(labels_->instr_map_size));
    as.push(dword_ptr(labels_->instr_map_ptr));
    as.push(eax);
    as.call((void*)&get_instr_ptr);
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
    as.mov(edx, dword_ptr(labels_->amx_data_ptr));
    as.mov(dword_ptr(edx, ecx), eax);
    as.mov(dword_ptr(esi, offsetof(AMX, stk)), ecx);
    as.mov(dword_ptr(esi, offsetof(AMX, paramcount)), 0);

    // Keep a copy of the old reset_ebp and reset_esp on the stack.
    as.mov(eax, dword_ptr(labels_->reset_ebp));
    as.mov(dword_ptr(ebp, var_reset_ebp), eax);
    as.mov(eax, dword_ptr(labels_->reset_esp));
    as.mov(dword_ptr(ebp, var_reset_esp), eax);

    // Call the function.
    as.push(dword_ptr(ebp, var_address));
    as.call(labels_->exec_helper);
    as.add(esp, 4);

    // Copt the return value if retval is not NULL.
    as.mov(ecx, dword_ptr(esp, arg_retval));
    as.cmp(ecx, 0);
    as.je(L_cleanup);
    as.mov(dword_ptr(ecx), eax);

  as.bind(L_cleanup);
    // Restore reset_ebp and reset_esp (remember they are kept in locals?).
    as.mov(eax, dword_ptr(ebp, var_reset_ebp));
    as.mov(dword_ptr(labels_->reset_ebp), eax);
    as.mov(eax, dword_ptr(ebp, var_reset_esp));
    as.mov(dword_ptr(labels_->reset_esp), eax);

    as.mov(eax, AMX_ERR_NONE);
    as.xchg(eax, dword_ptr(esi, offsetof(AMX, error)));

  as.bind(L_return);
    as.pop(esi);
    as.mov(esp, ebp);
    as.pop(ebp);
    as.ret();
}

void AsmjitBackend::emit_exec_helper(AsmJit::X86Assembler &as) const {
  as.bind(labels_->exec_helper);

  // Store the function address in eax.
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

  // All functions assume that ebx points to the AMX data.
  as.mov(ebx, dword_ptr(labels_->amx_data_ptr));

  // Store old ebp and esp on the stack.
  as.push(dword_ptr(labels_->ebp));
  as.push(dword_ptr(labels_->esp));

  // Most recent ebp and esp are stored in member variables.
  as.mov(dword_ptr(labels_->ebp), ebp);
  as.mov(dword_ptr(labels_->esp), esp);

  // Switch from the native stack to the AMX stack.
  as.mov(ecx, dword_ptr(labels_->amx_ptr));
  as.mov(edx, dword_ptr(ecx, offsetof(AMX, frm)));
  as.lea(ebp, dword_ptr(ebx, edx)); // ebp = amx_data + amx->frm
  as.mov(edx, dword_ptr(ecx, offsetof(AMX, stk)));
  as.lea(esp, dword_ptr(ebx, edx)); // esp = amx_data + amx->stk

  // In order to make halt() work we have to be able to return to this
  // point somehow. The easiest way it to set the stack registers as
  // if we called the offending instruction directly from here.
  as.lea(ecx, dword_ptr(esp, - 4));
  as.mov(dword_ptr(labels_->reset_esp), ecx);
  as.mov(dword_ptr(labels_->reset_ebp), ebp);

  // Call the function. Prior to his point ebx should point to the
  // AMX data and the both stack pointers should point to somewhere
  // in the AMX stack.
  as.call(eax);

  // Keep the AMX stack registers up-to-date. This wouldn't be necessary if
  // RETN didn't modify them (it pops all arguments off the stack).
  as.mov(eax, dword_ptr(labels_->amx_ptr));
  as.mov(edx, ebp);
  as.sub(edx, ebx);
  as.mov(dword_ptr(eax, offsetof(AMX, frm)), edx); // amx->frm = ebp - amx_data
  as.mov(edx, esp);
  as.sub(edx, ebx);
  as.mov(dword_ptr(eax, offsetof(AMX, stk)), edx); // amx->stk = esp - amx_data

  // Switch back to the native stack.
  as.mov(ebp, dword_ptr(labels_->ebp));
  as.mov(esp, dword_ptr(labels_->esp));

  as.pop(dword_ptr(labels_->esp));
  as.pop(dword_ptr(labels_->ebp));
    
  as.pop(edx);
  as.pop(ecx);
  as.pop(ebx);
  as.pop(edi);
  as.pop(esi);

  as.ret();
}

void AsmjitBackend::emit_halt_helper(AsmJit::X86Assembler &as) const {
  as.bind(labels_->halt_helper);

  // Since there's currently no way detect whether the function returns
  // normally or by executing halt there's no way if transmitting the
  // error code other than through a member variable.
  as.mov(eax, dword_ptr(esp, 4));
  as.mov(ecx, dword_ptr(labels_->amx_ptr));
  as.mov(dword_ptr(ecx, offsetof(AMX, error)), eax);

  // Reset stack so we can return right to call().
  as.mov(esp, dword_ptr(labels_->reset_esp));
  as.mov(ebp, dword_ptr(labels_->reset_ebp));

  // Pop public arguments as it would otherwise be done by RETN.
  as.pop(eax);
  as.add(esp, dword_ptr(esp));
  as.add(esp, 4);
  as.push(eax);

  as.ret();
}

void AsmjitBackend::emit_jump_helper(AsmJit::X86Assembler &as) const {
  as.bind(labels_->jump_helper);

  Label L_do_jump = as.newLabel();

    // Get pointer to the JIT code corresponding to the function.
    as.push(dword_ptr(esp, 4));           // address
    as.push(dword_ptr(labels_->instr_map_ptr)); // instr_map
    as.call((void*)&get_instr_ptr);
    as.add(esp, 8);
      
    // If the address wasn't valid, ignore continue execution as if
    // no jump was initiated (this is how AMX does this).
    as.cmp(eax, 0);
    as.jne(L_do_jump);
    as.ret();

  as.bind(L_do_jump);
    as.mov(eax, dword_ptr(esp, 4));  // address
    as.mov(ebp, dword_ptr(esp, 8));  // stack_base
    as.mov(esp, dword_ptr(esp, 12)); // stack_ptr
    as.jmp(eax);
}

void AsmjitBackend::emit_sysreq_c_helper(AsmJit::X86Assembler &as) const {
  as.bind(labels_->sysreq_c_helper);

  Label L_call = as.newLabel();
  Label L_return = as.newLabel();

  int arg_index = 8;
  int arg_stack_base = 12;
  int arg_stack_ptr = 16;

    as.push(ebp);
    as.mov(ebp, esp);

    as.push(dword_ptr(ebp, arg_index));
    as.push(dword_ptr(labels_->amx_ptr));
    as.call((void*)&get_native_addr);
    as.add(esp, 8);

    as.cmp(eax, 0);
    as.jne(L_call);
    as.mov(eax, AMX_ERR_NOTFOUND);
    as.jmp(L_return);

  as.bind(L_call);
    as.push(dword_ptr(ebp, arg_stack_ptr));
    as.push(dword_ptr(ebp, arg_stack_base));
    as.push(eax); // address
    as.call(labels_->sysreq_d_helper);
    as.add(esp, 12);

  as.bind(L_return);
    as.mov(esp, ebp);
    as.pop(ebp);
    as.ret();
}

void AsmjitBackend::emit_sysreq_d_helper(AsmJit::X86Assembler &as) const {
  as.bind(labels_->sysreq_d_helper);

  as.mov(eax, dword_ptr(esp, 4));   // address
  as.mov(ebp, dword_ptr(esp, 8));   // stack_base
  as.mov(esp, dword_ptr(esp, 12));  // stack_ptr
  as.mov(ecx, esp);                 // params
  as.mov(ebx, dword_ptr(esp, -16)); // return address

  // AMX natives use the cdecl calling convetion in which esi and edi
  // are callee-saved (but not edx, thus it will be loaded again below).
  as.mov(edx, dword_ptr(labels_->amx_ptr));
  as.mov(esi, dword_ptr(edx, offsetof(AMX, frm))); // saved_frm
  as.mov(edi, dword_ptr(edx, offsetof(AMX, stk))); // saved_stk

  // ebx is callee-saved as well so no worries. But we have to save it
  // on the stack because we already hold the return address in it...
  as.push(ebx);
  as.mov(ebx, dword_ptr(labels_->amx_data_ptr));

  // Switch to the native stack.
  as.sub(ebp, dword_ptr(labels_->amx_data_ptr));
  as.mov(dword_ptr(edx, offsetof(AMX, frm)), ebp); // amx->frm = ebp - amx_data
  as.mov(ebp, dword_ptr(labels_->ebp));
  as.sub(esp, dword_ptr(labels_->amx_data_ptr));
  as.mov(dword_ptr(edx, offsetof(AMX, stk)), esp); // amx->stk = esp - amx_data
  as.mov(esp, dword_ptr(labels_->esp));

  // Call the native function.
  as.push(ecx); // params
  as.push(edx); // amx
  as.call(eax); // address
  as.add(esp, 8);
  // eax contains return value, the code below must not overwrite it!!

  // Switch back to the AMX stack.
  as.mov(edx, dword_ptr(labels_->amx_ptr));
  as.mov(dword_ptr(labels_->ebp), ebp);
  as.mov(ecx, dword_ptr(edx, offsetof(AMX, frm)));
  as.lea(ebp, dword_ptr(ebx, ecx)); // ebp = amx_data + amx->frm
  as.mov(dword_ptr(labels_->esp), esp);
  as.mov(ecx, dword_ptr(edx, offsetof(AMX, stk)));
  as.lea(esp, dword_ptr(ebx, ecx)); // ebp = amx_data + amx->stk

  // Restore frm and stk to the values they had before the call.
  // This is to stop natives from messing up the AMX stack.
  as.mov(dword_ptr(edx, offsetof(AMX, frm)), esi); // amx->frm = saved_frm
  as.mov(dword_ptr(edx, offsetof(AMX, stk)), edi); // amx->stk = saved_stk

  // At this point we desired return addres happens to be on the stack already
  // and already ebx points to the AMX data like it should.
  as.ret();
}

void *AsmjitBackend::get_next_instr_ptr(AsmJit::X86Assembler &as) const {
  intptr_t ip = reinterpret_cast<intptr_t>(as.getCode()) + as.getCodeSize();
  return reinterpret_cast<void*>(ip);
}

// Returns current pointer to the RuntimeData array (value can change as
// more code is emitted by the assembler).
intptr_t *AsmjitBackend::get_runtime_data(AsmJit::X86Assembler &as) const {
  return reinterpret_cast<intptr_t*>(as.getCode());
}

bool AsmjitBackend::collect_jump_targets(AMXPtr amx,
                                         std::set<cell> &refs) const
{
  AMXDisassembler disas(amx);
  AMXInstruction instr;
  bool error = false;

  while (disas.decode(instr, &error)) {
    AMXOpcode opcode = instr.opcode();
    switch (opcode.id()) {
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
      case OP_JSGEQ:
      case OP_CALL:
        refs.insert(instr.operand() - reinterpret_cast<intptr_t>(amx.code()));
        break;
      case OP_CASETBL: {
        int n = instr.num_operands();
        for (int i = 1; i < n; i += 2) {
          refs.insert(instr.operand(i) - reinterpret_cast<intptr_t>(amx.code()));
        }
        break;
      }
      case OP_PROC:
        refs.insert(instr.address());
        break;
    }
  }

  return !error;
}

Label &AsmjitBackend::amx_label(X86Assembler &as, cell address, const std::string &name) {
  std::pair<cell, std::string> key = std::make_pair(address, name);
  LabelMap::iterator it = label_map_.find(key);
  if (it != label_map_.end()) {
    return *it->second;
  } else {
    std::pair<LabelMap::iterator, bool> it_inserted =
        label_map_.insert(std::make_pair(key, new Label(as.newLabel())));
    return *it_inserted.first->second;
  }
}

// static
cell JIT_CDECL AsmjitBackend::get_public_addr(AMX *amx, int index) {
  return AMXPtr(amx).get_public_addr(index);
}

// static
cell JIT_CDECL AsmjitBackend::get_native_addr(AMX *amx, int index) {
  return AMXPtr(amx).get_native_addr(index);
}

// static
void *JIT_CDECL AsmjitBackend::get_instr_ptr(cell address, void *instr_map,
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

} // namespace jit