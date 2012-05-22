// Copyright (c) 2012, Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// // LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <cassert>
#include <cstddef>
#include <cstring>
#include <map>
#include <memory>
#include <string>

#include <amx/amx.h>
#include <AsmJit/X86/X86Assembler.h>
#include <AsmJit/Core/MemoryManager.h>

#include "jit.h"

namespace jit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AmxVm implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cell AmxVm::getPublicAddress(cell index) const {
	if (index == AMX_EXEC_MAIN) {
		return getHeader()->cip;
	}
	if (index < 0 || index >= getNumPublics()) {
		return 0;
	}
	return getPublics()[index].address;
}

cell AmxVm::getNativeAddress(int index) const {
	if (index < 0 || index >= getNumNatives()) {
		return 0;
	}
	return getNatives()[index].address;
}

int AmxVm::getPublicIndex(cell address) const {
	if (address == getHeader()->cip) {
		return AMX_EXEC_MAIN;
	}
	for (int i = 0; i < getNumPublics(); i++) {
		if (getPublics()[i].address == address) {
			return i;
		}
	}
	return -1;
}

int AmxVm::getNativeIndex(cell address) const {
	for (int i = 0; i < getNumNatives(); i++) {
		if (getNatives()[i].address == address) {
			return i;
		}
	}
	return -1;
}

const char *AmxVm::getPublicName(int index) const {
	if (index == AMX_EXEC_MAIN) {
		return "main";
	}
	if (index < 0 || index >= getNumPublics()) {
		return 0;
	}
	return reinterpret_cast<char*>(amx_->base + getPublics()[index].nameofs);
}

const char *AmxVm::getNativeName(int index) const {
	if (index < 0 || index >= getNumNatives()) {
		return 0;
	}
	return reinterpret_cast<char*>(amx_->base + getNatives()[index].nameofs);
}

cell *AmxVm::pushStack(cell value) {
	amx_->stk -= sizeof(cell);
	cell *stack = getStack();
	*stack = value;
	return stack;
}

cell AmxVm::popStack() {
	cell *stack = getStack();
	amx_->stk += sizeof(cell);
	return *stack;
}

