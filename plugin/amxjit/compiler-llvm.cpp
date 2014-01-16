// Copyright (c) 2013-2013 Zeex
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

#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Support/TargetSelect.h>
#include "compiler-llvm.h"

namespace amxjit {

CompilerLLVM::CompilerLLVM():
  context(),
  module("amx", context),
  builder(llvm::BasicBlock::Create(context))
{
}

CompilerLLVM::~CompilerLLVM() {
}

bool CompilerLLVM::Setup() {
  llvm::InitializeNativeTarget();
  return true;
}

bool CompilerLLVM::Process(const Instruction &instr) {
  return true;
}

void CompilerLLVM::Abort() {
}

CompilerOutput *CompilerLLVM::Finish() {
  return 0;
}

void CompilerLLVM::load_pri(cell address) {
  // PRI = [address]
}

void CompilerLLVM::load_alt(cell address) {
  // ALT = [address]
}

void CompilerLLVM::load_s_pri(cell offset) {
  // PRI = [FRM + offset]
}

void CompilerLLVM::load_s_alt(cell offset) {
  // ALT = [FRM + offset]
}

void CompilerLLVM::lref_pri(cell address) {
  // PRI = [ [address] ]
}

void CompilerLLVM::lref_alt(cell address) {
  // ALT = [ [address] ]
}

void CompilerLLVM::lref_s_pri(cell offset) {
  // PRI = [ [FRM + offset] ]
}

void CompilerLLVM::lref_s_alt(cell offset) {
  // PRI = [ [FRM + offset] ]
}

void CompilerLLVM::load_i() {
  // PRI = [PRI] (full cell)
}

void CompilerLLVM::lodb_i(cell number) {
  // PRI = "number" bytes from [PRI] (read 1/2/4 bytes)
}

void CompilerLLVM::const_pri(cell value) {
  // PRI = value
}

void CompilerLLVM::const_alt(cell value) {
  // ALT = value
}

void CompilerLLVM::addr_pri(cell offset) {
  // PRI = FRM + offset
}

void CompilerLLVM::addr_alt(cell offset) {
  // ALT = FRM + offset
}

void CompilerLLVM::stor_pri(cell address) {
  // [address] = PRI
}

void CompilerLLVM::stor_alt(cell address) {
  // [address] = ALT
}

void CompilerLLVM::stor_s_pri(cell offset) {
  // [FRM + offset] = ALT
}

void CompilerLLVM::stor_s_alt(cell offset) {
  // [FRM + offset] = ALT
}

void CompilerLLVM::sref_pri(cell address) {
  // [ [address] ] = PRI
}

void CompilerLLVM::sref_alt(cell address) {
  // [ [address] ] = ALT
}

void CompilerLLVM::sref_s_pri(cell offset) {
  // [ [FRM + offset] ] = PRI
}

void CompilerLLVM::sref_s_alt(cell offset) {
  // [ [FRM + offset] ] = ALT
}

void CompilerLLVM::stor_i() {
  // [ALT] = PRI (full cell)
}

void CompilerLLVM::strb_i(cell number) {
  // "number" bytes at [ALT] = PRI (write 1/2/4 bytes)
}

void CompilerLLVM::lidx() {
  // PRI = [ ALT + (PRI x cell size) ]
}

void CompilerLLVM::lidx_b(cell shift) {
  // PRI = [ ALT + (PRI << shift) ]
}

void CompilerLLVM::idxaddr() {
  // PRI = ALT + (PRI x cell size) (calculate indexed address)
}

void CompilerLLVM::idxaddr_b(cell shift) {
  // PRI = ALT + (PRI << shift) (calculate indexed address)
}

void CompilerLLVM::align_pri(cell number) {
  // Little Endian: PRI ^= cell size - number
}

void CompilerLLVM::align_alt(cell number) {
  // Little Endian: ALT ^= cell size - number
}

void CompilerLLVM::lctrl(cell index) {
  // PRI is set to the current value of any of the special registers.
  // The index parameter must be:
  // 3=STP, 4=STK, 5=FRM, 6=CIP (of the next instruction)
}

void CompilerLLVM::sctrl(cell index) {
  // set the indexed special registers to the value in PRI.
  // The index parameter must be:
  // 6=CIP
}

void CompilerLLVM::move_pri() {
  // PRI = ALT
}

void CompilerLLVM::move_alt() {
  // ALT = PRI
}

void CompilerLLVM::xchg() {
  // Exchange PRI and ALT
}

void CompilerLLVM::push_pri() {
  // [STK] = PRI, STK = STK - cell size
}

void CompilerLLVM::push_alt() {
  // [STK] = ALT, STK = STK - cell size
}

void CompilerLLVM::push_c(cell value) {
  // [STK] = value, STK = STK - cell size
}

void CompilerLLVM::push(cell address) {
  // [STK] = [address], STK = STK - cell size
}

void CompilerLLVM::push_s(cell offset) {
  // [STK] = [FRM + offset], STK = STK - cell size
}

void CompilerLLVM::pop_pri() {
  // STK = STK + cell size, PRI = [STK]
}

void CompilerLLVM::pop_alt() {
  // STK = STK + cell size, ALT = [STK]
}

void CompilerLLVM::stack(cell value) {
  // ALT = STK, STK = STK + value
}

void CompilerLLVM::heap(cell value) {
  // ALT = HEA, HEA = HEA + value
}

void CompilerLLVM::proc() {
  // [STK] = FRM, STK = STK - cell size, FRM = STK
}

void CompilerLLVM::ret() {
  // STK = STK + cell size, FRM = [STK],
  // CIP = [STK], STK = STK + cell size
}

void CompilerLLVM::retn() {
  // STK = STK + cell size, FRM = [STK],
  // CIP = [STK], STK = STK + cell size
  // The RETN instruction removes a specified number of bytes
  // from the stack. The value to adjust STK with must be
  // pushed prior to the call.
}

void CompilerLLVM::call(cell address) {
  // [STK] = CIP + 5, STK = STK - cell size
  // CIP = CIP + offset
  // The CALL instruction jumps to an address after storing the
  // address of the next sequential instruction on the stack.
  // The address jumped to is relative to the current CIP,
  // but the address on the stack is an absolute address.
}

void CompilerLLVM::jump_pri() {
  // CIP = PRI (indirect jump)
}

void CompilerLLVM::jump(cell address) {
  // CIP = CIP + offset (jump to the address relative from
  // the current position)
}

void CompilerLLVM::jzer(cell address) {
  // if PRI == 0 then CIP = CIP + offset
}

void CompilerLLVM::jnz(cell address) {
  // if PRI != 0 then CIP = CIP + offset
}

void CompilerLLVM::jeq(cell address) {
  // if PRI == ALT then CIP = CIP + offset
}

void CompilerLLVM::jneq(cell address) {
  // if PRI != ALT then CIP = CIP + offset
}

void CompilerLLVM::jless(cell address) {
  // if PRI < ALT then CIP = CIP + offset (unsigned)
}

void CompilerLLVM::jleq(cell address) {
  // if PRI <= ALT then CIP = CIP + offset (unsigned)
}

void CompilerLLVM::jgrtr(cell address) {
  // if PRI > ALT then CIP = CIP + offset (unsigned)
}

void CompilerLLVM::jgeq(cell address) {
  // if PRI >= ALT then CIP = CIP + offset (unsigned)
}

void CompilerLLVM::jsless(cell address) {
  // if PRI < ALT then CIP = CIP + offset (signed)
}

void CompilerLLVM::jsleq(cell address) {
  // if PRI <= ALT then CIP = CIP + offset (signed)
}

void CompilerLLVM::jsgrtr(cell address) {
  // if PRI > ALT then CIP = CIP + offset (signed)
}

void CompilerLLVM::jsgeq(cell address) {
  // if PRI >= ALT then CIP = CIP + offset (signed)
}

void CompilerLLVM::shl() {
  // PRI = PRI << ALT
}

void CompilerLLVM::shr() {
  // PRI = PRI >> ALT (without sign extension)
}

void CompilerLLVM::sshr() {
  // PRI = PRI >> ALT with sign extension
}

void CompilerLLVM::shl_c_pri(cell value) {
  // PRI = PRI << value
}

void CompilerLLVM::shl_c_alt(cell value) {
  // ALT = ALT << value
}

void CompilerLLVM::shr_c_pri(cell value) {
  // PRI = PRI >> value (without sign extension)
}

void CompilerLLVM::shr_c_alt(cell value) {
  // PRI = PRI >> value (without sign extension)
}

void CompilerLLVM::smul() {
  // PRI = PRI * ALT (signed multiply)
}

void CompilerLLVM::sdiv() {
  // PRI = PRI / ALT (signed divide), ALT = PRI mod ALT
}

void CompilerLLVM::sdiv_alt() {
  // PRI = ALT / PRI (signed divide), ALT = ALT mod PRI
}

void CompilerLLVM::umul() {
  // PRI = PRI * ALT (unsigned multiply)
}

void CompilerLLVM::udiv() {
  // PRI = PRI / ALT (unsigned divide), ALT = PRI mod ALT
}

void CompilerLLVM::udiv_alt() {
  // PRI = ALT / PRI (unsigned divide), ALT = ALT mod PRI
}

void CompilerLLVM::add() {
  // PRI = PRI + ALT
}

void CompilerLLVM::sub() {
  // PRI = PRI - ALT
}

void CompilerLLVM::sub_alt() {
  // PRI = ALT - PRI
  // or:
  // PRI = -(PRI - ALT)
}

void CompilerLLVM::and_() {
  // PRI = PRI & ALT
}

void CompilerLLVM::or_() {
  // PRI = PRI | ALT
}

void CompilerLLVM::xor_() {
  // PRI = PRI ^ ALT
}

void CompilerLLVM::not_() {
  // PRI = !PRI
}

void CompilerLLVM::neg() {
  // PRI = -PRI
}

void CompilerLLVM::invert() {
  // PRI = ~PRI
}

void CompilerLLVM::add_c(cell value) {
  // PRI = PRI + value
}

void CompilerLLVM::smul_c(cell value) {
  // PRI = PRI * value
}

void CompilerLLVM::zero_pri() {
  // PRI = 0
}

void CompilerLLVM::zero_alt() {
  // ALT = 0
}

void CompilerLLVM::zero(cell address) {
  // [address] = 0
}

void CompilerLLVM::zero_s(cell offset) {
  // [FRM + offset] = 0
}

void CompilerLLVM::sign_pri() {
  // sign extent the byte in PRI to a cell
}

void CompilerLLVM::sign_alt() {
  // sign extent the byte in ALT to a cell
}

void CompilerLLVM::eq() {
  // PRI = PRI == ALT ? 1 :
}

void CompilerLLVM::neq() {
  // PRI = PRI != ALT ? 1 :
}

void CompilerLLVM::less() {
  // PRI = PRI < ALT ? 1 :
}

void CompilerLLVM::leq() {
  // PRI = PRI <= ALT ? 1 :
}

void CompilerLLVM::grtr() {
  // PRI = PRI > ALT ? 1 :
}

void CompilerLLVM::geq() {
  // PRI = PRI >= ALT ? 1 :
}

void CompilerLLVM::sless() {
  // PRI = PRI < ALT ? 1 :
}

void CompilerLLVM::sleq() {
  // PRI = PRI <= ALT ? 1 :
}

void CompilerLLVM::sgrtr() {
  // PRI = PRI > ALT ? 1 :
}

void CompilerLLVM::sgeq() {
  // PRI = PRI >= ALT ? 1 :
}

void CompilerLLVM::eq_c_pri(cell value) {
  // PRI = PRI == value ? 1 :
}

void CompilerLLVM::eq_c_alt(cell value) {
  // PRI = ALT == value ? 1 :
}

void CompilerLLVM::inc_pri() {
  // PRI = PRI + 1
}

void CompilerLLVM::inc_alt() {
  // ALT = ALT + 1
}

void CompilerLLVM::inc(cell address) {
  // [address] = [address] + 1
}

void CompilerLLVM::inc_s(cell offset) {
  // [FRM + offset] = [FRM + offset] + 1
}

void CompilerLLVM::inc_i() {
  // [PRI] = [PRI] + 1
}

void CompilerLLVM::dec_pri() {
  // PRI = PRI - 1
}

void CompilerLLVM::dec_alt() {
  // ALT = ALT - 1
}

void CompilerLLVM::dec(cell address) {
  // [address] = [address] - 1
}

void CompilerLLVM::dec_s(cell offset) {
  // [FRM + offset] = [FRM + offset] - 1
}

void CompilerLLVM::dec_i() {
  // [PRI] = [PRI] - 1
}

void CompilerLLVM::movs(cell num_bytes) {
  // Copy memory from [PRI] to [ALT]. The parameter
  // specifies the number of bytes. The blocks should not
  // overlap.
}

void CompilerLLVM::cmps(cell num_bytes) {
  // Compare memory blocks at [PRI] and [ALT]. The parameter
  // specifies the number of bytes. The blocks should not
  // overlap.
}

void CompilerLLVM::fill(cell num_bytes) {
  // Fill memory at [ALT] with value in [PRI]. The parameter
  // specifies the number of bytes, which must be a multiple
  // of the cell size.
}

void CompilerLLVM::halt(cell error_code) {
  // Abort execution (exit value in PRI), parameters other than 0
  // have a special meaning.
}

void CompilerLLVM::bounds(cell value) {
  // Abort execution if PRI > value or if PRI < 0.
}

void CompilerLLVM::sysreq_pri() {
  // call system service, service number in PRI
}

void CompilerLLVM::sysreq_c(cell index, const char *name) {
  // call system service
}

void CompilerLLVM::sysreq_d(cell address, const char *name) {
  // call system service
}

void CompilerLLVM::switch_(const CaseTable &case_table) {
  // Compare PRI to the values in the case table (whose address
  // is passed as an offset from CIP) and jump to the associated
  // address in the matching record.
}

void CompilerLLVM::casetbl() {
  // A variable number of case records follows this opcode, where
  // each record takes two cells.
}

void CompilerLLVM::swap_pri() {
  // [STK] = PRI and PRI = [STK]
}

void CompilerLLVM::swap_alt() {
  // [STK] = ALT and ALT = [STK]
}

void CompilerLLVM::push_adr(cell offset) {
  // [STK] = FRM + offset, STK = STK - cell size
}

void CompilerLLVM::nop() {
  // no-operation, for code alignment
}

void CompilerLLVM::break_() {
  // conditional breakpoint
}

} // namespace amxjit
