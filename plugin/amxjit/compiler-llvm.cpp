// Copyright (c) 2013 Zeex
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

CompilerLLVM::CompilerLLVM()
  : context_(),
    module_("amx", context_),
    builder_(llvm::BasicBlock::Create(context_))
{
}

CompilerLLVM::~CompilerLLVM() {
}

bool CompilerLLVM::setup(AMXPtr amx) {
  llvm::InitializeNativeTarget();
  return true;
}

bool CompilerLLVM::process(const Instruction &instr) {
  return true;
}

void CompilerLLVM::abort() {
}

CompilerOutput *CompilerLLVM::finish() {
  return 0;
}

void CompilerLLVM::emit_load_pri(cell address) {
  // PRI = [address]
}

void CompilerLLVM::emit_load_alt(cell address) {
  // ALT = [address]
}

void CompilerLLVM::emit_load_s_pri(cell offset) {
  // PRI = [FRM + offset]
}

void CompilerLLVM::emit_load_s_alt(cell offset) {
  // ALT = [FRM + offset]
}

void CompilerLLVM::emit_lref_pri(cell address) {
  // PRI = [ [address] ]
}

void CompilerLLVM::emit_lref_alt(cell address) {
  // ALT = [ [address] ]
}

void CompilerLLVM::emit_lref_s_pri(cell offset) {
  // PRI = [ [FRM + offset] ]
}

void CompilerLLVM::emit_lref_s_alt(cell offset) {
  // PRI = [ [FRM + offset] ]
}

void CompilerLLVM::emit_load_i() {
  // PRI = [PRI] (full cell)
}

void CompilerLLVM::emit_lodb_i(cell number) {
  // PRI = "number" bytes from [PRI] (read 1/2/4 bytes)
}

void CompilerLLVM::emit_const_pri(cell value) {
  // PRI = value
}

void CompilerLLVM::emit_const_alt(cell value) {
  // ALT = value
}

void CompilerLLVM::emit_addr_pri(cell offset) {
  // PRI = FRM + offset
}

void CompilerLLVM::emit_addr_alt(cell offset) {
  // ALT = FRM + offset
}

void CompilerLLVM::emit_stor_pri(cell address) {
  // [address] = PRI
}

void CompilerLLVM::emit_stor_alt(cell address) {
  // [address] = ALT
}

void CompilerLLVM::emit_stor_s_pri(cell offset) {
  // [FRM + offset] = ALT
}

void CompilerLLVM::emit_stor_s_alt(cell offset) {
  // [FRM + offset] = ALT
}

void CompilerLLVM::emit_sref_pri(cell address) {
  // [ [address] ] = PRI
}

void CompilerLLVM::emit_sref_alt(cell address) {
  // [ [address] ] = ALT
}

void CompilerLLVM::emit_sref_s_pri(cell offset) {
  // [ [FRM + offset] ] = PRI
}

void CompilerLLVM::emit_sref_s_alt(cell offset) {
  // [ [FRM + offset] ] = ALT
}

void CompilerLLVM::emit_stor_i() {
  // [ALT] = PRI (full cell)
}

void CompilerLLVM::emit_strb_i(cell number) {
  // "number" bytes at [ALT] = PRI (write 1/2/4 bytes)
}

void CompilerLLVM::emit_lidx() {
  // PRI = [ ALT + (PRI x cell size) ]
}

void CompilerLLVM::emit_lidx_b(cell shift) {
  // PRI = [ ALT + (PRI << shift) ]
}

void CompilerLLVM::emit_idxaddr() {
  // PRI = ALT + (PRI x cell size) (calculate indexed address)
}

void CompilerLLVM::emit_idxaddr_b(cell shift) {
  // PRI = ALT + (PRI << shift) (calculate indexed address)
}

void CompilerLLVM::emit_align_pri(cell number) {
  // Little Endian: PRI ^= cell size - number
}