void AmxVm::popStack(int ncells) {
	amx_->stk += ncells * sizeof(cell);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AmxDisassembler implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AmxDisassembler::AmxDisassembler(AmxVm vm)
	: vm_(vm)
	, opcodeTable_(0)
	, ip_(0)
{
}

bool AmxDisassembler::nextInstr() {
	if (ip_ < 0 || vm_.getHeader()->cod + ip_ >= vm_.getHeader()->dat) {
		// Went out of code, stop here.
		return false;
	}

	cell opcode = *reinterpret_cast<cell*>(vm_.getCode() + ip_);

	if (opcodeTable_ != 0) {
		// Lookup this opcode in opcode table.
		for (int i = 0; i < NUM_AMX_OPCODES; i++) {
			if (opcodeTable_[i] == opcode) {
				opcode = i;
				break;
			}
		}
	}

	ip_ += sizeof(cell);

	switch (opcode) {
	// Instructions with one operand.
	case OP_LOAD_PRI:
	case OP_LOAD_ALT:
	case OP_LOAD_S_PRI:
	case OP_LOAD_S_ALT:
	case OP_LREF_PRI:
	case OP_LREF_ALT:
	case OP_LREF_S_PRI:
	case OP_LREF_S_ALT:
	case OP_LODB_I:
	case OP_CONST_PRI:
	case OP_CONST_ALT:
	case OP_ADDR_PRI:
	case OP_ADDR_ALT:
	case OP_STOR_PRI:
	case OP_STOR_ALT:
	case OP_STOR_S_PRI:
	case OP_STOR_S_ALT:
	case OP_SREF_PRI:
	case OP_SREF_ALT:
	case OP_SREF_S_PRI:
	case OP_SREF_S_ALT:
	case OP_STRB_I:
	case OP_LIDX_B:
	case OP_IDXADDR_B:
	case OP_ALIGN_PRI:
	case OP_ALIGN_ALT:
	case OP_LCTRL:
	case OP_SCTRL:
	case OP_PUSH_R:
	case OP_PUSH_C:
	case OP_PUSH:
	case OP_PUSH_S:
	case OP_STACK:
	case OP_HEAP:
	case OP_JREL:
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
	case OP_SHL_C_PRI:
	case OP_SHL_C_ALT:
	case OP_SHR_C_PRI:
	case OP_SHR_C_ALT:
	case OP_ADD_C:
	case OP_SMUL_C:
	case OP_ZERO:
	case OP_ZERO_S:
	case OP_EQ_C_PRI:
	case OP_EQ_C_ALT:
	case OP_INC:
	case OP_INC_S:
	case OP_DEC:
	case OP_DEC_S:
	case OP_MOVS:
	case OP_CMPS:
	case OP_FILL:
	case OP_HALT:
	case OP_BOUNDS:
	case OP_CALL:
	case OP_SYSREQ_C:
	case OP_PUSH_ADR:
	case OP_SYSREQ_D:
	case OP_SWITCH:
		ip_ += sizeof(cell);
		break;

	// Instructions with no operands.
	case OP_LOAD_I:
	case OP_STOR_I:
	case OP_LIDX:
	case OP_IDXADDR:
	case OP_MOVE_PRI:
	case OP_MOVE_ALT:
	case OP_XCHG:
	case OP_PUSH_PRI:
	case OP_PUSH_ALT:
	case OP_POP_PRI:
	case OP_POP_ALT:
	case OP_PROC:
	case OP_RET:
	case OP_RETN:
	case OP_CALL_PRI:
	case OP_SHL:
	case OP_SHR:
	case OP_SSHR:
	case OP_SMUL:
	case OP_SDIV:
	case OP_SDIV_ALT:
	case OP_UMUL:
	case OP_UDIV:
	case OP_UDIV_ALT:
	case OP_ADD:
	case OP_SUB:
	case OP_SUB_ALT:
	case OP_AND:
	case OP_OR:
	case OP_XOR:
	case OP_NOT:
	case OP_NEG:
	case OP_INVERT:
	case OP_ZERO_PRI:
	case OP_ZERO_ALT:
	case OP_SIGN_PRI:
	case OP_SIGN_ALT:
	case OP_EQ:
	case OP_NEQ:
	case OP_LESS:
	case OP_LEQ:
	case OP_GRTR:
	case OP_GEQ:
	case OP_SLESS:
	case OP_SLEQ:
	case OP_SGRTR:
	case OP_SGEQ:
	case OP_INC_PRI:
	case OP_INC_ALT:
	case OP_INC_I:
	case OP_DEC_PRI:
	case OP_DEC_ALT:
	case OP_DEC_I:
	case OP_SYSREQ_PRI:
	case OP_JUMP_PRI:
	case OP_SWAP_PRI:
	case OP_SWAP_ALT:
	case OP_NOP:
	case OP_BREAK:
		break;

	// Special instructions.
	case OP_CASETBL: {
		int num = *reinterpret_cast<cell*>(vm_.getCode() + ip_);
		ip_ += sizeof(cell);
		ip_ += 2 * num + 1;
		break;
	}

	default:
		throw InvalidInstructionError(getInstr());
	}

	return true;
}

AmxInstruction AmxDisassembler::getInstr() const {
	return AmxInstruction(reinterpret_cast<cell*>(vm_.getCode() + ip_));
}

std::vector<AmxInstruction> AmxDisassembler::disassemble() {
	std::vector<AmxInstruction> instrs;
	while (nextInstr()) {
		instrs.push_back(getInstr());
	}
	return instrs;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CallContext implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CallContext::CallContext(AmxVm vm)
	: vm_(vm)
	, params_(0)
{
	AMX *amx = vm_.getAmx();
	params_ = vm_.pushStack(amx->paramcount * sizeof(cell));
	amx->paramcount = 0;
}

CallContext::~CallContext() {
	int paramcount = vm_.popStack();
	vm_.popStack(paramcount / sizeof(cell));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Jitter implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// JIT compiler intrinsics.
Jitter::Intrinsic Jitter::intrinsics_[] = {
	{"float",       &Jitter::native_float},
	{"float",       &Jitter::native_float},
	{"floatabs",    &Jitter::native_floatabs},
	{"floatadd",    &Jitter::native_floatadd},
	{"floatsub",    &Jitter::native_floatsub},
	{"floatmul",    &Jitter::native_floatmul},
	{"floatdiv",    &Jitter::native_floatdiv},
	{"floatsqroot", &Jitter::native_floatsqroot},
	{"floatlog",    &Jitter::native_floatlog}
};

Jitter::Jitter(AMX *amx, cell *opcode_list)
	: vm_(amx)
	, opcodeList_(opcode_list)
	, code_(0)
	, codeSize_(0)
	, haltEsp_(0)
	, haltEbp_(0)
	, callFunctionHelper_(0)
	, codeMap_(0)
	, labelMap_(0)
{
}

Jitter::~Jitter() {
	if (code_ != 0) {
		AsmJit::MemoryManager::getGlobal()->free(code_);
	}
}

void Jitter::compile(std::FILE *list_stream) {
	if (code_ != 0) {
		return;
	}

	AmxDisassembler disasm(vm_);
	std::vector<AmxInstruction> instrs = disasm.disassemble();

	AsmJit::X86Assembler as;
	AsmJit::FileLogger logger(list_stream);
	as.setLogger(&logger);

	std::auto_ptr<CodeMap> code_map(new CodeMap);
	std::auto_ptr<LabelMap> labelMap(new LabelMap);

	cell current_function = 0;

	for (std::vector<AmxInstruction>::iterator instr_iterator = instrs.begin();
			instr_iterator != instrs.end(); ++instr_iterator)
	{
		AmxInstruction &instr = *instr_iterator;

		cell cip = reinterpret_cast<cell>(instr.getPtr())
		         - reinterpret_cast<cell>(vm_.getCode());
		as.bind(L(as, labelMap.get(), cip));

		code_map->insert(std::make_pair(cip, as.getCodeSize()));

		switch (instr.getOpcode()) {
		case OP_LOAD_PRI: // address
			// PRI = [address]
			as.mov(AsmJit::eax, AsmJit::dword_ptr_abs(reinterpret_cast<void*>(vm_.getData() + instr.getOperand())));
			break;
		case OP_LOAD_ALT: // address
			// PRI = [address]
			as.mov(AsmJit::ecx, AsmJit::dword_ptr_abs(reinterpret_cast<void*>(vm_.getData() + instr.getOperand())));
			break;
		case OP_LOAD_S_PRI: // offset
			// PRI = [FRM + offset]
			as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand()));
			break;
		case OP_LOAD_S_ALT: // offset
			// ALT = [FRM + offset]
			as.mov(AsmJit::ecx, AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand()));
			break;
		case OP_LREF_PRI: // address
			// PRI = [ [address] ]
			as.mov(AsmJit::edx, AsmJit::dword_ptr_abs(reinterpret_cast<void*>(vm_.getData() + instr.getOperand())));
			as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::edx, reinterpret_cast<sysint_t>(vm_.getData())));
			break;
		case OP_LREF_ALT: // address
			// ALT = [ [address] ]
			as.mov(AsmJit::edx, AsmJit::dword_ptr_abs(reinterpret_cast<void*>(vm_.getData() + instr.getOperand())));
			as.mov(AsmJit::ecx, AsmJit::dword_ptr(AsmJit::edx, reinterpret_cast<sysint_t>(vm_.getData())));
			break;
		case OP_LREF_S_PRI: // offset
			// PRI = [ [FRM + offset] ]
			as.mov(AsmJit::edx, AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand()));
			as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::edx, reinterpret_cast<sysint_t>(vm_.getData())));
			break;
		case OP_LREF_S_ALT: // offset
			// PRI = [ [FRM + offset] ]
			as.mov(AsmJit::edx, AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand()));
			as.mov(AsmJit::ecx, AsmJit::dword_ptr(AsmJit::edx, reinterpret_cast<sysint_t>(vm_.getData())));
			break;
		case OP_LOAD_I:
			// PRI = [PRI] (full cell)
			as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::eax, reinterpret_cast<sysint_t>(vm_.getData())));
			break;
		case OP_LODB_I: // number
			// PRI = "number" bytes from [PRI] (read 1/2/4 bytes)
			switch (instr.getOperand()) {
			case 1:
				as.movzx(AsmJit::eax, AsmJit::byte_ptr(AsmJit::eax, reinterpret_cast<sysint_t>(vm_.getData())));
				break;
			case 2:
				as.movzx(AsmJit::eax, AsmJit::word_ptr(AsmJit::eax, reinterpret_cast<sysint_t>(vm_.getData())));
				break;
			case 4:
				as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::eax, reinterpret_cast<sysint_t>(vm_.getData())));
				break;
			}
			break;
		case OP_CONST_PRI: // value
			// PRI = value
			as.mov(AsmJit::eax, instr.getOperand());
			break;
		case OP_CONST_ALT: // value
			// ALT = value
			as.mov(AsmJit::ecx, instr.getOperand());
			break;
		case OP_ADDR_PRI: // offset
			// PRI = FRM + offset
			as.lea(AsmJit::eax, AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand() - reinterpret_cast<sysint_t>(vm_.getData())));
			break;
		case OP_ADDR_ALT: // offset
			// ALT = FRM + offset
			as.lea(AsmJit::ecx, AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand() - reinterpret_cast<sysint_t>(vm_.getData())));
			break;
		case OP_STOR_PRI: // address
			// [address] = PRI
			as.mov(AsmJit::dword_ptr_abs(reinterpret_cast<void*>(vm_.getData() + instr.getOperand())), AsmJit::eax);
			break;
		case OP_STOR_ALT: // address
			// [address] = ALT
			as.mov(AsmJit::dword_ptr_abs(reinterpret_cast<void*>(vm_.getData() + instr.getOperand())), AsmJit::ecx);
			break;
		case OP_STOR_S_PRI: // offset
			// [FRM + offset] = ALT
			as.mov(AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand()), AsmJit::eax);
			break;
		case OP_STOR_S_ALT: // offset
			// [FRM + offset] = ALT
			as.mov(AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand()), AsmJit::ecx);
			break;
		case OP_SREF_PRI: // address
			// [ [address] ] = PRI
			as.mov(AsmJit::edx, AsmJit::dword_ptr_abs(reinterpret_cast<void*>(vm_.getData() + instr.getOperand())));
			as.mov(AsmJit::dword_ptr(AsmJit::edx, reinterpret_cast<sysint_t>(vm_.getData())), AsmJit::eax);
			break;
		case OP_SREF_ALT: // address
			// [ [address] ] = ALT
			as.mov(AsmJit::edx, AsmJit::dword_ptr_abs(reinterpret_cast<void*>(vm_.getData() + instr.getOperand())));
			as.mov(AsmJit::dword_ptr(AsmJit::edx, reinterpret_cast<sysint_t>(vm_.getData())), AsmJit::ecx);
			break;
		case OP_SREF_S_PRI: // offset
			// [ [FRM + offset] ] = PRI
			as.mov(AsmJit::edx, AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand()));
			as.mov(AsmJit::dword_ptr(AsmJit::edx, reinterpret_cast<sysint_t>(vm_.getData())), AsmJit::eax);
			break;
		case OP_SREF_S_ALT: // offset
			// [ [FRM + offset] ] = ALT
			as.mov(AsmJit::edx, AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand()));
			as.mov(AsmJit::dword_ptr(AsmJit::edx, reinterpret_cast<sysint_t>(vm_.getData())), AsmJit::ecx);
			break;
		case OP_STOR_I:
			// [ALT] = PRI (full cell)
			as.mov(AsmJit::dword_ptr(AsmJit::ecx, reinterpret_cast<sysint_t>(vm_.getData())), AsmJit::eax);
			break;
		case OP_STRB_I: // number
			// "number" bytes at [ALT] = PRI (write 1/2/4 bytes)
			switch (instr.getOperand()) {
			case 1:
				as.mov(AsmJit::byte_ptr(AsmJit::ecx, reinterpret_cast<sysint_t>(vm_.getData())), AsmJit::al);
				break;
			case 2:
				as.mov(AsmJit::word_ptr(AsmJit::ecx, reinterpret_cast<sysint_t>(vm_.getData())), AsmJit::ax);
				break;
			case 4:
				as.mov(AsmJit::dword_ptr(AsmJit::ecx, reinterpret_cast<sysint_t>(vm_.getData())), AsmJit::eax);
				break;
			}
			break;
		case OP_LIDX:
			// PRI = [ ALT + (PRI x cell size) ]
			as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::ecx, AsmJit::eax, 2, reinterpret_cast<sysint_t>(vm_.getData())));
			break;
		case OP_LIDX_B: // shift
			// PRI = [ ALT + (PRI << shift) ]
			as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::ecx, AsmJit::eax, instr.getOperand(), reinterpret_cast<sysint_t>(vm_.getData())));
			break;
		case OP_IDXADDR:
			// PRI = ALT + (PRI x cell size) (calculate indexed address)
			as.lea(AsmJit::eax, AsmJit::dword_ptr(AsmJit::ecx, AsmJit::eax, 2));
			break;
		case OP_IDXADDR_B: // shift
			// PRI = ALT + (PRI << shift) (calculate indexed address)
			as.lea(AsmJit::eax, AsmJit::dword_ptr(AsmJit::ecx, AsmJit::eax, instr.getOperand()));
			break;
		case OP_ALIGN_PRI: // number
			// Little Endian: PRI ^= cell size - number
			#if BYTE_ORDER == LITTLE_ENDIAN
				if (instr.getOperand() < sizeof(cell)) {
					as.xor_(AsmJit::eax, sizeof(cell) - instr.getOperand());
				}
			#endif
			break;
		case OP_ALIGN_ALT: // number
			// Little Endian: ALT ^= cell size - number
			#if BYTE_ORDER == LITTLE_ENDIAN
				if (instr.getOperand() < sizeof(cell)) {
					as.xor_(AsmJit::ecx, sizeof(cell) - instr.getOperand());
				}
			#endif
			break;
		case OP_LCTRL: // index
			// PRI is set to the current value of any of the special registers.
			// The index parameter must be: 0=COD, 1=DAT, 2=HEA,
			// 3=STP, 4=STK, 5=FRM, 6=CIP (of the next instruction)
			switch (instr.getOperand()) {
			case 0:
				as.mov(AsmJit::eax, AsmJit::dword_ptr_abs(reinterpret_cast<void*>(&vm_.getHeader()->cod)));
				break;
			case 1:
				as.mov(AsmJit::eax, AsmJit::dword_ptr_abs(reinterpret_cast<void*>(&vm_.getHeader()->dat)));
				break;
			case 2:
				as.mov(AsmJit::eax, AsmJit::dword_ptr_abs(reinterpret_cast<void*>(&vm_.getAmx()->hea)));
				break;
			case 4:
				as.lea(AsmJit::eax, AsmJit::dword_ptr(AsmJit::esp, -reinterpret_cast<sysint_t>(vm_.getData())));
				break;
			case 5:
				as.lea(AsmJit::eax, AsmJit::dword_ptr(AsmJit::ebp, -reinterpret_cast<sysint_t>(vm_.getData())));
				break;
			case 6: {
				if (instr_iterator == instrs.end() - 1) {
					// Can't get address of next instruction since this one is the last.
					throw InvalidInstructionError(instr);
				}
				AmxInstruction &next_instr = *(instr_iterator + 1);
				as.mov(AsmJit::eax, reinterpret_cast<sysint_t>(next_instr.getPtr())
				            - reinterpret_cast<sysint_t>(vm_.getCode()));
				break;
			}
			default:
				throw UnsupportedInstructionError(instr);
			}
			break;
		case OP_SCTRL: // index
			// set the indexed special registers to the value in PRI.
			// The index parameter must be: 2=HEA, 4=STK, 5=FRM,
			// 6=CIP
			switch (instr.getOperand()) {
			case 2:
				as.mov(AsmJit::dword_ptr_abs(reinterpret_cast<void*>(&vm_.getAmx()->hea)), AsmJit::eax);
				break;
			case 4:
				as.lea(AsmJit::esp, AsmJit::dword_ptr(AsmJit::eax, reinterpret_cast<sysint_t>(vm_.getData())));
				break;
			case 5:
				as.lea(AsmJit::ebp, AsmJit::dword_ptr(AsmJit::eax, reinterpret_cast<sysint_t>(vm_.getData())));
				break;
			case 6:
				as.push(AsmJit::esp);
				as.push(AsmJit::eax);
				as.push(reinterpret_cast<sysint_t>(this));
				as.call(reinterpret_cast<void*>(Jitter::doJump));
				// Didn't jump because of invalid address - exit with error.
				halt(as, AMX_ERR_INVINSTR);
				break;
			default:
				throw UnsupportedInstructionError(instr);
			}
			break;
		case OP_MOVE_PRI:
			// PRI = ALT
			as.mov(AsmJit::eax, AsmJit::ecx);
			break;
		case OP_MOVE_ALT:
			// ALT = PRI
			as.mov(AsmJit::ecx, AsmJit::eax);
			break;
		case OP_XCHG:
			// Exchange PRI and ALT
			as.xchg(AsmJit::eax, AsmJit::ecx);
			break;
		case OP_PUSH_PRI:
			// [STK] = PRI, STK = STK - cell size
			as.push(AsmJit::eax);
			break;
		case OP_PUSH_ALT:
			// [STK] = ALT, STK = STK - cell size
			as.push(AsmJit::ecx);
			break;
		case OP_PUSH_C: // value
			// [STK] = value, STK = STK - cell size
			as.push(instr.getOperand());
			break;
		case OP_PUSH: // address
			// [STK] = [address], STK = STK - cell size
			as.push(AsmJit::dword_ptr_abs(reinterpret_cast<void*>(instr.getOperand() + vm_.getData())));
			break;
		case OP_PUSH_S: // offset
			// [STK] = [FRM + offset], STK = STK - cell size
			as.push(AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand()));
			break;
		case OP_POP_PRI:
			// STK = STK + cell size, PRI = [STK]
			as.pop(AsmJit::eax);
			break;
		case OP_POP_ALT:
			// STK = STK + cell size, ALT = [STK]
			as.pop(AsmJit::ecx);
			break;
		case OP_STACK: // value
			// ALT = STK, STK = STK + value
			as.lea(AsmJit::ecx, AsmJit::dword_ptr(AsmJit::esp, -reinterpret_cast<sysint_t>(vm_.getData())));
			as.add(AsmJit::esp, instr.getOperand());
			break;
		case OP_HEAP: // value
			// ALT = HEA, HEA = HEA + value
			as.mov(AsmJit::ecx, AsmJit::dword_ptr_abs(reinterpret_cast<void*>(&vm_.getAmx()->hea)));
			as.add(AsmJit::dword_ptr_abs(reinterpret_cast<void*>(&vm_.getAmx()->hea)), instr.getOperand());
			break;
		case OP_PROC:
			// [STK] = FRM, STK = STK - cell size, FRM = STK
			as.lea(AsmJit::edx, AsmJit::dword_ptr(AsmJit::ebp, -reinterpret_cast<sysint_t>(vm_.getData())));
			as.push(AsmJit::edx);
			as.mov(AsmJit::ebp, AsmJit::esp);
			current_function = cip;
			break;
		case OP_RET:
		case OP_RETN:
			// STK = STK + cell size, FRM = [STK],
			// CIP = [STK], STK = STK + cell size
			// The RETN instruction removes a specified number of bytes
			// from the stack. The value to adjust STK with must be
			// pushed prior to the call.
			as.pop(AsmJit::ebp);
			as.add(AsmJit::ebp, reinterpret_cast<sysint_t>(vm_.getData()));
			as.ret();
			break;
		case OP_CALL: { // offset
			// [STK] = CIP + 5, STK = STK - cell size
			// CIP = CIP + offset
			// The CALL instruction jumps to an address after storing the
			// address of the next sequential instruction on the stack.
			// The address jumped to is relative to the current CIP,
			// but the address on the stack is an absolute address.
			cell fn_addr = instr.getOperand() - reinterpret_cast<cell>(vm_.getCode());
			as.call(L(as, labelMap.get(), fn_addr));
			as.add(AsmJit::esp, AsmJit::dword_ptr(AsmJit::esp));
			as.add(AsmJit::esp, 4);
			break;
		}
		case OP_CALL_PRI:
			// Same as CALL, the address to jump to is in PRI.
			as.call(AsmJit::eax);
			as.add(AsmJit::esp, AsmJit::dword_ptr(AsmJit::esp));
			as.add(AsmJit::esp, 4);
			break;

		case OP_JUMP:
		case OP_JUMP_PRI:
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
			cell dest = instr.getOperand() - reinterpret_cast<cell>(vm_.getCode());
			AsmJit::Label &L_dest = L(as, labelMap.get(), dest);

			switch (instr.getOpcode()) {
				case OP_JUMP: // offset
					// CIP = CIP + offset (jump to the address relative from
					// the current position)
					as.jmp(L_dest);
					break;
				case OP_JUMP_PRI:
					// CIP = PRI (indirect jump)
					as.push(AsmJit::esp);
					as.push(AsmJit::eax);
					as.push(reinterpret_cast<sysint_t>(this));
					as.call(reinterpret_cast<void*>(Jitter::doJump));
					// Didn't jump because of invalid address - exit with error.
					halt(as, AMX_ERR_INVINSTR);
					break;
				case OP_JZER: // offset
					// if PRI == 0 then CIP = CIP + offset
					as.cmp(AsmJit::eax, 0);
					as.jz(L_dest);
					break;
				case OP_JNZ: // offset
					// if PRI != 0 then CIP = CIP + offset
					as.cmp(AsmJit::eax, 0);
					as.jnz(L_dest);
					break;
				case OP_JEQ: // offset
					// if PRI == ALT then CIP = CIP + offset
					as.cmp(AsmJit::eax, AsmJit::ecx);
					as.je(L_dest);
					break;
				case OP_JNEQ: // offset
					// if PRI != ALT then CIP = CIP + offset
					as.cmp(AsmJit::eax, AsmJit::ecx);
					as.jne(L_dest);
					break;
				case OP_JLESS: // offset
					// if PRI < ALT then CIP = CIP + offset (unsigned)
					as.cmp(AsmJit::eax, AsmJit::ecx);
					as.jb(L_dest);
					break;
				case OP_JLEQ: // offset
					// if PRI <= ALT then CIP = CIP + offset (unsigned)
					as.cmp(AsmJit::eax, AsmJit::ecx);
					as.jbe(L_dest);
					break;
				case OP_JGRTR: // offset
					// if PRI > ALT then CIP = CIP + offset (unsigned)
					as.cmp(AsmJit::eax, AsmJit::ecx);
					as.ja(L_dest);
					break;
				case OP_JGEQ: // offset
					// if PRI >= ALT then CIP = CIP + offset (unsigned)
					as.cmp(AsmJit::eax, AsmJit::ecx);
					as.jae(L_dest);
					break;
				case OP_JSLESS: // offset
					// if PRI < ALT then CIP = CIP + offset (signed)
					as.cmp(AsmJit::eax, AsmJit::ecx);
					as.jl(L_dest);
					break;
				case OP_JSLEQ: // offset
					// if PRI <= ALT then CIP = CIP + offset (signed)
					as.cmp(AsmJit::eax, AsmJit::ecx);
					as.jle(L_dest);
					break;
				case OP_JSGRTR: // offset
					// if PRI > ALT then CIP = CIP + offset (signed)
					as.cmp(AsmJit::eax, AsmJit::ecx);
					as.jg(L_dest);
					break;
				case OP_JSGEQ: // offset
					// if PRI >= ALT then CIP = CIP + offset (signed)
					as.cmp(AsmJit::eax, AsmJit::ecx);
					as.jge(L_dest);
					break;
			}
			break;
		}

		case OP_SHL:
			// PRI = PRI << ALT
			as.shl(AsmJit::eax, AsmJit::cl);
			break;
		case OP_SHR:
			// PRI = PRI >> ALT (without sign extension)
			as.shr(AsmJit::eax, AsmJit::cl);
			break;
		case OP_SSHR:
			// PRI = PRI >> ALT with sign extension
			as.sar(AsmJit::eax, AsmJit::cl);
			break;
		case OP_SHL_C_PRI: // value
			// PRI = PRI << value
			as.shl(AsmJit::eax, static_cast<unsigned char>(instr.getOperand()));
			break;
		case OP_SHL_C_ALT: // value
			// ALT = ALT << value
			as.shl(AsmJit::ecx, static_cast<unsigned char>(instr.getOperand()));
			break;
		case OP_SHR_C_PRI: // value
			// PRI = PRI >> value (without sign extension)
			as.shr(AsmJit::eax, static_cast<unsigned char>(instr.getOperand()));
			break;
		case OP_SHR_C_ALT: // value
			// PRI = PRI >> value (without sign extension)
			as.shl(AsmJit::ecx, static_cast<unsigned char>(instr.getOperand()));
			break;
		case OP_SMUL:
			// PRI = PRI * ALT (signed multiply)
			as.xor_(AsmJit::edx, AsmJit::edx);
			as.imul(AsmJit::ecx);
			break;
		case OP_SDIV:
			// PRI = PRI / ALT (signed divide), ALT = PRI mod ALT
			as.xor_(AsmJit::edx, AsmJit::edx);
			as.idiv(AsmJit::ecx);
			as.mov(AsmJit::ecx, AsmJit::edx);
			break;
		case OP_SDIV_ALT:
			// PRI = ALT / PRI (signed divide), ALT = ALT mod PRI
			as.xchg(AsmJit::eax, AsmJit::ecx);
			as.xor_(AsmJit::edx, AsmJit::edx);
			as.idiv(AsmJit::ecx);
			as.mov(AsmJit::ecx, AsmJit::edx);
			break;
		case OP_UMUL:
			// PRI = PRI * ALT (unsigned multiply)
			as.xor_(AsmJit::edx, AsmJit::edx);
			as.mul(AsmJit::ecx);
			break;
		case OP_UDIV:
			// PRI = PRI / ALT (unsigned divide), ALT = PRI mod ALT
			as.xor_(AsmJit::edx, AsmJit::edx);
			as.div(AsmJit::ecx);
			as.mov(AsmJit::ecx, AsmJit::edx);
			break;
		case OP_UDIV_ALT:
			// PRI = ALT / PRI (unsigned divide), ALT = ALT mod PRI
			as.xchg(AsmJit::eax, AsmJit::ecx);
			as.xor_(AsmJit::edx, AsmJit::edx);
			as.div(AsmJit::ecx);
			as.mov(AsmJit::ecx, AsmJit::edx);
			break;
		case OP_ADD:
			// PRI = PRI + ALT
			as.add(AsmJit::eax, AsmJit::ecx);
			break;
		case OP_SUB:
			// PRI = PRI - ALT
			as.sub(AsmJit::eax, AsmJit::ecx);
			break;
		case OP_SUB_ALT:
			// PRI = ALT - PRI
			// or:
			// PRI = -(PRI - ALT)
			as.sub(AsmJit::eax, AsmJit::ecx);
			as.neg(AsmJit::eax);
			break;
		case OP_AND:
			// PRI = PRI & ALT
			as.and_(AsmJit::eax, AsmJit::ecx);
			break;
		case OP_OR:
			// PRI = PRI | ALT
			as.or_(AsmJit::eax, AsmJit::ecx);
			break;
		case OP_XOR:
			// PRI = PRI ^ ALT
			as.xor_(AsmJit::eax, AsmJit::ecx);
			break;
		case OP_NOT:
			// PRI = !PRI
			as.test(AsmJit::eax, AsmJit::eax);
			as.setz(AsmJit::cl);
			as.movzx(AsmJit::eax, AsmJit::cl);
			break;
		case OP_NEG:
			// PRI = -PRI
			as.neg(AsmJit::eax);
			break;
		case OP_INVERT:
			// PRI = ~PRI
			as.not_(AsmJit::eax);
			break;
		case OP_ADD_C: // value
			// PRI = PRI + value
			as.add(AsmJit::eax, instr.getOperand());
			break;
		case OP_SMUL_C: // value
			// PRI = PRI * value
			as.imul(AsmJit::eax, instr.getOperand());
			break;
		case OP_ZERO_PRI:
			// PRI = 0
			as.xor_(AsmJit::eax, AsmJit::eax);
			break;
		case OP_ZERO_ALT:
			// ALT = 0
			as.xor_(AsmJit::ecx, AsmJit::ecx);
			break;
		case OP_ZERO: // address
			// [address] = 0
			as.mov(AsmJit::dword_ptr_abs(reinterpret_cast<void*>(instr.getOperand() + vm_.getData())), 0);
			break;
		case OP_ZERO_S: // offset
			// [FRM + offset] = 0
			as.mov(AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand()), 0);
			break;
		case OP_SIGN_PRI:
			// sign extent the byte in PRI to a cell
			as.movsx(AsmJit::eax, AsmJit::al);
			break;
		case OP_SIGN_ALT:
			// sign extent the byte in ALT to a cell
			as.movsx(AsmJit::ecx, AsmJit::cl);
			break;
		case OP_EQ:
			// PRI = PRI == ALT ? 1 : 0
			as.cmp(AsmJit::eax, AsmJit::ecx);
			as.sete(AsmJit::al);
			as.movzx(AsmJit::eax, AsmJit::al);
			break;
		case OP_NEQ:
			// PRI = PRI != ALT ? 1 : 0
			as.cmp(AsmJit::eax, AsmJit::ecx);
			as.setne(AsmJit::al);
			as.movzx(AsmJit::eax, AsmJit::al);
			break;
		case OP_LESS:
			// PRI = PRI < ALT ? 1 : 0 (unsigned)
			as.cmp(AsmJit::eax, AsmJit::ecx);
			as.setb(AsmJit::al);
			as.movzx(AsmJit::eax, AsmJit::al);
			break;
		case OP_LEQ:
			// PRI = PRI <= ALT ? 1 : 0 (unsigned)
			as.cmp(AsmJit::eax, AsmJit::ecx);
			as.setbe(AsmJit::al);
			as.movzx(AsmJit::eax, AsmJit::al);
			break;
		case OP_GRTR:
			// PRI = PRI > ALT ? 1 : 0 (unsigned)
			as.cmp(AsmJit::eax, AsmJit::ecx);
			as.seta(AsmJit::al);
			as.movzx(AsmJit::eax, AsmJit::al);
			break;
		case OP_GEQ:
			// PRI = PRI >= ALT ? 1 : 0 (unsigned)
			as.cmp(AsmJit::eax, AsmJit::ecx);
			as.setae(AsmJit::al);
			as.movzx(AsmJit::eax, AsmJit::al);
			break;
		case OP_SLESS:
			// PRI = PRI < ALT ? 1 : 0 (signed)
			as.cmp(AsmJit::eax, AsmJit::ecx);
			as.setl(AsmJit::al);
			as.movzx(AsmJit::eax, AsmJit::al);
			break;
		case OP_SLEQ:
			// PRI = PRI <= ALT ? 1 : 0 (signed)
			as.cmp(AsmJit::eax, AsmJit::ecx);
			as.setle(AsmJit::al);
			as.movzx(AsmJit::eax, AsmJit::al);
			break;
		case OP_SGRTR:
			// PRI = PRI > ALT ? 1 : 0 (signed)
			as.cmp(AsmJit::eax, AsmJit::ecx);
			as.setg(AsmJit::al);
			as.movzx(AsmJit::eax, AsmJit::al);
			break;
		case OP_SGEQ:
			// PRI = PRI >= ALT ? 1 : 0 (signed)
			as.cmp(AsmJit::eax, AsmJit::ecx);
			as.setge(AsmJit::al);
			as.movzx(AsmJit::eax, AsmJit::al);
			break;
		case OP_EQ_C_PRI: // value
			// PRI = PRI == value ? 1 : 0
			as.cmp(AsmJit::eax, instr.getOperand());
			as.sete(AsmJit::al);
			as.movzx(AsmJit::eax, AsmJit::al);
			break;
		case OP_EQ_C_ALT: // value
			// PRI = ALT == value ? 1 : 0
			as.cmp(AsmJit::ecx, instr.getOperand());
			as.sete(AsmJit::al);
			as.movzx(AsmJit::eax, AsmJit::al);
			break;
		case OP_INC_PRI:
			// PRI = PRI + 1
			as.inc(AsmJit::eax);
			break;
		case OP_INC_ALT:
			// ALT = ALT + 1
			as.inc(AsmJit::ecx);
			break;
		case OP_INC: // address
			// [address] = [address] + 1
			as.inc(AsmJit::dword_ptr_abs(reinterpret_cast<void*>(vm_.getData() + instr.getOperand())));
			break;
		case OP_INC_S: // offset
			// [FRM + offset] = [FRM + offset] + 1
			as.inc(AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand()));
			break;
		case OP_INC_I:
			// [PRI] = [PRI] + 1
			as.inc(AsmJit::dword_ptr(AsmJit::eax, reinterpret_cast<sysint_t>(vm_.getData())));
			break;
		case OP_DEC_PRI:
			// PRI = PRI - 1
			as.dec(AsmJit::eax);
			break;
		case OP_DEC_ALT:
			// ALT = ALT - 1
			as.dec(AsmJit::ecx);
			break;
		case OP_DEC: // address
			// [address] = [address] - 1
			as.dec(AsmJit::dword_ptr_abs(reinterpret_cast<void*>(vm_.getData() + instr.getOperand())));
			break;
		case OP_DEC_S: // offset
			// [FRM + offset] = [FRM + offset] - 1
			as.dec(AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand()));
			break;
		case OP_DEC_I:
			// [PRI] = [PRI] - 1
			as.dec(AsmJit::dword_ptr(AsmJit::eax, reinterpret_cast<sysint_t>(vm_.getData())));
			break;
		case OP_MOVS: // number
			// Copy memory from [PRI] to [ALT]. The parameter
			// specifies the number of bytes. The blocks should not
			// overlap.
			as.lea(AsmJit::esi, AsmJit::dword_ptr(AsmJit::eax, reinterpret_cast<sysint_t>(vm_.getData())));
			as.lea(AsmJit::edi, AsmJit::dword_ptr(AsmJit::ecx, reinterpret_cast<sysint_t>(vm_.getData())));
			as.push(AsmJit::ecx);
			if (instr.getOperand() % 4 == 0) {
				as.mov(AsmJit::ecx, instr.getOperand() / 4);
				as.rep_movsd();
			} else if (instr.getOperand() % 2 == 0) {
				as.mov(AsmJit::ecx, instr.getOperand() / 2);
				as.rep_movsw();
			} else {
				as.mov(AsmJit::ecx, instr.getOperand());
				as.rep_movsb();
			}
			as.pop(AsmJit::ecx);
			break;
		case OP_CMPS: // number
			// Compare memory blocks at [PRI] and [ALT]. The parameter
			// specifies the number of bytes. The blocks should not
			// overlap.
			as.push(instr.getOperand());
			as.lea(AsmJit::edx, AsmJit::dword_ptr(AsmJit::ecx, reinterpret_cast<sysint_t>(vm_.getData())));
			as.push(AsmJit::edx);
			as.lea(AsmJit::edx, AsmJit::dword_ptr(AsmJit::eax, reinterpret_cast<sysint_t>(vm_.getData())));
			as.push(AsmJit::edx);
			as.call(reinterpret_cast<void*>(std::memcmp));
			as.call(AsmJit::edx);
			as.add(AsmJit::esp, 12);
			break;
		case OP_FILL: { // number
			// Fill memory at [ALT] with value in [PRI]. The parameter
			// specifies the number of bytes, which must be a multiple
			// of the cell size.
			AsmJit::Label &L_loop = L(as, labelMap.get(), cip, "loop");
			as.lea(AsmJit::edi, AsmJit::dword_ptr(AsmJit::ecx, reinterpret_cast<sysint_t>(vm_.getData())));                      // memory start
			as.lea(AsmJit::esi, AsmJit::dword_ptr(AsmJit::ecx, reinterpret_cast<sysint_t>(vm_.getData()) + instr.getOperand())); // memory end
			as.bind(L_loop);
				as.mov(AsmJit::dword_ptr(AsmJit::edi), AsmJit::eax);
				as.add(AsmJit::edi, sizeof(cell));
				as.cmp(AsmJit::edi, AsmJit::esi);
			as.jl(L_loop); // if AsmJit::edi < AsmJit::esi fill next cell
			break;
		}
		case OP_HALT: // number
			// Abort execution (exit value in PRI), parameters other than 0
			// have a special meaning.
			halt(as, instr.getOperand());
			break;
		case OP_BOUNDS: { // value
			// Abort execution if PRI > value or if PRI < 0.
			AsmJit::Label &L_halt = L(as, labelMap.get(), cip, "halt");
			AsmJit::Label &L_good = L(as, labelMap.get(), cip, "good");
				as.cmp(AsmJit::eax, instr.getOperand());
			as.jg(L_halt);
				as.cmp(AsmJit::eax, 0);
				as.jl(L_halt);
				as.jmp(L_good);
			as.bind(L_halt);
				halt(as, AMX_ERR_BOUNDS);
			as.bind(L_good);
			break;
		}
		case OP_SYSREQ_PRI: {
			// call system service, service number in PRI
			AsmJit::Label &L_halt = L(as, labelMap.get(), cip, "halt");
				as.push(AsmJit::eax);
				as.push(reinterpret_cast<sysint_t>(vm_.getAmx()));
				//as.call(reinterpret_cast<void*>(getNativeAddress));
				as.add(AsmJit::esp, 8);
				as.test(AsmJit::eax, AsmJit::eax);
				as.jz(L_halt);
				as.mov(AsmJit::edi, AsmJit::esp);
				beginExternalCode(as);
					as.push(AsmJit::edi);
					as.push(reinterpret_cast<sysint_t>(vm_.getAmx()));
					as.call(AsmJit::eax);
					as.add(AsmJit::esp, 8);
				endExternalCode(as);
			as.bind(L_halt);
				halt(as, AMX_ERR_NOTFOUND);
			break;
		}
		case OP_SYSREQ_C:   // index
		case OP_SYSREQ_D: { // address
			// call system service
			std::string nativeName;
			switch (instr.getOpcode()) {
				case OP_SYSREQ_C:
					nativeName = vm_.getNativeName(instr.getOperand());
					break;
				case OP_SYSREQ_D: {
					int index = vm_.getNativeIndex(instr.getOperand());
					if (index != -1) {
						nativeName = vm_.getNativeName(index);
					}
					break;
				}
			}
			// Replace calls to various natives with their optimized equivalents.
			for (int i = 0; i < sizeof(intrinsics_) / sizeof(Intrinsic); i++) {
				if (intrinsics_[i].name == nativeName) {
					(*this.*(intrinsics_[i].impl))(as);
					goto special_native;
				}
				goto ordinary_native;
			}
		ordinary_native:
			as.mov(AsmJit::edi, AsmJit::esp);
			beginExternalCode(as);
				as.push(AsmJit::edi);
				as.push(reinterpret_cast<sysint_t>(vm_.getAmx()));
				switch (instr.getOpcode()) {
					case OP_SYSREQ_C:
						as.call(reinterpret_cast<void*>(vm_.getNativeAddress(instr.getOperand())));
						break;
					case OP_SYSREQ_D:
						as.call(reinterpret_cast<void*>(instr.getOperand()));
						break;
				}
				as.add(AsmJit::esp, 8);
			endExternalCode(as);
		special_native:
			break;
		}
		case OP_SWITCH: { // offset
			// Compare PRI to the values in the case table (whose address
			// is passed as an offset from CIP) and jump to the associated
			// the address in the matching record.

			struct case_record {
				cell value;    // case value
				cell address;  // address to jump to (absolute)
			} *case_table;

			// Get pointer to the start of the case table.
			case_table = reinterpret_cast<case_record*>(instr.getOperand() + sizeof(cell));

			// Get address of the "default" record.
			cell default_addr = case_table[0].address - reinterpret_cast<cell>(vm_.getCode());

			// The number of cases follows the CASETBL opcode (which follows the SWITCH).
			int num_cases = *(reinterpret_cast<cell*>(instr.getOperand()) + 1);

			if (num_cases > 0) {
				// Get minimum and maximum values.
				cell *min_value = 0;
				cell *max_value = 0;
				for (int i = 0; i < num_cases; i++) {
					cell *value = &case_table[i + 1].value;
					if (min_value == 0 || *value < *min_value) {
						min_value = value;
					}
					if (max_value == 0 || *value > *max_value) {
						max_value = value;
					}
				}

				// Check if the value in AsmJit::eax is in the allowed range.
				// If not, jump to the default case (i.e. no match).
				as.cmp(AsmJit::eax, *min_value);
				as.jl(L(as, labelMap.get(), default_addr));
				as.cmp(AsmJit::eax, *max_value);
				as.jg(L(as, labelMap.get(), default_addr));

				// OK now sequentially compare AsmJit::eax with each value.
				// This is pretty slow so I probably should optimize
				// this in future...
				for (int i = 0; i < num_cases; i++) {
					as.cmp(AsmJit::eax, case_table[i + 1].value);
					as.je(L(as, labelMap.get(), case_table[i + 1].address - reinterpret_cast<cell>(vm_.getCode())));
				}
			}

			// No match found - go for default case.
			as.jmp(L(as, labelMap.get(), default_addr));
			break;
		}
		case OP_CASETBL: // ...
			// A variable number of case records follows this opcode, where
			// each record takes two cells.
			break;
		case OP_SWAP_PRI:
			// [STK] = PRI and PRI = [STK]
			as.xchg(AsmJit::dword_ptr(AsmJit::esp), AsmJit::eax);
			break;
		case OP_SWAP_ALT:
			// [STK] = ALT and ALT = [STK]
			as.xchg(AsmJit::dword_ptr(AsmJit::esp), AsmJit::ecx);
			break;
		case OP_PUSH_ADR: // offset
			// [STK] = FRM + offset, STK = STK - cell size
			as.lea(AsmJit::edx, AsmJit::dword_ptr(AsmJit::ebp, instr.getOperand() - reinterpret_cast<sysint_t>(vm_.getData())));
			as.push(AsmJit::edx);
			break;
		case OP_NOP:
			// no-operation, for code alignment
			break;
		case OP_BREAK:
			// conditional breakpoint
			break;
		case OP_PUSH_R:
		case OP_FILE:
		case OP_SYMBOL:
		case OP_LINE:
		case OP_SRANGE:
		case OP_SYMTAG:
		case OP_JREL:
			// obsolete
			throw ObsoleteInstructionError(instr);
		default:
			throw InvalidInstructionError(instr);
		}
	}

	code_ = as.make();
	codeSize_ = as.getCodeSize();

	codeMap_ = code_map.release();
	labelMap_ = labelMap.release();
}

