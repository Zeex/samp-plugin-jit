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
#include <AsmJit/AsmJit.h>

#include "jit.h"

namespace jit {

using namespace AsmJit;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AmxInstruction implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const char *AmxInstruction::opcodeNames[] = {
	"none",         "load.pri",     "load.alt",     "load.s.pri",
	"load.s.alt",   "lref.pri",     "lref.alt",     "lref.s.pri",
	"lref.s.alt",   "load.i",       "lodb.i",       "const.pri",
	"const.alt",    "addr.pri",     "addr.alt",     "stor.pri",
	"stor.alt",     "stor.s.pri",   "stor.s.alt",   "sref.pri",
	"sref.alt",     "sref.s.pri",   "sref.s.alt",   "stor.i",
	"strb.i",       "lidx",         "lidx.b",       "idxaddr",
	"idxaddr.b",    "align.pri",    "align.alt",    "lctrl",
	"sctrl",        "move.pri",     "move.alt",     "xchg",
	"push.pri",     "push.alt",     "push.r",       "push.c",
	"push",         "push.s",       "pop.pri",      "pop.alt",
	"stack",        "heap",         "proc",         "ret",
	"retn",         "call",         "call.pri",     "jump",
	"jrel",         "jzer",         "jnz",          "jeq",
	"jneq",         "jless",        "jleq",         "jgrtr",
	"jgeq",         "jsless",       "jsleq",        "jsgrtr",
	"jsgeq",        "shl",          "shr",          "sshr",
	"shl.c.pri",    "shl.c.alt",    "shr.c.pri",    "shr.c.alt",
	"smul",         "sdiv",         "sdiv.alt",     "umul",
	"udiv",         "udiv.alt",     "add",          "sub",
	"sub.alt",      "and",          "or",           "xor",
	"not",          "neg",          "invert",       "add.c",
	"smul.c",       "zero.pri",     "zero.alt",     "zero",
	"zero.s",       "sign.pri",     "sign.alt",     "eq",
	"neq",          "less",         "leq",          "grtr",
	"geq",          "sless",        "sleq",         "sgrtr",
	"sgeq",         "eq.c.pri",     "eq.c.alt",     "inc.pri",
	"inc.alt",      "inc",          "inc.s",        "inc.i",
	"dec.pri",      "dec.alt",      "dec",          "dec.s",
	"dec.i",        "movs",         "cmps",         "fill",
	"halt",         "bounds",       "sysreq.pri",   "sysreq.c",
	"file",         "line",         "symbol",       "srange",
	"jump.pri",     "switch",       "casetbl",      "swap.pri",
	"swap.alt",     "push.adr",     "nop",          "sysreq.d",
	"symtag",       "break"
};

AmxInstruction::AmxInstruction() 
	: address_(0), opcode_(OP_NONE), operands_() 
{
}

const char *AmxInstruction::getName() const {
	if (opcode_ >= 0 && opcode_ < NUM_AMX_OPCODES) {
		return opcodeNames[opcode_];
	}
	return 0;
}

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

bool AmxDisassembler::decode(AmxInstruction &instr, bool *error) {
	if (error != 0) {
		*error = false;
	}

	if (ip_ < 0 || vm_.getHeader()->cod + ip_ >= vm_.getHeader()->dat) {
		// Went out of the code section.
		return false;
	}

	instr.getOperands().clear();
	instr.setAddress(ip_);

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
	instr.setOpcode(static_cast<AmxOpcode>(opcode));

	switch (opcode) {
	// Instructions with one operand.
	case OP_LOAD_PRI:   case OP_LOAD_ALT:   case OP_LOAD_S_PRI: case OP_LOAD_S_ALT:
	case OP_LREF_PRI:   case OP_LREF_ALT:   case OP_LREF_S_PRI: case OP_LREF_S_ALT:
	case OP_LODB_I:     case OP_CONST_PRI:  case OP_CONST_ALT:  case OP_ADDR_PRI:
	case OP_ADDR_ALT:   case OP_STOR_PRI:   case OP_STOR_ALT:   case OP_STOR_S_PRI:
	case OP_STOR_S_ALT: case OP_SREF_PRI:   case OP_SREF_ALT:   case OP_SREF_S_PRI:
	case OP_SREF_S_ALT: case OP_STRB_I:     case OP_LIDX_B:     case OP_IDXADDR_B:
	case OP_ALIGN_PRI:  case OP_ALIGN_ALT:  case OP_LCTRL:      case OP_SCTRL:
	case OP_PUSH_R:     case OP_PUSH_C:     case OP_PUSH:       case OP_PUSH_S:
	case OP_STACK:      case OP_HEAP:       case OP_JREL:       case OP_JUMP:
	case OP_JZER:       case OP_JNZ:        case OP_JEQ:        case OP_JNEQ:
	case OP_JLESS:      case OP_JLEQ:       case OP_JGRTR:      case OP_JGEQ:
	case OP_JSLESS:     case OP_JSLEQ:      case OP_JSGRTR:     case OP_JSGEQ:
	case OP_SHL_C_PRI:  case OP_SHL_C_ALT:  case OP_SHR_C_PRI:  case OP_SHR_C_ALT:
	case OP_ADD_C:      case OP_SMUL_C:     case OP_ZERO:       case OP_ZERO_S:
	case OP_EQ_C_PRI:   case OP_EQ_C_ALT:   case OP_INC:        case OP_INC_S:
	case OP_DEC:        case OP_DEC_S:      case OP_MOVS:       case OP_CMPS:
	case OP_FILL:       case OP_HALT:       case OP_BOUNDS:     case OP_CALL:
	case OP_SYSREQ_C:   case OP_PUSH_ADR:   case OP_SYSREQ_D:   case OP_SWITCH:
		instr.addOperand(*reinterpret_cast<cell*>(vm_.getCode() + ip_));
		ip_ += sizeof(cell);
		break;

	// Instructions with no operands.
	case OP_LOAD_I:     case OP_STOR_I:     case OP_LIDX:       case OP_IDXADDR:
	case OP_MOVE_PRI:   case OP_MOVE_ALT:   case OP_XCHG:       case OP_PUSH_PRI:
	case OP_PUSH_ALT:   case OP_POP_PRI:    case OP_POP_ALT:    case OP_PROC:
	case OP_RET:        case OP_RETN:       case OP_CALL_PRI:   case OP_SHL:
	case OP_SHR:        case OP_SSHR:       case OP_SMUL:       case OP_SDIV:
	case OP_SDIV_ALT:   case OP_UMUL:       case OP_UDIV:       case OP_UDIV_ALT:
	case OP_ADD:        case OP_SUB:        case OP_SUB_ALT:    case OP_AND:
	case OP_OR:         case OP_XOR:        case OP_NOT:        case OP_NEG:
	case OP_INVERT:     case OP_ZERO_PRI:   case OP_ZERO_ALT:   case OP_SIGN_PRI:
	case OP_SIGN_ALT:   case OP_EQ:         case OP_NEQ:        case OP_LESS:
	case OP_LEQ:        case OP_GRTR:       case OP_GEQ:        case OP_SLESS:
	case OP_SLEQ:       case OP_SGRTR:      case OP_SGEQ:       case OP_INC_PRI:
	case OP_INC_ALT:    case OP_INC_I:      case OP_DEC_PRI:    case OP_DEC_ALT:
	case OP_DEC_I:      case OP_SYSREQ_PRI: case OP_JUMP_PRI:   case OP_SWAP_PRI:
	case OP_SWAP_ALT:   case OP_NOP:        case OP_BREAK:
		break;

	// Special instructions.
	case OP_CASETBL: {
		int num = *reinterpret_cast<cell*>(vm_.getCode() + ip_) + 1;
		// num case records follow, each is 2 cells big.
		for (int i = 0; i < num * 2; i++) {
			instr.addOperand(*reinterpret_cast<cell*>(vm_.getCode() + ip_));
			ip_ += sizeof(cell);
		}
		break;
	}

	default:
		if (error != 0) {
			*error = true;
		}
		return false;
	}

	return true;
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

Jitter::Jitter(AMX *amx, cell *opcodeTable)
	: vm_(amx)
	, opcodeTable_(opcodeTable)
	, code_(0)
	, codeSize_(0)
	, haltEsp_(0)
	, haltEbp_(0)
	, callHelper_(0)
{
}

Jitter::~Jitter() {
	MemoryManager *mem = MemoryManager::getGlobal();
	mem->free(code_);
	mem->free((void*)callHelper_);
}

void *Jitter::getInstrPtr(cell amx_ip, void *code_ptr) const {
	int native_ip = getInstrOffset(amx_ip);
	if (native_ip >= 0) {
		return reinterpret_cast<void*>(reinterpret_cast<int>(code_ptr) + native_ip);
	}
	return 0;
}

int Jitter::getInstrOffset(cell amx_ip) const {
	CodeMap::const_iterator iterator = codeMap_.find(amx_ip);
	if (iterator != codeMap_.end()) {
		return iterator->second;
	}
	return -1;
}

bool Jitter::compile(CompileErrorHandler errorHandler) {
	assert(code_ == 0 && "You can't compile() twice, create a new Jitter instead");

	AmxDisassembler disas(vm_);
	disas.setOpcodeTable(opcodeTable_);

	X86Assembler as;
	L_halt_ = as.newLabel();

	std::set<cell> jumpRefs;
	getJumpRefs(jumpRefs);

	bool error = false;
	AmxInstruction instr;

	while (disas.decode(instr, &error)) {
		cell cip = instr.getAddress();

		if (jumpRefs.find(cip) != jumpRefs.end()) {
			// This place is referenced by a JCC/JUMP/CALL/CASETBL instruction,
			// so put a label here.
			as.bind(L(as, cip));
		}

		codeMap_.insert(std::make_pair(cip, as.getCodeSize()));

		switch (instr.getOpcode()) {
		case OP_LOAD_PRI: // address
			// PRI = [address]
			as.mov(eax, dword_ptr(ebx, instr.getOperand()));
			break;
		case OP_LOAD_ALT: // address
			// ALT = [address]
			as.mov(ecx, dword_ptr(ebx, instr.getOperand()));
			break;
		case OP_LOAD_S_PRI: // offset
			// PRI = [FRM + offset]
			as.mov(eax, dword_ptr(ebp, instr.getOperand()));
			break;
		case OP_LOAD_S_ALT: // offset
			// ALT = [FRM + offset]
			as.mov(ecx, dword_ptr(ebp, instr.getOperand()));
			break;
		case OP_LREF_PRI: // address
			// PRI = [ [address] ]
			as.mov(edx, dword_ptr(ebx, instr.getOperand()));
			as.mov(eax, dword_ptr(ebx, edx));
			break;
		case OP_LREF_ALT: // address
			// ALT = [ [address] ]
			as.mov(edx, dword_ptr(ebx, + instr.getOperand()));
			as.mov(ecx, dword_ptr(ebx, edx));
			break;
		case OP_LREF_S_PRI: // offset
			// PRI = [ [FRM + offset] ]
			as.mov(edx, dword_ptr(ebp, instr.getOperand()));
			as.mov(eax, dword_ptr(ebx, edx));
			break;
		case OP_LREF_S_ALT: // offset
			// PRI = [ [FRM + offset] ]
			as.mov(edx, dword_ptr(ebp, instr.getOperand()));
			as.mov(ecx, dword_ptr(ebx, edx));
			break;
		case OP_LOAD_I:
			// PRI = [PRI] (full cell)
			as.mov(eax, dword_ptr(ebx, eax));
			break;
		case OP_LODB_I: // number
			// PRI = "number" bytes from [PRI] (read 1/2/4 bytes)
			switch (instr.getOperand()) {
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
			break;
		case OP_CONST_PRI: // value
			// PRI = value
			as.mov(eax, instr.getOperand());
			break;
		case OP_CONST_ALT: // value
			// ALT = value
			as.mov(ecx, instr.getOperand());
			break;
		case OP_ADDR_PRI: // offset
			// PRI = FRM + offset
			as.lea(eax, dword_ptr(ebp, instr.getOperand()));
			as.sub(eax, ebx);
			break;
		case OP_ADDR_ALT: // offset
			// ALT = FRM + offset
			as.lea(ecx, dword_ptr(ebp, instr.getOperand()));
			as.sub(ecx, ebx);
			break;
		case OP_STOR_PRI: // address
			// [address] = PRI
			as.mov(dword_ptr(ebx, instr.getOperand()), eax);
			break;
		case OP_STOR_ALT: // address
			// [address] = ALT
			as.mov(dword_ptr(ebx, instr.getOperand()), ecx);
			break;
		case OP_STOR_S_PRI: // offset
			// [FRM + offset] = ALT
			as.mov(dword_ptr(ebp, instr.getOperand()), eax);
			break;
		case OP_STOR_S_ALT: // offset
			// [FRM + offset] = ALT
			as.mov(dword_ptr(ebp, instr.getOperand()), ecx);
			break;
		case OP_SREF_PRI: // address
			// [ [address] ] = PRI
			as.mov(edx, dword_ptr(ebx, instr.getOperand()));
			as.mov(dword_ptr(ebx, edx), eax);
			break;
		case OP_SREF_ALT: // address
			// [ [address] ] = ALT
			as.mov(edx, dword_ptr(ebx, instr.getOperand()));
			as.mov(dword_ptr(ebx, edx), ecx);
			break;
		case OP_SREF_S_PRI: // offset
			// [ [FRM + offset] ] = PRI
			as.mov(edx, dword_ptr(ebp, instr.getOperand()));
			as.mov(dword_ptr(ebx, edx), eax);
			break;
		case OP_SREF_S_ALT: // offset
			// [ [FRM + offset] ] = ALT
			as.mov(edx, dword_ptr(ebp, instr.getOperand()));
			as.mov(dword_ptr(ebx, edx), ecx);
			break;
		case OP_STOR_I:
			// [ALT] = PRI (full cell)
			as.mov(dword_ptr(ebx, ecx), eax);
			break;
		case OP_STRB_I: // number
			// "number" bytes at [ALT] = PRI (write 1/2/4 bytes)
			switch (instr.getOperand()) {
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
			break;
		case OP_LIDX:
			// PRI = [ ALT + (PRI x cell size) ]
			as.lea(edx, dword_ptr(ebx, ecx));
			as.mov(eax, dword_ptr(edx, eax, 2));
			break;
		case OP_LIDX_B: // shift
			// PRI = [ ALT + (PRI << shift) ]
			as.lea(edx, dword_ptr(ebx, ecx));
			as.mov(eax, dword_ptr(edx, eax, instr.getOperand()));
			break;
		case OP_IDXADDR:
			// PRI = ALT + (PRI x cell size) (calculate indexed address)
			as.lea(eax, dword_ptr(ecx, eax, 2));
			break;
		case OP_IDXADDR_B: // shift
			// PRI = ALT + (PRI << shift) (calculate indexed address)
			as.lea(eax, dword_ptr(ecx, eax, instr.getOperand()));
			break;
		case OP_ALIGN_PRI: // number
			// Little Endian: PRI ^= cell size - number
			#if BYTE_ORDER == LITTLE_ENDIAN
				if (instr.getOperand() < sizeof(cell)) {
					as.xor_(eax, sizeof(cell) - instr.getOperand());
				}
			#endif
			break;
		case OP_ALIGN_ALT: // number
			// Little Endian: ALT ^= cell size - number
			#if BYTE_ORDER == LITTLE_ENDIAN
				if (instr.getOperand() < sizeof(cell)) {
					as.xor_(ecx, sizeof(cell) - instr.getOperand());
				}
			#endif
			break;
		case OP_LCTRL: // index
			// PRI is set to the current value of any of the special registers.
			// The index parameter must be: 0=COD, 1=DAT, 2=HEA,
			// 3=STP, 4=STK, 5=FRM, 6=CIP (of the next instruction)
			switch (instr.getOperand()) {
			case 0:
				as.mov(eax, dword_ptr_abs(reinterpret_cast<void*>(&vm_.getHeader()->cod)));
				break;
			case 1:
				as.mov(eax, dword_ptr_abs(reinterpret_cast<void*>(&vm_.getHeader()->dat)));
				break;
			case 2:
				as.mov(eax, dword_ptr_abs(reinterpret_cast<void*>(&vm_.getAmx()->hea)));
				break;
			case 4:
				as.mov(eax, esp);
				as.sub(eax, ebx);
				break;
			case 5:
				as.mov(eax, ebp);
				as.sub(eax, ebx);
				break;
			case 6: {
				as.mov(eax, instr.getAddress() + instr.getSize());
				break;
			}
			default:
				goto compile_error;
			}
			break;
		case OP_SCTRL: // index
			// set the indexed special registers to the value in PRI.
			// The index parameter must be: 2=HEA, 4=STK, 5=FRM,
			// 6=CIP
			switch (instr.getOperand()) {
			case 2:
				as.mov(dword_ptr_abs(reinterpret_cast<void*>(&vm_.getAmx()->hea)), eax);
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
				as.push(reinterpret_cast<int>(this));
				as.call(reinterpret_cast<void*>(Jitter::doJump));
				// Didn't jump because of invalid address - exit with error.
				halt(as, AMX_ERR_INVINSTR);
				break;
			default:
				goto compile_error;
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
			as.push(instr.getOperand());
			break;
		case OP_PUSH: // address
			// [STK] = [address], STK = STK - cell size
			as.push(dword_ptr(ebx, instr.getOperand()));
			break;
		case OP_PUSH_S: // offset
			// [STK] = [FRM + offset], STK = STK - cell size
			as.push(dword_ptr(ebp, instr.getOperand()));
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
			as.add(esp, instr.getOperand());
			break;
		case OP_HEAP: // value
			// ALT = HEA, HEA = HEA + value
			as.mov(ecx, dword_ptr_abs(reinterpret_cast<void*>(&vm_.getAmx()->hea)));
			as.add(dword_ptr_abs(reinterpret_cast<void*>(&vm_.getAmx()->hea)), instr.getOperand());
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
			as.call(L(as, instr.getOperand() - reinterpret_cast<cell>(vm_.getCode())));
			break;
		case OP_CALL_PRI:
			// Same as CALL, the address to jump to is in PRI.
			as.push(esp);
			as.push(eax);
			as.push(reinterpret_cast<int>(this));
			as.call(reinterpret_cast<void*>(Jitter::doCall));
			break;
		case OP_JUMP_PRI:
			// CIP = PRI (indirect jump)
			as.push(esp);
			as.push(eax);
			as.push(reinterpret_cast<int>(this));
			as.call(reinterpret_cast<void*>(Jitter::doJump));
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
			cell dest = instr.getOperand() - reinterpret_cast<cell>(vm_.getCode());
			Label &L_dest = L(as, dest);

			switch (instr.getOpcode()) {
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
			as.shl(eax, static_cast<unsigned char>(instr.getOperand()));
			break;
		case OP_SHL_C_ALT: // value
			// ALT = ALT << value
			as.shl(ecx, static_cast<unsigned char>(instr.getOperand()));
			break;
		case OP_SHR_C_PRI: // value
			// PRI = PRI >> value (without sign extension)
			as.shr(eax, static_cast<unsigned char>(instr.getOperand()));
			break;
		case OP_SHR_C_ALT: // value
			// PRI = PRI >> value (without sign extension)
			as.shl(ecx, static_cast<unsigned char>(instr.getOperand()));
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
			as.add(eax, instr.getOperand());
			break;
		case OP_SMUL_C: // value
			// PRI = PRI * value
			as.imul(eax, instr.getOperand());
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
			as.mov(dword_ptr(ebx, instr.getOperand()), 0);
			break;
		case OP_ZERO_S: // offset
			// [FRM + offset] = 0
			as.mov(dword_ptr(ebp, instr.getOperand()), 0);
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
			as.cmp(eax, instr.getOperand());
			as.sete(al);
			as.movzx(eax, al);
			break;
		case OP_EQ_C_ALT: // value
			// PRI = ALT == value ? 1 : 0
			as.cmp(ecx, instr.getOperand());
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
			as.inc(dword_ptr(ebx, instr.getOperand()));
			break;
		case OP_INC_S: // offset
			// [FRM + offset] = [FRM + offset] + 1
			as.inc(dword_ptr(ebp, instr.getOperand()));
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
			as.dec(dword_ptr(ebx, instr.getOperand()));
			break;
		case OP_DEC_S: // offset
			// [FRM + offset] = [FRM + offset] - 1
			as.dec(dword_ptr(ebp, instr.getOperand()));
			break;
		case OP_DEC_I:
			// [PRI] = [PRI] - 1
			as.dec(dword_ptr(ebx, eax));
			break;
		case OP_MOVS: // number
			// Copy memory from [PRI] to [ALT]. The parameter
			// specifies the number of bytes. The blocks should not
			// overlap.
			as.lea(esi, dword_ptr(ebx, eax));
			as.lea(edi, dword_ptr(ebx, ecx));
			as.push(ecx);
			if (instr.getOperand() % 4 == 0) {
				as.mov(ecx, instr.getOperand() / 4);
				as.rep_movsd();
			} else if (instr.getOperand() % 2 == 0) {
				as.mov(ecx, instr.getOperand() / 2);
				as.rep_movsw();
			} else {
				as.mov(ecx, instr.getOperand());
				as.rep_movsb();
			}
			as.pop(ecx);
			break;
		case OP_CMPS: { // number
			// Compare memory blocks at [PRI] and [ALT]. The parameter
			// specifies the number of bytes. The blocks should not
			// overlap.
			Label L_above(L(as, cip, "above"));
			Label L_below(L(as, cip, "below"));
			Label L_equal(L(as, cip, "equal"));
			Label L_continue(L(as, cip, "continue"));
				as.cld();
				as.lea(edi, dword_ptr(ebx, eax));
				as.lea(esi, dword_ptr(ebx, ecx));
				as.push(ecx);
				as.mov(ecx, instr.getOperand());
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
		case OP_FILL: { // number
			// Fill memory at [ALT] with value in [PRI]. The parameter
			// specifies the number of bytes, which must be a multiple
			// of the cell size.
			as.lea(edi, dword_ptr(ebx, ecx));
			as.push(ecx);
			as.mov(ecx, instr.getOperand() / sizeof(cell));
			as.rep_stosd();
			as.pop(ecx);
			break;
		}
		case OP_HALT: // number
			// Abort execution (exit value in PRI), parameters other than 0
			// have a special meaning.
			halt(as, instr.getOperand());
			break;
		case OP_BOUNDS: { // value
			// Abort execution if PRI > value or if PRI < 0.
			Label &L_halt = L(as, cip, "halt");
			Label &L_good = L(as, cip, "good");
				as.cmp(eax, instr.getOperand());
			as.jg(L_halt);
				as.cmp(eax, 0);
				as.jl(L_halt);
				as.jmp(L_good);
			as.bind(L_halt);
				halt(as, AMX_ERR_BOUNDS);
			as.bind(L_good);
			break;
		}
		case OP_SYSREQ_PRI: {
			// call system service, service number in PRI
			as.mov(edi, esp);
			endJitCode(as);
				as.push(edi);
				as.push(eax);
				as.push(reinterpret_cast<int>(this));
				as.call(reinterpret_cast<void*>(Jitter::doSysreq));
			beginJitCode(as);
			break;
		}
		case OP_SYSREQ_C:   // index
		case OP_SYSREQ_D: { // address
			// call system service
			const char *nativeName = 0;
			switch (instr.getOpcode()) {
				case OP_SYSREQ_C:
					nativeName = vm_.getNativeName(instr.getOperand());
					break;
				case OP_SYSREQ_D: {
					nativeName = vm_.getNativeName(vm_.getNativeIndex(instr.getOperand()));
					break;
				}
			}
			if (nativeName == 0) {
				goto invalid_native;
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
			as.mov(edi, esp);
			endJitCode(as);
				as.push(edi);
				as.push(reinterpret_cast<int>(vm_.getAmx()));
				switch (instr.getOpcode()) {
					case OP_SYSREQ_C: {
						cell address = vm_.getNativeAddress(instr.getOperand());
						if (address == 0) {
							goto invalid_native;
						}
						as.call(reinterpret_cast<void*>(address));
						break;
					}
					case OP_SYSREQ_D:
						as.call(reinterpret_cast<void*>(instr.getOperand()));
						break;
				}
				as.add(esp, 8);
			beginJitCode(as);
			break;

		special_native:
			break;

		invalid_native:
			goto compile_error;
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

				// Check if the value in eax is in the allowed range.
				// If not, jump to the default case (i.e. no match).
				as.cmp(eax, *min_value);
				as.jl(L(as, default_addr));
				as.cmp(eax, *max_value);
				as.jg(L(as, default_addr));

				// OK now sequentially compare eax with each value.
				// This is pretty slow so I probably should optimize
				// this in future...
				for (int i = 0; i < num_cases; i++) {
					as.cmp(eax, case_table[i + 1].value);
					as.je(L(as, case_table[i + 1].address - reinterpret_cast<cell>(vm_.getCode())));
				}
			}

			// No match found - go for default case.
			as.jmp(L(as, default_addr));
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
			as.lea(edx, dword_ptr(ebp, instr.getOperand()));
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
			goto compile_error;
		}
	}

	// Halt implementation follows the code. To issue a HALT one does the following:
	// 1. puts the error code in to ECX
	// 2. and jumps to this point
	as.bind(L_halt_);
		as.push(ecx);
		as.push(reinterpret_cast<int>(this));
		as.call(reinterpret_cast<void*>(Jitter::doHalt));

	if (error) {
		goto compile_error;
	}

	code_     = as.make();
	codeSize_ = as.getCodeSize();
	return true;

compile_error:
	if (errorHandler != 0) {
		errorHandler(vm_, instr);
	}
	return false;
}

int Jitter::call(cell address, cell *retval) {
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

	if (callHelper_ == 0) {
		X86Assembler as;
		
		as.mov(eax, dword_ptr(esp, 4));
		as.pushad();
		as.mov(ebx, reinterpret_cast<int>(vm_.getData()));
		beginJitCode(as);
			as.lea(ecx, dword_ptr(esp, - 4));
			as.mov(dword_ptr_abs(&haltEsp_), ecx);
			as.mov(dword_ptr_abs(&haltEbp_), ebp);
			as.call(eax);
		endJitCode(as);
		as.popad();
		as.ret();

		callHelper_ = (CallHelper)as.make();
	}

	vm_.getAmx()->error = AMX_ERR_NONE;

	void *start = getInstrPtr(address, getCode());
	assert(start != 0);

	void *haltEbp = haltEbp_;
	void *haltEsp = haltEsp_;

	cell retval_ = callHelper_(start);
	if (retval != 0) {
		*retval = retval_;
	}

	haltEbp_ = haltEbp;
	haltEsp_ = haltEsp;

	return vm_.getAmx()->error;
}

int Jitter::exec(int index, cell *retval) {
	cell address = vm_.getPublicAddress(index);
	if (address == 0) {
		return AMX_ERR_INDEX;
	}

	// Push size of arguments and reset parameter count.
	vm_.pushStack(vm_.getAmx()->paramcount * sizeof(cell));
	vm_.getAmx()->paramcount = 0;

	return call(address, retval);
}

void Jitter::getJumpRefs(std::set<cell> &refs) const {
	AmxDisassembler disas(vm_);
	AmxInstruction instr;

	while (disas.decode(instr)) {
		AmxOpcode opcode = instr.getOpcode();
		switch (opcode) {
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
				refs.insert(instr.getOperand() - reinterpret_cast<int>(vm_.getCode()));
				break;
			case OP_CASETBL:
				int n = instr.getNumOperands();
				for (int i = 1; i < n; i += 2) {
					refs.insert(instr.getOperand(i) - reinterpret_cast<int>(vm_.getCode()));
				}
				break;
		}
	}
}

Label &Jitter::L(X86Assembler &as, cell address) {
	return L(as, address, std::string());
}

Label &Jitter::L(X86Assembler &as, cell address, const std::string &name) {
	LabelMap::iterator iterator = labelMap_.find(TaggedAddress(address, name));
	if (iterator != labelMap_.end()) {
		return iterator->second;
	} else {
		std::pair<LabelMap::iterator, bool> where =
				labelMap_.insert(std::make_pair(TaggedAddress(address, name), as.newLabel()));
		return where.first->second;
	}
}

void JIT_STDCALL Jitter::doJump(Jitter *jitter, cell address, void *stack) {
	CodeMap::const_iterator it = jitter->codeMap_.find(address);

	typedef void (JIT_CDECL *DoJumpHelper)(void *dest, void *stack);
	static DoJumpHelper doJumpHelper = 0;

	if (it != jitter->codeMap_.end()) {
		void *dest = it->second + reinterpret_cast<char*>(jitter->code_);

		if (doJumpHelper == 0) {
			X86Assembler as;
			as.mov(eax, dword_ptr(esp, 4));
			as.mov(esp, dword_ptr(esp, 8));
			as.jmp(eax);
			doJumpHelper = (DoJumpHelper)as.make();
		}

		doJumpHelper(dest, stack);
	}

	doHalt(jitter, AMX_ERR_INVINSTR);
}

cell JIT_STDCALL Jitter::doCall(Jitter *jitter, cell address, void *stack) {
	CodeMap::const_iterator it = jitter->codeMap_.find(address);

	typedef cell (JIT_CDECL *DoCallHelper)(void *func, void *stack);
	static DoCallHelper doCallHelper = 0;

	if (it != jitter->codeMap_.end()) {
		void *func = it->second + reinterpret_cast<char*>(jitter->code_);

		if (doCallHelper == 0) {
			X86Assembler as;
			as.mov(eax, dword_ptr(esp, 4));
			as.mov(esp, dword_ptr(esp, 8));
			as.call(eax);
			as.ret();
			doCallHelper = (DoCallHelper)as.make();
		}

		return doCallHelper(func, stack);
	}

	doHalt(jitter, AMX_ERR_INVINSTR);
	return 0; // make compiler happy
}

cell JIT_STDCALL Jitter::doSysreq(Jitter *jitter, int index, cell *params) {
	AMX_NATIVE native = reinterpret_cast<AMX_NATIVE>(jitter->vm_.getNativeAddress(index));
	if (native != 0) {
		return native(jitter->vm_.getAmx(), params);
	}
	doHalt(jitter, AMX_ERR_NOTFOUND);
	return 0; // make compiler happy
}

void JIT_STDCALL Jitter::doHalt(Jitter *jitter, int errorCode) {
	typedef void (JIT_CDECL *DoHaltHelper)(int errorCode);
	static DoHaltHelper doHaltHelper = 0;

	if (doHaltHelper == 0) {
		X86Assembler as;
		as.mov(eax, dword_ptr(esp, 4));
		as.mov(dword_ptr_abs(reinterpret_cast<void*>(&jitter->vm_.getAmx()->error)), eax);
		as.mov(esp, dword_ptr_abs(reinterpret_cast<void*>(&jitter->haltEsp_)));
		as.mov(ebp, dword_ptr_abs(reinterpret_cast<void*>(&jitter->haltEbp_)));
		as.ret();
		doHaltHelper = (DoHaltHelper)as.make();
	}

	doHaltHelper(errorCode);
}

void Jitter::native_float(X86Assembler &as) {
	as.fild(dword_ptr(esp, 4));
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatabs(X86Assembler &as) {
	as.fld(dword_ptr(esp, 4));
	as.fabs();
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatadd(X86Assembler &as) {
	as.fld(dword_ptr(esp, 4));
	as.fadd(dword_ptr(esp, 8));
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatsub(X86Assembler &as) {
	as.fld(dword_ptr(esp, 4));
	as.fsub(dword_ptr(esp, 8));
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatmul(X86Assembler &as) {
	as.fld(dword_ptr(esp, 4));
	as.fmul(dword_ptr(esp, 8));
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatdiv(X86Assembler &as) {
	as.fld(dword_ptr(esp, 4));
	as.fdiv(dword_ptr(esp, 8));
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatsqroot(X86Assembler &as) {
	as.fld(dword_ptr(esp, 4));
	as.fsqrt();
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatlog(X86Assembler &as) {
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

void Jitter::halt(X86Assembler &as, cell errorCode) {
	as.mov(ecx, errorCode);
	as.jmp(L_halt_);
}

void Jitter::endJitCode(X86Assembler &as) {
	as.mov(edx, ebp);
	as.sub(edx, ebx);
	as.mov(dword_ptr_abs(&vm_.getAmx()->frm), edx);
	as.mov(ebp, dword_ptr_abs(&ebp_));
	as.mov(edx, esp);
	as.sub(edx, ebx);
	as.mov(dword_ptr_abs(&vm_.getAmx()->stk), edx);
	as.mov(esp, dword_ptr_abs(&esp_));
}

void Jitter::beginJitCode(X86Assembler &as) {
	as.mov(dword_ptr_abs(&ebp_), ebp);
	as.mov(edx, dword_ptr_abs(&vm_.getAmx()->frm));
	as.lea(ebp, dword_ptr(ebx, edx));
	as.mov(dword_ptr_abs(&esp_), esp);
	as.mov(edx, dword_ptr_abs(&vm_.getAmx()->stk));
	as.lea(esp, dword_ptr(ebx, edx));
}

} // namespace jit