void CompilerLLVM::emit_align_alt(cell number) {
  // Little Endian: ALT ^= cell size - number
}

void CompilerLLVM::emit_lctrl(cell index) {
  // PRI is set to the current value of any of the special registers.
  // The index parameter must be:
  // 3=STP, 4=STK, 5=FRM, 6=CIP (of the next instruction)
}

void CompilerLLVM::emit_sctrl(cell index) {
  // set the indexed special registers to the value in PRI.
  // The index parameter must be:
  // 6=CIP
}

void CompilerLLVM::emit_move_pri() {
  // PRI = ALT
}

void CompilerLLVM::emit_move_alt() {
  // ALT = PRI
}

void CompilerLLVM::emit_xchg() {
  // Exchange PRI and ALT
}

void CompilerLLVM::emit_push_pri() {
  // [STK] = PRI, STK = STK - cell size
}

void CompilerLLVM::emit_push_alt() {
  // [STK] = ALT, STK = STK - cell size
}

void CompilerLLVM::emit_push_c(cell value) {
  // [STK] = value, STK = STK - cell size
}

void CompilerLLVM::emit_push(cell address) {
  // [STK] = [address], STK = STK - cell size
}

void CompilerLLVM::emit_push_s(cell offset) {
  // [STK] = [FRM + offset], STK = STK - cell size
}

void CompilerLLVM::emit_pop_pri() {
  // STK = STK + cell size, PRI = [STK]
}

void CompilerLLVM::emit_pop_alt() {
  // STK = STK + cell size, ALT = [STK]
}

void CompilerLLVM::emit_stack(cell value) {
  // ALT = STK, STK = STK + value
}

void CompilerLLVM::emit_heap(cell value) {
  // ALT = HEA, HEA = HEA + value
}

void CompilerLLVM::emit_proc() {
  // [STK] = FRM, STK = STK - cell size, FRM = STK
}

void CompilerLLVM::emit_ret() {
  // STK = STK + cell size, FRM = [STK],
  // CIP = [STK], STK = STK + cell size
}

void CompilerLLVM::emit_retn() {
  // STK = STK + cell size, FRM = [STK],
  // CIP = [STK], STK = STK + cell size
  // The RETN instruction removes a specified number of bytes
  // from the stack. The value to adjust STK with must be
  // pushed prior to the call.
}

void CompilerLLVM::emit_call(cell address) {
  // [STK] = CIP + 5, STK = STK - cell size
  // CIP = CIP + offset
  // The CALL instruction jumps to an address after storing the
  // address of the next sequential instruction on the stack.
  // The address jumped to is relative to the current CIP,
  // but the address on the stack is an absolute address.
}

void CompilerLLVM::emit_jump_pri() {
  // CIP = PRI (indirect jump)
}

void CompilerLLVM::emit_jump(cell address) {
  // CIP = CIP + offset (jump to the address relative from
  // the current position)
}

void CompilerLLVM::emit_jzer(cell address) {
  // if PRI == 0 then CIP = CIP + offset
}

void CompilerLLVM::emit_jnz(cell address) {
  // if PRI != 0 then CIP = CIP + offset
}

void CompilerLLVM::emit_jeq(cell address) {
  // if PRI == ALT then CIP = CIP + offset
}

void CompilerLLVM::emit_jneq(cell address) {
  // if PRI != ALT then CIP = CIP + offset
}

void CompilerLLVM::emit_jless(cell address) {
  // if PRI < ALT then CIP = CIP + offset (unsigned)
}

void CompilerLLVM::emit_jleq(cell address) {
  // if PRI <= ALT then CIP = CIP + offset (unsigned)
}

void CompilerLLVM::emit_jgrtr(cell address) {
  // if PRI > ALT then CIP = CIP + offset (unsigned)
}

void CompilerLLVM::emit_jgeq(cell address) {
  // if PRI >= ALT then CIP = CIP + offset (unsigned)
}

void CompilerLLVM::emit_jsless(cell address) {
  // if PRI < ALT then CIP = CIP + offset (signed)
}