void Jitter::halt(AsmJit::X86Assembler &as, cell error_code) {
	as.mov(AsmJit::dword_ptr_abs(reinterpret_cast<void*>(&vm_.getAmx()->error)), error_code);
	as.mov(AsmJit::esp, AsmJit::dword_ptr_abs(reinterpret_cast<void*>(&haltEsp_)));
	as.mov(AsmJit::ebp, AsmJit::dword_ptr_abs(reinterpret_cast<void*>(&haltEbp_)));
	as.ret();
}

void Jitter::beginExternalCode(AsmJit::X86Assembler &as) {
	as.lea(AsmJit::edx, AsmJit::dword_ptr(AsmJit::ebp, -reinterpret_cast<sysint_t>(vm_.getData())));
	as.mov(AsmJit::dword_ptr_abs(&vm_.getAmx()->frm), AsmJit::edx);
	as.mov(AsmJit::ebp, AsmJit::dword_ptr_abs(&ebp_));
	as.lea(AsmJit::edx, AsmJit::dword_ptr(AsmJit::esp, -reinterpret_cast<sysint_t>(vm_.getData())));
	as.mov(AsmJit::dword_ptr_abs(&vm_.getAmx()->stk), AsmJit::edx);
	as.mov(AsmJit::esp, AsmJit::dword_ptr_abs(&esp_));
}

void Jitter::endExternalCode(AsmJit::X86Assembler &as) {
	as.mov(AsmJit::dword_ptr_abs(&ebp_), AsmJit::ebp);
	as.mov(AsmJit::edx, AsmJit::dword_ptr_abs(&vm_.getAmx()->frm));
	as.lea(AsmJit::ebp, AsmJit::dword_ptr(AsmJit::edx, reinterpret_cast<sysint_t>(vm_.getData())));
	as.mov(AsmJit::dword_ptr_abs(&esp_), AsmJit::esp);
	as.mov(AsmJit::edx, AsmJit::dword_ptr_abs(&vm_.getAmx()->stk));
	as.lea(AsmJit::esp, AsmJit::dword_ptr(AsmJit::edx, reinterpret_cast<sysint_t>(vm_.getData())));
}

void Jitter::native_float(AsmJit::X86Assembler &as) {
	as.fild(AsmJit::dword_ptr(AsmJit::esp, 4));
	as.sub(AsmJit::esp, 4);
	as.fstp(AsmJit::dword_ptr(AsmJit::esp));
	as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::esp));
	as.add(AsmJit::esp, 4);
}

void Jitter::native_floatabs(AsmJit::X86Assembler &as) {
	as.fld(AsmJit::dword_ptr(AsmJit::esp, 4));
	as.fabs();
	as.sub(AsmJit::esp, 4);
	as.fstp(AsmJit::dword_ptr(AsmJit::esp));
	as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::esp));
	as.add(AsmJit::esp, 4);
}