void CompilerLLVM::emit_jsleq(cell address) {
  // if PRI <= ALT then CIP = CIP + offset (signed)
}

void CompilerLLVM::emit_jsgrtr(cell address) {
  // if PRI > ALT then CIP = CIP + offset (signed)
}

void CompilerLLVM::emit_jsgeq(cell address) {
  // if PRI >= ALT then CIP = CIP + offset (signed)
}

void CompilerLLVM::emit_shl() {
  // PRI = PRI << ALT
}

void CompilerLLVM::emit_shr() {
  // PRI = PRI >> ALT (without sign extension)
}

void CompilerLLVM::emit_sshr() {
  // PRI = PRI >> ALT with sign extension
}

void CompilerLLVM::emit_shl_c_pri(cell value) {
  // PRI = PRI << value
}

void CompilerLLVM::emit_shl_c_alt(cell value) {
  // ALT = ALT << value
}

void CompilerLLVM::emit_shr_c_pri(cell value) {
  // PRI = PRI >> value (without sign extension)
}

void CompilerLLVM::emit_shr_c_alt(cell value) {
  // PRI = PRI >> value (without sign extension)
}

void CompilerLLVM::emit_smul() {
  // PRI = PRI * ALT (signed multiply)
}

void CompilerLLVM::emit_sdiv() {
  // PRI = PRI / ALT (signed divide), ALT = PRI mod ALT
}

void CompilerLLVM::emit_sdiv_alt() {
  // PRI = ALT / PRI (signed divide), ALT = ALT mod PRI
}

void CompilerLLVM::emit_umul() {
  // PRI = PRI * ALT (unsigned multiply)
}

void CompilerLLVM::emit_udiv() {
  // PRI = PRI / ALT (unsigned divide), ALT = PRI mod ALT
}

void CompilerLLVM::emit_udiv_alt() {
  // PRI = ALT / PRI (unsigned divide), ALT = ALT mod PRI
}

void CompilerLLVM::emit_add() {
  // PRI = PRI + ALT
}

void CompilerLLVM::emit_sub() {
  // PRI = PRI - ALT
}

void CompilerLLVM::emit_sub_alt() {
  // PRI = ALT - PRI
  // or:
  // PRI = -(PRI - ALT)
}

void CompilerLLVM::emit_and() {
  // PRI = PRI & ALT
}

void CompilerLLVM::emit_or() {
  // PRI = PRI | ALT
}

void CompilerLLVM::emit_xor() {
  // PRI = PRI ^ ALT
}

void CompilerLLVM::emit_not() {
  // PRI = !PRI
}

void CompilerLLVM::emit_neg() {
  // PRI = -PRI
}

void CompilerLLVM::emit_invert() {
  // PRI = ~PRI
}

void CompilerLLVM::emit_add_c(cell value) {
  // PRI = PRI + value
}

void CompilerLLVM::emit_smul_c(cell value) {
  // PRI = PRI * value
}

void CompilerLLVM::emit_zero_pri() {
  // PRI = 0
}

void CompilerLLVM::emit_zero_alt() {
  // ALT = 0
}

void CompilerLLVM::emit_zero(cell address) {
  // [address] = 0
}

void CompilerLLVM::emit_zero_s(cell offset) {
  // [FRM + offset] = 0
}

void CompilerLLVM::emit_sign_pri() {
  // sign extent the byte in PRI to a cell
}

void CompilerLLVM::emit_sign_alt() {
  // sign extent the byte in ALT to a cell
}

void CompilerLLVM::emit_eq() {
  // PRI = PRI == ALT ? 1 :
}

void CompilerLLVM::emit_neq() {
  // PRI = PRI != ALT ? 1 :
}

void CompilerLLVM::emit_less() {
  // PRI = PRI < ALT ? 1 :
}

void CompilerLLVM::emit_leq() {
  // PRI = PRI <= ALT ? 1 :
}