void Jitter::native_floatadd(AsmJit::X86Assembler &as) {
	as.fld(AsmJit::dword_ptr(AsmJit::esp, 4));
	as.fadd(AsmJit::dword_ptr(AsmJit::esp, 8));
	as.sub(AsmJit::esp, 4);
	as.fstp(AsmJit::dword_ptr(AsmJit::esp));
	as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::esp));
	as.add(AsmJit::esp, 4);
}

void Jitter::native_floatsub(AsmJit::X86Assembler &as) {
	as.fld(AsmJit::dword_ptr(AsmJit::esp, 4));
	as.fsub(AsmJit::dword_ptr(AsmJit::esp, 8));
	as.sub(AsmJit::esp, 4);
	as.fstp(AsmJit::dword_ptr(AsmJit::esp));
	as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::esp));
	as.add(AsmJit::esp, 4);
}

void Jitter::native_floatmul(AsmJit::X86Assembler &as) {
	as.fld(AsmJit::dword_ptr(AsmJit::esp, 4));
	as.fmul(AsmJit::dword_ptr(AsmJit::esp, 8));
	as.sub(AsmJit::esp, 4);
	as.fstp(AsmJit::dword_ptr(AsmJit::esp));
	as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::esp));
	as.add(AsmJit::esp, 4);
}

void Jitter::native_floatdiv(AsmJit::X86Assembler &as) {
	as.fld(AsmJit::dword_ptr(AsmJit::esp, 4));
	as.fdiv(AsmJit::dword_ptr(AsmJit::esp, 8));
	as.sub(AsmJit::esp, 4);
	as.fstp(AsmJit::dword_ptr(AsmJit::esp));
	as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::esp));
	as.add(AsmJit::esp, 4);
}

void Jitter::native_floatsqroot(AsmJit::X86Assembler &as) {
	as.fld(AsmJit::dword_ptr(AsmJit::esp, 4));
	as.fsqrt();
	as.sub(AsmJit::esp, 4);
	as.fstp(AsmJit::dword_ptr(AsmJit::esp));
	as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::esp));
	as.add(AsmJit::esp, 4);
}

void Jitter::native_floatlog(AsmJit::X86Assembler &as) {
	as.fld1();
	as.fld(AsmJit::dword_ptr(AsmJit::esp, 8));
	as.fyl2x();
	as.fld1();
	as.fdivrp(AsmJit::st(1));
	as.fld(AsmJit::dword_ptr(AsmJit::esp, 4));
	as.fyl2x();
	as.sub(AsmJit::esp, 4);
	as.fstp(AsmJit::dword_ptr(AsmJit::esp));
	as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::esp));
	as.add(AsmJit::esp, 4);
}