void CompilerLLVM::emit_grtr() {
  // PRI = PRI > ALT ? 1 :
}

void CompilerLLVM::emit_geq() {
  // PRI = PRI >= ALT ? 1 :
}

void CompilerLLVM::emit_sless() {
  // PRI = PRI < ALT ? 1 :
}

void CompilerLLVM::emit_sleq() {
  // PRI = PRI <= ALT ? 1 :
}

void CompilerLLVM::emit_sgrtr() {
  // PRI = PRI > ALT ? 1 :
}

void CompilerLLVM::emit_sgeq() {
  // PRI = PRI >= ALT ? 1 :
}

void CompilerLLVM::emit_eq_c_pri(cell value) {
  // PRI = PRI == value ? 1 :
}

void CompilerLLVM::emit_eq_c_alt(cell value) {
  // PRI = ALT == value ? 1 :
}

void CompilerLLVM::emit_inc_pri() {
  // PRI = PRI + 1
}

void CompilerLLVM::emit_inc_alt() {
  // ALT = ALT + 1
}

void CompilerLLVM::emit_inc(cell address) {
  // [address] = [address] + 1
}

void CompilerLLVM::emit_inc_s(cell offset) {
  // [FRM + offset] = [FRM + offset] + 1
}

void CompilerLLVM::emit_inc_i() {
  // [PRI] = [PRI] + 1
}

void CompilerLLVM::emit_dec_pri() {
  // PRI = PRI - 1
}

void CompilerLLVM::emit_dec_alt() {
  // ALT = ALT - 1
}

void CompilerLLVM::emit_dec(cell address) {
  // [address] = [address] - 1
}

void CompilerLLVM::emit_dec_s(cell offset) {
  // [FRM + offset] = [FRM + offset] - 1
}

void CompilerLLVM::emit_dec_i() {
  // [PRI] = [PRI] - 1
}

void CompilerLLVM::emit_movs(cell num_bytes) {
  // Copy memory from [PRI] to [ALT]. The parameter
  // specifies the number of bytes. The blocks should not
  // overlap.
}

void CompilerLLVM::emit_cmps(cell num_bytes) {
  // Compare memory blocks at [PRI] and [ALT]. The parameter
  // specifies the number of bytes. The blocks should not
  // overlap.
}

void CompilerLLVM::emit_fill(cell num_bytes) {
  // Fill memory at [ALT] with value in [PRI]. The parameter
  // specifies the number of bytes, which must be a multiple
  // of the cell size.
}

void CompilerLLVM::emit_halt(cell error_code) {
  // Abort execution (exit value in PRI), parameters other than 0
  // have a special meaning.
}

void CompilerLLVM::emit_bounds(cell value) {
  // Abort execution if PRI > value or if PRI < 0.
}

void CompilerLLVM::emit_sysreq_pri() {
  // call system service, service number in PRI
}

void CompilerLLVM::emit_sysreq_c(cell index, const char *name) {
  // call system service
}

void CompilerLLVM::emit_sysreq_d(cell address, const char *name) {
  // call system service
}

void CompilerLLVM::emit_switch(const CaseTable &case_table) {
  // Compare PRI to the values in the case table (whose address
  // is passed as an offset from CIP) and jump to the associated
  // address in the matching record.
}

void CompilerLLVM::emit_casetbl() {
  // A variable number of case records follows this opcode, where
  // each record takes two cells.
}

void CompilerLLVM::emit_swap_pri() {
  // [STK] = PRI and PRI = [STK]
}

void CompilerLLVM::emit_swap_alt() {
  // [STK] = ALT and ALT = [STK]
}

void CompilerLLVM::emit_push_adr(cell offset) {
  // [STK] = FRM + offset, STK = STK - cell size
}

void CompilerLLVM::emit_nop() {
  // no-operation, for code alignment
}

void CompilerLLVM::emit_break() {
  // conditional breakpoint
}

} // namespace amxjit