AsmJit::Label &Jitter::L(AsmJit::X86Assembler &as, LabelMap *labelMap, cell address, const std::string &name) {
	LabelMap::iterator iterator = labelMap->find(TaggedAddress(address, name));
	if (iterator != labelMap->end()) {
		return iterator->second;
	} else {
		std::pair<LabelMap::iterator, bool> where =
				labelMap->insert(std::make_pair(TaggedAddress(address, name), as.newLabel()));
		return where.first->second;
	}
}

void JIT_STDCALL Jitter::doJump(Jitter *jitter, cell ip, void *stack) {
	CodeMap::const_iterator it = jitter->codeMap_->find(ip);

	typedef void (JIT_CDECL *DoJumpHelper)(void *dest, void *stack);
	static DoJumpHelper doJumpHelper = 0;

	if (it != jitter->codeMap_->end()) {
		void *dest = it->second + reinterpret_cast<char*>(jitter->code_);

		if (doJumpHelper == 0) {
			AsmJit::X86Assembler as;

			as.mov(AsmJit::esp, AsmJit::dword_ptr(AsmJit::esp, 8));
			as.jmp(AsmJit::dword_ptr(AsmJit::esp, 4));
			as.ret();

			doJumpHelper = (DoJumpHelper)as.make();
		}

		doJumpHelper(dest, stack);
	}
}

int Jitter::callFunction(cell address, cell *retval) {
	if (vm_.getAmx()->hea >= vm_.getAmx()->stk) {
		return AMX_ERR_STACKERR;
	}
	if (vm_.getAmx()->hea < vm_.getAmx()->hlw) {
		return AMX_ERR_HEAPLOW;
	}
	if (vm_.getAmx()->stk > vm_.getAmx()->stp) {
		return AMX_ERR_STACKLOW;
	}

	// Make sure all natives are registered.
	if ((vm_.getAmx()->flags & AMX_FLAG_NTVREG) == 0) {
		return AMX_ERR_NOTFOUND;
	}

	if (callFunctionHelper_ == 0) {
		AsmJit::X86Assembler as;
		
		// Copy function address to AsmJit::eax.
		as.mov(AsmJit::eax, AsmJit::dword_ptr(AsmJit::esp, 4));

		// The ESI, EDI and ECX registers can be modified by JIT code so they
		// must be saved somewhere e.g. on the stack. The EAX register is used
		// to store the return value of this function so it's not listed here.
		as.push(AsmJit::esi);
		as.push(AsmJit::edi);
		as.push(AsmJit::ecx);

		// Keep the two stack pointers in this Jitter object.
		as.mov(AsmJit::dword_ptr_abs(&ebp_), AsmJit::ebp);
		as.mov(AsmJit::dword_ptr_abs(&esp_), AsmJit::esp);

		// JIT code is executed on AMX stack, so switch to it.
		as.mov(AsmJit::ecx, AsmJit::dword_ptr_abs(&vm_.getAmx()->frm));
		as.lea(AsmJit::ebp, AsmJit::dword_ptr(AsmJit::ecx, reinterpret_cast<sysint_t>(vm_.getData())));		
		as.mov(AsmJit::ecx, AsmJit::dword_ptr_abs(&vm_.getAmx()->stk));
		as.lea(AsmJit::esp, AsmJit::dword_ptr(AsmJit::ecx, reinterpret_cast<sysint_t>(vm_.getData())));

		// haltEsp_ and haltEbp_ are needed for HALT and BOUNDS
		// instructions to quickly abort a currenly running function.
		as.mov(AsmJit::dword_ptr_abs(&haltEbp_), AsmJit::ebp);
		as.lea(AsmJit::ecx, AsmJit::dword_ptr(AsmJit::esp, - 4));
		as.mov(AsmJit::dword_ptr_abs(&haltEsp_), AsmJit::ecx);

		// Call the target function. The function address is stored in AsmJit::eax.
		as.call(AsmJit::eax);

		// Synchronize STK and FRM with ESP and EBP respectively.
		as.lea(AsmJit::ecx, AsmJit::dword_ptr(AsmJit::ebp, -reinterpret_cast<sysint_t>(vm_.getData())));
		as.mov(AsmJit::dword_ptr_abs(&vm_.getAmx()->frm), AsmJit::ecx);
		as.lea(AsmJit::ecx, AsmJit::dword_ptr(AsmJit::esp, -reinterpret_cast<sysint_t>(vm_.getData())));
		as.mov(AsmJit::dword_ptr_abs(&vm_.getAmx()->stk), AsmJit::ecx);

		// Switch back to the real stack.
		as.mov(AsmJit::ebp, AsmJit::dword_ptr_abs(&ebp_));
		as.mov(AsmJit::esp, AsmJit::dword_ptr_abs(&esp_));

		as.pop(AsmJit::ecx);
		as.pop(AsmJit::edi);
		as.pop(AsmJit::esi);

		as.ret();

		callFunctionHelper_ = (CallFunctionHelper)as.make();
	}

	vm_.getAmx()->error = AMX_ERR_NONE;

	void *start = getInstrPtr(address, getCode());
	assert(start != 0);

	void *haltEbp = haltEbp_;
	void *haltEsp = haltEsp_;

	cell retval_ = callFunctionHelper_(start);
	if (retval != 0) {
		*retval = retval_;
	}

	haltEbp_ = haltEbp;
	haltEsp_ = haltEsp;

	return vm_.getAmx()->error;
}

int Jitter::callPublicFunction(int index, cell *retval) {
	cell address = vm_.getPublicAddress(index);
	if (address == 0) {
		return AMX_ERR_INDEX;
	}
	CallContext ctx(vm_);
	return callFunction(address, retval);
}

} // namespace jit
