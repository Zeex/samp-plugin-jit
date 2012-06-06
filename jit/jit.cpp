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
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <cassert>
#include <cstddef>
#include <cstring>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include <amx/amx.h>
#include <AsmJit/AsmJit.h>

#include "jit.h"

#if defined __GNUC__
	#define likely(x)       __builtin_expect((x),1)
	#define unlikely(x)     __builtin_expect((x),0)
#else
	#define likely(x)         (x)
	#define unlikely(x)       (x)
#endif

namespace jit {

using namespace AsmJit;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AMXInstruction implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const AMXInstruction::StaticInfoTableEntry AMXInstruction::info[NUM_AMX_OPCODES] = {
	{"none",           REG_NONE,             REG_NONE},
	{"load.pri",       REG_NONE,             REG_PRI},
	{"load.alt",       REG_NONE,             REG_ALT},
	{"load.s.pri",     REG_FRM,              REG_PRI},
	{"load.s.alt",     REG_FRM,              REG_ALT},
	{"lref.pri",       REG_NONE,             REG_PRI},
	{"lref.alt",       REG_NONE,             REG_ALT},
	{"lref.s.pri",     REG_FRM,              REG_PRI},
	{"lref.s.alt",     REG_FRM,              REG_ALT},
	{"load.i",         REG_PRI,              REG_PRI},
	{"lodb.i",         REG_PRI,              REG_PRI},
	{"const.pri",      REG_NONE,             REG_PRI},
	{"const.alt",      REG_NONE,             REG_ALT},
	{"addr.pri",       REG_FRM,              REG_PRI},
	{"addr.alt",       REG_FRM,              REG_ALT},
	{"stor.pri",       REG_PRI,              REG_NONE},
	{"stor.alt",       REG_ALT,              REG_NONE},
	{"stor.s.pri",     REG_FRM | REG_PRI,    REG_NONE},
	{"stor.s.alt",     REG_FRM | REG_ALT,    REG_NONE},
	{"sref.pri",       REG_PRI,              REG_NONE},
	{"sref.alt",       REG_ALT,              REG_NONE},
	{"sref.s.pri",     REG_FRM | REG_PRI,    REG_NONE},
	{"sref.s.alt",     REG_FRM | REG_ALT,    REG_NONE},
	{"stor.i",         REG_PRI | REG_ALT,    REG_NONE},
	{"strb.i",         REG_PRI | REG_ALT,    REG_NONE},
	{"lidx",           REG_PRI | REG_ALT,    REG_PRI},
	{"lidx.b",         REG_PRI | REG_ALT,    REG_PRI},
	{"idxaddr",        REG_PRI | REG_ALT,    REG_PRI},
	{"idxaddr.b",      REG_PRI | REG_ALT,    REG_PRI},
	{"align.pri",      REG_NONE, REG_PRI},
	{"align.alt",      REG_NONE, REG_ALT},
	{"lctrl",          REG_PRI | REG_COD |
	                   REG_DAT | REG_HEA |
	                   REG_STP | REG_STK |
	                   REG_FRM | REG_CIP,    REG_PRI},
	{"sctrl",          REG_PRI,              REG_HEA | REG_STK |
	                                         REG_FRM | REG_CIP},
	{"move.pri",       REG_ALT,              REG_PRI},
	{"move.alt",       REG_PRI,              REG_ALT},
	{"xchg",           REG_PRI | REG_ALT,    REG_PRI | REG_ALT},
	{"push.pri",       REG_PRI | REG_STK,    REG_STK},
	{"push.alt",       REG_ALT | REG_STK,    REG_STK},
	{"push.r",         REG_NONE,             REG_NONE},
	{"push.c",         REG_STK,              REG_STK},
	{"push",           REG_STK,              REG_STK},
	{"push.s",         REG_STK | REG_FRM,    REG_STK},
	{"pop.pri",        REG_STK,              REG_PRI | REG_STK},
	{"pop.alt",        REG_STK,              REG_ALT | REG_STK},
	{"stack",          REG_STK,              REG_ALT | REG_STK},
	{"heap",           REG_HEA,              REG_ALT | REG_HEA},
	{"proc",           REG_STK | REG_FRM,    REG_STK | REG_FRM},
	{"ret",            REG_STK,              REG_STK | REG_FRM |
	                                         REG_CIP},
	{"retn",           REG_STK,              REG_STK | REG_FRM |
	                                         REG_CIP},
	{"call",           REG_STK | REG_CIP,    REG_PRI | REG_ALT |
	                                         REG_STK | REG_CIP},
	{"call.pri",       REG_PRI | REG_STK |
	                   REG_CIP,              REG_STK | REG_CIP},
	{"jump",           REG_NONE,             REG_CIP},
	{"jrel",           REG_NONE,             REG_NONE},
	{"jzer",           REG_PRI,              REG_CIP},
	{"jnz",            REG_PRI,              REG_CIP},
	{"jeq",            REG_PRI | REG_ALT,    REG_CIP},
	{"jneq",           REG_PRI | REG_ALT,    REG_CIP},
	{"jless",          REG_PRI | REG_ALT,    REG_CIP},
	{"jleq",           REG_PRI | REG_ALT,    REG_CIP},
	{"jgrtr",          REG_PRI | REG_ALT,    REG_CIP},
	{"jgeq",           REG_PRI | REG_ALT,    REG_CIP},
	{"jsless",         REG_PRI | REG_ALT,    REG_CIP},
	{"jsleq",          REG_PRI | REG_ALT,    REG_CIP},
	{"jsgrtr",         REG_PRI | REG_ALT,    REG_CIP},
	{"jsgeq",          REG_PRI | REG_ALT,    REG_CIP},
	{"shl",            REG_PRI | REG_ALT,    REG_PRI},
	{"shr",            REG_PRI | REG_ALT,    REG_PRI},
	{"sshr",           REG_PRI | REG_ALT,    REG_PRI},
	{"shl.c.pri",      REG_PRI,              REG_PRI},
	{"shl.c.alt",      REG_ALT,              REG_ALT},
	{"shr.c.pri",      REG_PRI,              REG_PRI},
	{"shr.c.alt",      REG_ALT,              REG_ALT},
	{"smul",           REG_PRI | REG_ALT,    REG_PRI},
	{"sdiv",           REG_PRI | REG_ALT,    REG_PRI},
	{"sdiv.alt",       REG_PRI | REG_ALT,    REG_PRI},
	{"umul",           REG_PRI | REG_ALT,    REG_PRI},
	{"udiv",           REG_PRI | REG_ALT,    REG_PRI},
	{"udiv.alt",       REG_PRI | REG_ALT,    REG_PRI},
	{"add",            REG_PRI | REG_ALT,    REG_PRI},
	{"sub",            REG_PRI | REG_ALT,    REG_PRI},
	{"sub.alt",        REG_PRI | REG_ALT,    REG_PRI},
	{"and",            REG_PRI | REG_ALT,    REG_PRI},
	{"or",             REG_PRI | REG_ALT,    REG_PRI},
	{"xor",            REG_PRI | REG_ALT,    REG_PRI},
	{"not",            REG_PRI,              REG_PRI},
	{"neg",            REG_PRI,              REG_PRI},
	{"invert",         REG_PRI,              REG_PRI},
	{"add.c",          REG_PRI,              REG_PRI},
	{"smul.c",         REG_PRI,              REG_PRI},
	{"zero.pri",       REG_NONE,             REG_PRI},
	{"zero.alt",       REG_NONE,             REG_PRI},
	{"zero",           REG_NONE,             REG_NONE},
	{"zero.s",         REG_FRM,              REG_NONE},
	{"sign.pri",       REG_PRI,              REG_PRI},
	{"sign.alt",       REG_ALT,              REG_ALT},
	{"eq",             REG_PRI | REG_ALT,    REG_PRI},
	{"neq",            REG_PRI | REG_ALT,    REG_PRI},
	{"less",           REG_PRI | REG_ALT,    REG_PRI},
	{"leq",            REG_PRI | REG_ALT,    REG_PRI},
	{"grtr",           REG_PRI | REG_ALT,    REG_PRI},
	{"geq",            REG_PRI | REG_ALT,    REG_PRI},
	{"sless",          REG_PRI | REG_ALT,    REG_PRI},
	{"sleq",           REG_PRI | REG_ALT,    REG_PRI},
	{"sgrtr",          REG_PRI | REG_ALT,    REG_PRI},
	{"sgeq",           REG_PRI | REG_ALT,    REG_PRI},
	{"eq.c.pri",       REG_PRI,              REG_PRI},
	{"eq.c.alt",       REG_ALT,              REG_PRI},
	{"inc.pri",        REG_PRI,              REG_PRI},
	{"inc.alt",        REG_ALT,              REG_ALT},
	{"inc",            REG_NONE,             REG_NONE},
	{"inc.s",          REG_FRM,              REG_NONE},
	{"inc.i",          REG_PRI,              REG_NONE},
	{"dec.pri",        REG_PRI,              REG_PRI},
	{"dec.alt",        REG_ALT,              REG_ALT},
	{"dec",            REG_NONE,             REG_NONE},
	{"dec.s",          REG_FRM,              REG_NONE},
	{"dec.i",          REG_PRI,              REG_NONE},
	{"movs",           REG_PRI | REG_ALT,    REG_NONE},
	{"cmps",           REG_PRI | REG_ALT,    REG_NONE},
	{"fill",           REG_PRI | REG_ALT,    REG_NONE},
	{"halt",           REG_PRI,              REG_NONE},
	{"bounds",         REG_PRI,              REG_NONE},
	{"sysreq.pri",     REG_PRI,              REG_PRI | REG_ALT |
	                                         REG_COD | REG_DAT |
	                                         REG_HEA | REG_STP |
	                                         REG_STK | REG_FRM |
	                                         REG_CIP},
	{"sysreq.c",       REG_NONE,             REG_PRI | REG_ALT |
	                                         REG_COD | REG_DAT |
	                                         REG_HEA | REG_STP |
	                                         REG_STK | REG_FRM |
	                                         REG_CIP},
	{"file",           REG_NONE,             REG_NONE},
	{"line",           REG_NONE,             REG_NONE},
	{"symbol",         REG_NONE,             REG_NONE},
	{"srange",         REG_NONE,             REG_NONE},
	{"jump.pri",       REG_PRI,              REG_CIP},
	{"switch",         REG_PRI,              REG_CIP},
	{"casetbl",        REG_NONE,             REG_NONE},
	{"swap.pri",       REG_PRI | REG_STK,    REG_PRI},
	{"swap.alt",       REG_ALT | REG_STK,    REG_ALT},
	{"push.adr",       REG_STK | REG_FRM,    REG_STK},
	{"nop",            REG_NONE,             REG_NONE},
	{"sysreq.d",       REG_NONE,             REG_PRI | REG_ALT |
	                                         REG_COD | REG_DAT |
	                                         REG_HEA | REG_STP |
	                                         REG_STK | REG_FRM |
	                                         REG_CIP},
	{"symtag",         REG_NONE,             REG_NONE},
	{"break",          REG_NONE,             REG_NONE}
};

AMXInstruction::AMXInstruction() 
	: address_(0), opcode_(OP_NONE), operands_() 
{
}

const char *AMXInstruction::name() const {
	if (opcode_ >= 0 && opcode_ < NUM_AMX_OPCODES) {
		return info[opcode_].name;
	}
	return 0;
}

int AMXInstruction::inputRegisters() const {
	if (opcode_ >= 0 && opcode_ < NUM_AMX_OPCODES) {
		return info[opcode_].inputRegisters;
	}
	return 0;
}

int AMXInstruction::outputRegisters() const {
	if (opcode_ >= 0 && opcode_ < NUM_AMX_OPCODES) {
		return info[opcode_].outputRegisters;
	}
	return 0;
}

std::string AMXInstruction::string() const {
	std::stringstream stream;
	if (name() != 0) {
		stream << name();
	} else {
		stream << std::setw(8) << std::setfill('0') << std::hex << opcode_;
	}
	for (std::vector<cell>::const_iterator it = operands_.begin(); it != operands_.end(); ++it) {
		stream << ' ' << std::setw(8) << std::setfill('0') << std::hex << *it;
	}
	return stream.str();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AMXScript implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AMXScript::AMXScript(AMX *amx)
	: amx_(amx)
{
}

cell AMXScript::getPublicAddress(cell index) const {
	if (index == AMX_EXEC_MAIN) {
		return amxHeader()->cip;
	}
	if (index < 0 || index >= numPublics()) {
		return 0;
	}
	return publics()[index].address;
}

cell AMXScript::getNativeAddress(int index) const {
	if (index < 0 || index >= numNatives()) {
		return 0;
	}
	return natives()[index].address;
}

int AMXScript::getPublicIndex(cell address) const {
	for (int i = 0; i < numPublics(); i++) {
		if (publics()[i].address == address) {
			return i;
		}
	}
	return -1;
}

int AMXScript::getNativeIndex(cell address) const {
	for (int i = 0; i < numNatives(); i++) {
		if (natives()[i].address == address) {
			return i;
		}
	}
	return -1;
}

const char *AMXScript::getPublicName(int index) const {
	if (index < 0 || index >= numPublics()) {
		return 0;
	}
	return reinterpret_cast<char*>(amx_->base + publics()[index].nameofs);
}

const char *AMXScript::getNativeName(int index) const {
	if (index < 0 || index >= numNatives()) {
		return 0;
	}
	return reinterpret_cast<char*>(amx_->base + natives()[index].nameofs);
}

cell *AMXScript::push(cell value) {
	amx_->stk -= sizeof(cell);
	cell *s = stack();
	*s = value;
	return s;
}

cell AMXScript::pop() {
	cell *s = stack();
	amx_->stk += sizeof(cell);
	return *s;
}

void AMXScript::pop(int ncells) {
	amx_->stk += ncells * sizeof(cell);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AMXDisassembler implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AMXDisassembler::AMXDisassembler(const AMXScript &amx)
	: amx_(amx)
	, opcodeTable_(0)
	, ip_(0)
{
}

bool AMXDisassembler::decode(AMXInstruction &instr, bool *error) {
	if (error != 0) {
		*error = false;
	}

	if (ip_ < 0 || amx_.amxHeader()->cod + ip_ >= amx_.amxHeader()->dat) {
		// Went out of the code section.
		return false;
	}

	instr.operands().clear();
	instr.setAddress(ip_);

	cell opcode = *reinterpret_cast<cell*>(amx_.code() + ip_);
	if (opcodeTable_ != 0) {
		// Lookup this opcode in the opcode relocation table.
		bool found = false;
		for (int i = 0; i < NUM_AMX_OPCODES; i++) {
			if (opcodeTable_[i] == opcode) {
				opcode = i;
				break;
			}
		}
	}

	ip_ += sizeof(cell);
	instr.setOpcode(static_cast<AMXOpcode>(opcode));

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
		instr.addOperand(*reinterpret_cast<cell*>(amx_.code() + ip_));
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
		int num = *reinterpret_cast<cell*>(amx_.code() + ip_) + 1;
		// num case records follow, each is 2 cells big.
		for (int i = 0; i < num * 2; i++) {
			instr.addOperand(*reinterpret_cast<cell*>(amx_.code() + ip_));
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
// JIT implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// JIT compiler intrinsics.
JIT::Intrinsic JIT::intrinsics_[] = {
	{"float",       &JIT::native_float},
	{"float",       &JIT::native_float},
	{"floatabs",    &JIT::native_floatabs},
	{"floatadd",    &JIT::native_floatadd},
	{"floatsub",    &JIT::native_floatsub},
	{"floatmul",    &JIT::native_floatmul},
	{"floatdiv",    &JIT::native_floatdiv},
	{"floatsqroot", &JIT::native_floatsqroot},
	{"floatlog",    &JIT::native_floatlog}
};

JIT::JIT(AMXScript amx)
	: amx_(amx)
	, opcodeTable_(0)
	, assembler_(0)
	, code_(0)
	, codeSize_(0)
	, haltEsp_(0)
	, haltEbp_(0)
	, haltHelper_(0)
	, jumpHelper_(0)
	, callHelper_(0)
	, sysreqHelper_(0)
{
}

JIT::~JIT() {
	MemoryManager *mem = MemoryManager::getGlobal();
	mem->free(code_);
	mem->free((void*)haltHelper_);
	mem->free((void*)jumpHelper_);
	mem->free((void*)callHelper_);
	mem->free((void*)sysreqHelper_);
}

void *JIT::getInstrPtr(cell address) const {
	int native_ip = getInstrOffset(address);
	if (native_ip >= 0) {
		return reinterpret_cast<void*>(reinterpret_cast<int>(code_) + native_ip);
	}
	return 0;
}

int JIT::getInstrOffset(cell address) const {
	CodeMap::const_iterator iterator = codeMap_.find(address);
	if (iterator != codeMap_.end()) {
		return iterator->second;
	}
	return -1;
}

bool JIT::compile(CompileErrorHandler errorHandler) {
	assert(code_ == 0 && "You can't compile() twice, create a new JIT instead");

	AMXDisassembler disas(amx_);
	disas.setOpcodeTable(opcodeTable_);

	X86Assembler *as;
	if (assembler_ == 0) {
		as = new X86Assembler;
	} else {
		as = assembler_;
	}

	L_halt_ = as->newLabel();

	std::set<cell> jumpRefs;
	getJumpRefs(jumpRefs);

	Logger *logger = as->getLogger();

	bool error = false;
	AMXInstruction instr;

	while (disas.decode(instr, &error)) {
		cell cip = instr.address();

		if (logger != 0) {
			if (instr.opcode() == OP_PROC) {
				logger->logString("\n\n\n");
			}
			logger->logFormat("\t; %08x %s\n", cip, instr.string().c_str());
		}

		if (jumpRefs.find(cip) != jumpRefs.end()) {
			as->bind(L(as, cip));
		}

		codeMap_.insert(std::make_pair(cip, as->getCodeSize()));

		switch (instr.opcode()) {
		case OP_LOAD_PRI: // address
			// PRI = [address]
			as->mov(eax, dword_ptr(ebx, instr.operand()));
			break;
		case OP_LOAD_ALT: // address
			// ALT = [address]
			as->mov(ecx, dword_ptr(ebx, instr.operand()));
			break;
		case OP_LOAD_S_PRI: // offset
			// PRI = [FRM + offset]
			as->mov(eax, dword_ptr(ebp, instr.operand()));
			break;
		case OP_LOAD_S_ALT: // offset
			// ALT = [FRM + offset]
			as->mov(ecx, dword_ptr(ebp, instr.operand()));
			break;
		case OP_LREF_PRI: // address
			// PRI = [ [address] ]
			as->mov(edx, dword_ptr(ebx, instr.operand()));
			as->mov(eax, dword_ptr(ebx, edx));
			break;
		case OP_LREF_ALT: // address
			// ALT = [ [address] ]
			as->mov(edx, dword_ptr(ebx, + instr.operand()));
			as->mov(ecx, dword_ptr(ebx, edx));
			break;
		case OP_LREF_S_PRI: // offset
			// PRI = [ [FRM + offset] ]
			as->mov(edx, dword_ptr(ebp, instr.operand()));
			as->mov(eax, dword_ptr(ebx, edx));
			break;
		case OP_LREF_S_ALT: // offset
			// PRI = [ [FRM + offset] ]
			as->mov(edx, dword_ptr(ebp, instr.operand()));
			as->mov(ecx, dword_ptr(ebx, edx));
			break;
		case OP_LOAD_I:
			// PRI = [PRI] (full cell)
			as->mov(eax, dword_ptr(ebx, eax));
			break;
		case OP_LODB_I: // number
			// PRI = "number" bytes from [PRI] (read 1/2/4 bytes)
			switch (instr.operand()) {
			case 1:
				as->movzx(eax, byte_ptr(ebx, eax));
				break;
			case 2:
				as->movzx(eax, word_ptr(ebx, eax));
				break;
			case 4:
				as->mov(eax, dword_ptr(ebx, eax));
				break;
			}
			break;
		case OP_CONST_PRI: // value
			// PRI = value
			if (instr.operand() == 0) {
				as->xor_(eax, eax);
			} else {
				as->mov(eax, instr.operand());
			}
			break;
		case OP_CONST_ALT: // value
			// ALT = value
			if (instr.operand() == 0) {
				as->xor_(ecx, ecx);
			} else {
				as->mov(ecx, instr.operand());
			}
			break;
		case OP_ADDR_PRI: // offset
			// PRI = FRM + offset
			as->lea(eax, dword_ptr(ebp, instr.operand()));
			as->sub(eax, ebx);
			break;
		case OP_ADDR_ALT: // offset
			// ALT = FRM + offset
			as->lea(ecx, dword_ptr(ebp, instr.operand()));
			as->sub(ecx, ebx);
			break;
		case OP_STOR_PRI: // address
			// [address] = PRI
			as->mov(dword_ptr(ebx, instr.operand()), eax);
			break;
		case OP_STOR_ALT: // address
			// [address] = ALT
			as->mov(dword_ptr(ebx, instr.operand()), ecx);
			break;
		case OP_STOR_S_PRI: // offset
			// [FRM + offset] = ALT
			as->mov(dword_ptr(ebp, instr.operand()), eax);
			break;
		case OP_STOR_S_ALT: // offset
			// [FRM + offset] = ALT
			as->mov(dword_ptr(ebp, instr.operand()), ecx);
			break;
		case OP_SREF_PRI: // address
			// [ [address] ] = PRI
			as->mov(edx, dword_ptr(ebx, instr.operand()));
			as->mov(dword_ptr(ebx, edx), eax);
			break;
		case OP_SREF_ALT: // address
			// [ [address] ] = ALT
			as->mov(edx, dword_ptr(ebx, instr.operand()));
			as->mov(dword_ptr(ebx, edx), ecx);
			break;
		case OP_SREF_S_PRI: // offset
			// [ [FRM + offset] ] = PRI
			as->mov(edx, dword_ptr(ebp, instr.operand()));
			as->mov(dword_ptr(ebx, edx), eax);
			break;
		case OP_SREF_S_ALT: // offset
			// [ [FRM + offset] ] = ALT
			as->mov(edx, dword_ptr(ebp, instr.operand()));
			as->mov(dword_ptr(ebx, edx), ecx);
			break;
		case OP_STOR_I:
			// [ALT] = PRI (full cell)
			as->mov(dword_ptr(ebx, ecx), eax);
			break;
		case OP_STRB_I: // number
			// "number" bytes at [ALT] = PRI (write 1/2/4 bytes)
			switch (instr.operand()) {
			case 1:
				as->mov(byte_ptr(ebx, ecx), al);
				break;
			case 2:
				as->mov(word_ptr(ebx, ecx), ax);
				break;
			case 4:
				as->mov(dword_ptr(ebx, ecx), eax);
				break;
			}
			break;
		case OP_LIDX:
			// PRI = [ ALT + (PRI x cell size) ]
			as->lea(edx, dword_ptr(ebx, ecx));
			as->mov(eax, dword_ptr(edx, eax, 2));
			break;
		case OP_LIDX_B: // shift
			// PRI = [ ALT + (PRI << shift) ]
			as->lea(edx, dword_ptr(ebx, ecx));
			as->mov(eax, dword_ptr(edx, eax, instr.operand()));
			break;
		case OP_IDXADDR:
			// PRI = ALT + (PRI x cell size) (calculate indexed address)
			as->lea(eax, dword_ptr(ecx, eax, 2));
			break;
		case OP_IDXADDR_B: // shift
			// PRI = ALT + (PRI << shift) (calculate indexed address)
			as->lea(eax, dword_ptr(ecx, eax, instr.operand()));
			break;
		case OP_ALIGN_PRI: // number
			// Little Endian: PRI ^= cell size - number
			#if BYTE_ORDER == LITTLE_ENDIAN
				if (instr.operand() < sizeof(cell)) {
					as->xor_(eax, sizeof(cell) - instr.operand());
				}
			#endif
			break;
		case OP_ALIGN_ALT: // number
			// Little Endian: ALT ^= cell size - number
			#if BYTE_ORDER == LITTLE_ENDIAN
				if (instr.operand() < sizeof(cell)) {
					as->xor_(ecx, sizeof(cell) - instr.operand());
				}
			#endif
			break;
		case OP_LCTRL: // index
			// PRI is set to the current value of any of the special registers.
			// The index parameter must be: 0=COD, 1=DAT, 2=HEA,
			// 3=STP, 4=STK, 5=FRM, 6=CIP (of the next instruction)
			switch (instr.operand()) {
			case 0:
				as->mov(eax, dword_ptr_abs(reinterpret_cast<void*>(&amx_.amxHeader()->cod)));
				break;
			case 1:
				as->mov(eax, dword_ptr_abs(reinterpret_cast<void*>(&amx_.amxHeader()->dat)));
				break;
			case 2:
				as->mov(eax, dword_ptr_abs(reinterpret_cast<void*>(&amx_->hea)));
				break;
			case 4:
				as->mov(eax, esp);
				as->sub(eax, ebx);
				break;
			case 5:
				as->mov(eax, ebp);
				as->sub(eax, ebx);
				break;
			case 6: {
				as->mov(eax, instr.address() + instr.size());
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
			switch (instr.operand()) {
			case 2:
				as->mov(dword_ptr_abs(reinterpret_cast<void*>(&amx_->hea)), eax);
				break;
			case 4:
				as->lea(esp, dword_ptr(ebx, eax));
				break;
			case 5:
				as->lea(ebp, dword_ptr(ebx, eax));
				break;
			case 6:
				as->push(esp);
				as->push(eax);
				as->push(reinterpret_cast<int>(this));
				as->call(reinterpret_cast<int>(JIT::doJump));
				break;
			default:
				goto compile_error;
			}
			break;
		case OP_MOVE_PRI:
			// PRI = ALT
			as->mov(eax, ecx);
			break;
		case OP_MOVE_ALT:
			// ALT = PRI
			as->mov(ecx, eax);
			break;
		case OP_XCHG:
			// Exchange PRI and ALT
			as->xchg(eax, ecx);
			break;
		case OP_PUSH_PRI:
			// [STK] = PRI, STK = STK - cell size
			as->push(eax);
			break;
		case OP_PUSH_ALT:
			// [STK] = ALT, STK = STK - cell size
			as->push(ecx);
			break;
		case OP_PUSH_C: // value
			// [STK] = value, STK = STK - cell size
			as->push(instr.operand());
			break;
		case OP_PUSH: // address
			// [STK] = [address], STK = STK - cell size
			as->push(dword_ptr(ebx, instr.operand()));
			break;
		case OP_PUSH_S: // offset
			// [STK] = [FRM + offset], STK = STK - cell size
			as->push(dword_ptr(ebp, instr.operand()));
			break;
		case OP_POP_PRI:
			// STK = STK + cell size, PRI = [STK]
			as->pop(eax);
			break;
		case OP_POP_ALT:
			// STK = STK + cell size, ALT = [STK]
			as->pop(ecx);
			break;
		case OP_STACK: // value
			// ALT = STK, STK = STK + value
			if (!canOverwriteRegister(cip + instr.size(), REG_ALT)) {
				as->mov(ecx, esp);
				as->sub(ecx, ebx);
			}
			if (instr.operand() >= 0) {
				as->add(esp, instr.operand());
			} else {
				as->sub(esp, -instr.operand());
			}
			break;
		case OP_HEAP: // value
			// ALT = HEA, HEA = HEA + value
			if (!canOverwriteRegister(cip + instr.size(), REG_ALT)) {
				as->mov(ecx, dword_ptr_abs(reinterpret_cast<void*>(&amx_->hea)));
			}
			if (instr.operand() >= 0) {
				as->add(dword_ptr_abs(reinterpret_cast<void*>(&amx_->hea)), instr.operand());
			} else {
				as->sub(dword_ptr_abs(reinterpret_cast<void*>(&amx_->hea)), -instr.operand());
			}
			break;
		case OP_PROC:
			// [STK] = FRM, STK = STK - cell size, FRM = STK
			as->push(ebp);
			as->mov(ebp, esp);
			as->sub(dword_ptr(esp), ebx);
			break;
		case OP_RET:
			// STK = STK + cell size, FRM = [STK],
			// CIP = [STK], STK = STK + cell size
			as->pop(ebp);
			as->add(ebp, ebx);
			as->ret();
			break;
		case OP_RETN:
			// STK = STK + cell size, FRM = [STK],
			// CIP = [STK], STK = STK + cell size
			// The RETN instruction removes a specified number of bytes
			// from the stack. The value to adjust STK with must be
			// pushed prior to the call.
			as->pop(ebp);
			as->add(ebp, ebx);
			as->pop(edx);
			as->add(esp, dword_ptr(esp));
			as->push(edx);
			as->ret(4);
			break;
		case OP_CALL: // offset
			// [STK] = CIP + 5, STK = STK - cell size
			// CIP = CIP + offset
			// The CALL instruction jumps to an address after storing the
			// address of the next sequential instruction on the stack.
			// The address jumped to is relative to the current CIP,
			// but the address on the stack is an absolute address.
			as->call(L(as, instr.operand() - reinterpret_cast<cell>(amx_.code())));
			break;
		case OP_JUMP_PRI:
			// CIP = PRI (indirect jump)
			as->push(ebp);                         // stackBase
			as->lea(edx, dword_ptr(esp, -12));
			as->push(edx);                         // stackPtr
			as->push(eax);                         // address
			as->push(reinterpret_cast<int>(this)); // jit
			as->call(reinterpret_cast<int>(JIT::doJump));
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
			Label &L_dest = L(as, instr.operand() - reinterpret_cast<cell>(amx_.code()));
			switch (instr.opcode()) {
				case OP_JUMP: // offset
					// CIP = CIP + offset (jump to the address relative from
					// the current position)
					as->jmp(L_dest);
					break;
				case OP_JZER: // offset
					// if PRI == 0 then CIP = CIP + offset
					as->cmp(eax, 0);
					as->jz(L_dest);
					break;
				case OP_JNZ: // offset
					// if PRI != 0 then CIP = CIP + offset
					as->cmp(eax, 0);
					as->jnz(L_dest);
					break;
				case OP_JEQ: // offset
					// if PRI == ALT then CIP = CIP + offset
					as->cmp(eax, ecx);
					as->je(L_dest);
					break;
				case OP_JNEQ: // offset
					// if PRI != ALT then CIP = CIP + offset
					as->cmp(eax, ecx);
					as->jne(L_dest);
					break;
				case OP_JLESS: // offset
					// if PRI < ALT then CIP = CIP + offset (unsigned)
					as->cmp(eax, ecx);
					as->jb(L_dest);
					break;
				case OP_JLEQ: // offset
					// if PRI <= ALT then CIP = CIP + offset (unsigned)
					as->cmp(eax, ecx);
					as->jbe(L_dest);
					break;
				case OP_JGRTR: // offset
					// if PRI > ALT then CIP = CIP + offset (unsigned)
					as->cmp(eax, ecx);
					as->ja(L_dest);
					break;
				case OP_JGEQ: // offset
					// if PRI >= ALT then CIP = CIP + offset (unsigned)
					as->cmp(eax, ecx);
					as->jae(L_dest);
					break;
				case OP_JSLESS: // offset
					// if PRI < ALT then CIP = CIP + offset (signed)
					as->cmp(eax, ecx);
					as->jl(L_dest);
					break;
				case OP_JSLEQ: // offset
					// if PRI <= ALT then CIP = CIP + offset (signed)
					as->cmp(eax, ecx);
					as->jle(L_dest);
					break;
				case OP_JSGRTR: // offset
					// if PRI > ALT then CIP = CIP + offset (signed)
					as->cmp(eax, ecx);
					as->jg(L_dest);
					break;
				case OP_JSGEQ: // offset
					// if PRI >= ALT then CIP = CIP + offset (signed)
					as->cmp(eax, ecx);
					as->jge(L_dest);
					break;
			}
			break;
		}
		case OP_SHL:
			// PRI = PRI << ALT
			as->shl(eax, cl);
			break;
		case OP_SHR:
			// PRI = PRI >> ALT (without sign extension)
			as->shr(eax, cl);
			break;
		case OP_SSHR:
			// PRI = PRI >> ALT with sign extension
			as->sar(eax, cl);
			break;
		case OP_SHL_C_PRI: // value
			// PRI = PRI << value
			as->shl(eax, static_cast<unsigned char>(instr.operand()));
			break;
		case OP_SHL_C_ALT: // value
			// ALT = ALT << value
			as->shl(ecx, static_cast<unsigned char>(instr.operand()));
			break;
		case OP_SHR_C_PRI: // value
			// PRI = PRI >> value (without sign extension)
			as->shr(eax, static_cast<unsigned char>(instr.operand()));
			break;
		case OP_SHR_C_ALT: // value
			// PRI = PRI >> value (without sign extension)
			as->shl(ecx, static_cast<unsigned char>(instr.operand()));
			break;
		case OP_SMUL:
			// PRI = PRI * ALT (signed multiply)
			as->xor_(edx, edx);
			as->imul(ecx);
			break;
		case OP_SDIV:
			// PRI = PRI / ALT (signed divide), ALT = PRI mod ALT
			as->xor_(edx, edx);
			as->idiv(ecx);
			as->mov(ecx, edx);
			break;
		case OP_SDIV_ALT:
			// PRI = ALT / PRI (signed divide), ALT = ALT mod PRI
			as->xchg(eax, ecx);
			as->xor_(edx, edx);
			as->idiv(ecx);
			as->mov(ecx, edx);
			break;
		case OP_UMUL:
			// PRI = PRI * ALT (unsigned multiply)
			as->xor_(edx, edx);
			as->mul(ecx);
			break;
		case OP_UDIV:
			// PRI = PRI / ALT (unsigned divide), ALT = PRI mod ALT
			as->xor_(edx, edx);
			as->div(ecx);
			as->mov(ecx, edx);
			break;
		case OP_UDIV_ALT:
			// PRI = ALT / PRI (unsigned divide), ALT = ALT mod PRI
			as->xchg(eax, ecx);
			as->xor_(edx, edx);
			as->div(ecx);
			as->mov(ecx, edx);
			break;
		case OP_ADD:
			// PRI = PRI + ALT
			as->add(eax, ecx);
			break;
		case OP_SUB:
			// PRI = PRI - ALT
			as->sub(eax, ecx);
			break;
		case OP_SUB_ALT:
			// PRI = ALT - PRI
			// or:
			// PRI = -(PRI - ALT)
			as->sub(eax, ecx);
			as->neg(eax);
			break;
		case OP_AND:
			// PRI = PRI & ALT
			as->and_(eax, ecx);
			break;
		case OP_OR:
			// PRI = PRI | ALT
			as->or_(eax, ecx);
			break;
		case OP_XOR:
			// PRI = PRI ^ ALT
			as->xor_(eax, ecx);
			break;
		case OP_NOT:
			// PRI = !PRI
			as->test(eax, eax);
			as->setz(al);
			as->movzx(eax, al);
			break;
		case OP_NEG:
			// PRI = -PRI
			as->neg(eax);
			break;
		case OP_INVERT:
			// PRI = ~PRI
			as->not_(eax);
			break;
		case OP_ADD_C: // value
			// PRI = PRI + value
			as->add(eax, instr.operand());
			break;
		case OP_SMUL_C: // value
			// PRI = PRI * value
			as->imul(eax, instr.operand());
			break;
		case OP_ZERO_PRI:
			// PRI = 0
			as->xor_(eax, eax);
			break;
		case OP_ZERO_ALT:
			// ALT = 0
			as->xor_(ecx, ecx);
			break;
		case OP_ZERO: // address
			// [address] = 0
			as->mov(dword_ptr(ebx, instr.operand()), 0);
			break;
		case OP_ZERO_S: // offset
			// [FRM + offset] = 0
			as->mov(dword_ptr(ebp, instr.operand()), 0);
			break;
		case OP_SIGN_PRI:
			// sign extent the byte in PRI to a cell
			as->movsx(eax, al);
			break;
		case OP_SIGN_ALT:
			// sign extent the byte in ALT to a cell
			as->movsx(ecx, cl);
			break;
		case OP_EQ:
			// PRI = PRI == ALT ? 1 : 0
			as->cmp(eax, ecx);
			as->sete(al);
			as->movzx(eax, al);
			break;
		case OP_NEQ:
			// PRI = PRI != ALT ? 1 : 0
			as->cmp(eax, ecx);
			as->setne(al);
			as->movzx(eax, al);
			break;
		case OP_LESS:
			// PRI = PRI < ALT ? 1 : 0 (unsigned)
			as->cmp(eax, ecx);
			as->setb(al);
			as->movzx(eax, al);
			break;
		case OP_LEQ:
			// PRI = PRI <= ALT ? 1 : 0 (unsigned)
			as->cmp(eax, ecx);
			as->setbe(al);
			as->movzx(eax, al);
			break;
		case OP_GRTR:
			// PRI = PRI > ALT ? 1 : 0 (unsigned)
			as->cmp(eax, ecx);
			as->seta(al);
			as->movzx(eax, al);
			break;
		case OP_GEQ:
			// PRI = PRI >= ALT ? 1 : 0 (unsigned)
			as->cmp(eax, ecx);
			as->setae(al);
			as->movzx(eax, al);
			break;
		case OP_SLESS:
			// PRI = PRI < ALT ? 1 : 0 (signed)
			as->cmp(eax, ecx);
			as->setl(al);
			as->movzx(eax, al);
			break;
		case OP_SLEQ:
			// PRI = PRI <= ALT ? 1 : 0 (signed)
			as->cmp(eax, ecx);
			as->setle(al);
			as->movzx(eax, al);
			break;
		case OP_SGRTR:
			// PRI = PRI > ALT ? 1 : 0 (signed)
			as->cmp(eax, ecx);
			as->setg(al);
			as->movzx(eax, al);
			break;
		case OP_SGEQ:
			// PRI = PRI >= ALT ? 1 : 0 (signed)
			as->cmp(eax, ecx);
			as->setge(al);
			as->movzx(eax, al);
			break;
		case OP_EQ_C_PRI: // value
			// PRI = PRI == value ? 1 : 0
			as->cmp(eax, instr.operand());
			as->sete(al);
			as->movzx(eax, al);
			break;
		case OP_EQ_C_ALT: // value
			// PRI = ALT == value ? 1 : 0
			as->cmp(ecx, instr.operand());
			as->sete(al);
			as->movzx(eax, al);
			break;
		case OP_INC_PRI:
			// PRI = PRI + 1
			as->inc(eax);
			break;
		case OP_INC_ALT:
			// ALT = ALT + 1
			as->inc(ecx);
			break;
		case OP_INC: // address
			// [address] = [address] + 1
			as->inc(dword_ptr(ebx, instr.operand()));
			break;
		case OP_INC_S: // offset
			// [FRM + offset] = [FRM + offset] + 1
			as->inc(dword_ptr(ebp, instr.operand()));
			break;
		case OP_INC_I:
			// [PRI] = [PRI] + 1
			as->inc(dword_ptr(ebx, eax));
			break;
		case OP_DEC_PRI:
			// PRI = PRI - 1
			as->dec(eax);
			break;
		case OP_DEC_ALT:
			// ALT = ALT - 1
			as->dec(ecx);
			break;
		case OP_DEC: // address
			// [address] = [address] - 1
			as->dec(dword_ptr(ebx, instr.operand()));
			break;
		case OP_DEC_S: // offset
			// [FRM + offset] = [FRM + offset] - 1
			as->dec(dword_ptr(ebp, instr.operand()));
			break;
		case OP_DEC_I:
			// [PRI] = [PRI] - 1
			as->dec(dword_ptr(ebx, eax));
			break;
		case OP_MOVS: { // number
			// Copy memory from [PRI] to [ALT]. The parameter
			// specifies the number of bytes. The blocks should not
			// overlap.
			bool needALT = !canOverwriteRegister(cip + instr.size(), REG_ALT);
			as->cld();
			as->lea(esi, dword_ptr(ebx, eax));
			as->lea(edi, dword_ptr(ebx, ecx));
			if (needALT) {
				as->push(ecx);
			}
			if (instr.operand() % 4 == 0) {
				as->mov(ecx, instr.operand() / 4);
				as->rep_movsd();
			} else if (instr.operand() % 2 == 0) {
				as->mov(ecx, instr.operand() / 2);
				as->rep_movsw();
			} else {
				as->mov(ecx, instr.operand());
				as->rep_movsb();
			}
			if (needALT) {
				as->pop(ecx);
			}
			break;
		}
		case OP_CMPS: { // number
			// Compare memory blocks at [PRI] and [ALT]. The parameter
			// specifies the number of bytes. The blocks should not
			// overlap.
			Label L_above(L(as, cip, "above"));
			Label L_below(L(as, cip, "below"));
			Label L_equal(L(as, cip, "equal"));
			Label L_continue(L(as, cip, "continue"));
			bool needALT = !canOverwriteRegister(cip + instr.size(), REG_ALT);
				as->cld();
				as->lea(edi, dword_ptr(ebx, eax));
				as->lea(esi, dword_ptr(ebx, ecx));
				if (needALT) {
					as->push(ecx);
				}
				as->mov(ecx, instr.operand());
				as->repe_cmpsb();
				if (needALT) {
					as->pop(ecx);
				}
				as->ja(L_above);
				as->jb(L_below);
				as->jz(L_equal);
			as->bind(L_above);
				as->mov(eax, 1);
				as->jmp(L_continue);
			as->bind(L_below);
				as->mov(eax, -1);
				as->jmp(L_continue);
			as->bind(L_equal);
				as->xor_(eax, eax);
			as->bind(L_continue);
			break;
		}
		case OP_FILL: { // number
			// Fill memory at [ALT] with value in [PRI]. The parameter
			// specifies the number of bytes, which must be a multiple
			// of the cell size.
			bool needALT = !canOverwriteRegister(cip + instr.size(), REG_ALT);
			as->cld();
			as->lea(edi, dword_ptr(ebx, ecx));
			if (needALT) {
				as->push(ecx);
			}
			as->mov(ecx, instr.operand() / sizeof(cell));
			as->rep_stosd();
			if (needALT) {
				as->pop(ecx);
			}
			break;
		}
		case OP_HALT: // number
			// Abort execution (exit value in PRI), parameters other than 0
			// have a special meaning.
			as->mov(ecx, instr.operand());
			as->jmp(L_halt_);
			break;
		case OP_BOUNDS: { // value
			// Abort execution if PRI > value or if PRI < 0.
			Label &L_halt = L(as, cip, "halt");
			Label &L_good = L(as, cip, "good");
				as->cmp(eax, instr.operand());
			as->jg(L_halt);
				as->cmp(eax, 0);
				as->jl(L_halt);
				as->jmp(L_good);
			as->bind(L_halt);
				as->mov(ecx, AMX_ERR_BOUNDS);
				as->jmp(L_halt_);
			as->bind(L_good);
			break;
		}
		case OP_SYSREQ_PRI: {
			// call system service, service number in PRI
			as->push(esp);                         // stackPtr
			as->push(ebp);                         // stackBase
			as->push(eax);                         // index
			as->push(reinterpret_cast<int>(this)); // jit
			as->call(reinterpret_cast<int>(JIT::doSysreqC));
			break;
		}
		case OP_SYSREQ_C:   // index
		case OP_SYSREQ_D: { // address
			// call system service
			const char *nativeName = 0;
			switch (instr.opcode()) {
				case OP_SYSREQ_C:
					nativeName = amx_.getNativeName(instr.operand());
					break;
				case OP_SYSREQ_D: {
					nativeName = amx_.getNativeName(amx_.getNativeIndex(instr.operand()));
					break;
				}
			}
			if (nativeName == 0) {
				goto invalid_native;
			}

			// Replace calls to various natives with their optimized equivalents.
			for (int i = 0; i < sizeof(intrinsics_) / sizeof(*intrinsics_); i++) {
				if (intrinsics_[i].name == nativeName) {
					(*this.*(intrinsics_[i].impl))(as);
					goto special_native;
				}
			}
			goto ordinary_native;

		ordinary_native:
			switch (instr.opcode()) {
				case OP_SYSREQ_C: {
					as->push(esp);                         // stackPtr
					as->push(ebp);                         // stackBase
					as->push(instr.operand());             // index
					as->push(reinterpret_cast<int>(this)); // jit
					as->call(reinterpret_cast<int>(JIT::doSysreqC));
					break;
				}
				case OP_SYSREQ_D:
					as->push(esp);                         // stackPtr
					as->push(ebp);                         // stackBase
					as->push(instr.operand());             // address
					as->push(reinterpret_cast<int>(this)); // jit
					as->call(reinterpret_cast<int>(JIT::doSysreqD));
					break;
			}
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
			case_table = reinterpret_cast<case_record*>(instr.operand() + sizeof(cell));

			// Get address of the "default" record.
			cell default_addr = case_table[0].address - reinterpret_cast<cell>(amx_.code());

			// The number of cases follows the CASETBL opcode (which follows the SWITCH).
			int num_cases = *(reinterpret_cast<cell*>(instr.operand()) + 1);

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
				as->cmp(eax, *min_value);
				as->jl(L(as, default_addr));
				as->cmp(eax, *max_value);
				as->jg(L(as, default_addr));

				// OK now sequentially compare eax with each value.
				// This is pretty slow so I probably should optimize
				// this in future...
				for (int i = 0; i < num_cases; i++) {
					as->cmp(eax, case_table[i + 1].value);
					as->je(L(as, case_table[i + 1].address - reinterpret_cast<cell>(amx_.code())));
				}
			}

			// No match found - go for default case.
			as->jmp(L(as, default_addr));
			break;
		}
		case OP_CASETBL: // ...
			// A variable number of case records follows this opcode, where
			// each record takes two cells.
			break;
		case OP_SWAP_PRI:
			// [STK] = PRI and PRI = [STK]
			as->xchg(dword_ptr(esp), eax);
			break;
		case OP_SWAP_ALT:
			// [STK] = ALT and ALT = [STK]
			as->xchg(dword_ptr(esp), ecx);
			break;
		case OP_PUSH_ADR: // offset
			// [STK] = FRM + offset, STK = STK - cell size
			as->lea(edx, dword_ptr(ebp, instr.operand()));
			as->sub(edx, ebx);
			as->push(edx);
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

	if (logger != 0) {
		logger->logString("\n\n\n");
	}
	as->bind(L_halt_);
		as->push(ecx);
		as->push(reinterpret_cast<int>(this));
		as->call(reinterpret_cast<int>(JIT::doHalt));

	if (error) {
		goto compile_error;
	}

	code_     = as->make();
	codeSize_ = as->getCodeSize();

	if (as != assembler_) {
		delete as;
	}

	return true;

compile_error:
	if (errorHandler != 0) {
		errorHandler(amx_, instr);
	}

	if (as != assembler_) {
		delete as;
	}

	return false;
}

int JIT::call(cell address, cell *retval) {
	if (amx_->hea >= amx_->stk) {
		return AMX_ERR_STACKERR;
	}
	if (amx_->hea < amx_->hlw) {
		return AMX_ERR_HEAPLOW;
	}
	if (amx_->stk > amx_->stp) {
		return AMX_ERR_STACKLOW;
	}

	// Make sure all natives are registered.
	if ((amx_->flags & AMX_FLAG_NTVREG) == 0) {
		return AMX_ERR_NOTFOUND;
	}

	if (unlikely(callHelper_ == 0)) {
		X86Assembler as;

		as.mov(eax, dword_ptr(esp, 4));
		as.push(esi);
		as.push(edi);
		as.push(ebx);
		as.push(ecx);
		as.push(edx);

		as.mov(dword_ptr_abs(&ebp_), ebp);
		as.mov(edx, dword_ptr_abs(&amx_->frm));
		as.lea(ebp, dword_ptr(edx, reinterpret_cast<int>(amx_.data())));
		as.mov(dword_ptr_abs(&esp_), esp);
		as.mov(edx, dword_ptr_abs(&amx_->stk));
		as.lea(esp, dword_ptr(edx, reinterpret_cast<int>(amx_.data())));

		as.lea(ecx, dword_ptr(esp, - 4));
		as.mov(dword_ptr_abs(&haltEsp_), ecx);
		as.mov(dword_ptr_abs(&haltEbp_), ebp);
		as.mov(ebx, reinterpret_cast<int>(amx_.data()));
		as.call(eax);

		as.lea(edx, dword_ptr(ebp, -reinterpret_cast<int>(amx_.data())));
		as.mov(dword_ptr_abs(&amx_->frm), edx);
		as.mov(ebp, dword_ptr_abs(&ebp_));
		as.lea(edx, dword_ptr(esp, -reinterpret_cast<int>(amx_.data())));
		as.mov(dword_ptr_abs(&amx_->stk), edx);
		as.mov(esp, dword_ptr_abs(&esp_));

		as.pop(edx);
		as.pop(ecx);
		as.pop(ebx);
		as.pop(edi);
		as.pop(esi);
		as.ret();

		callHelper_ = (CallHelper)as.make();
	}

	amx_->error = AMX_ERR_NONE;

	void *start = getInstrPtr(address);
	assert(start != 0);

	void *haltEbp = haltEbp_;
	void *haltEsp = haltEsp_;

	cell retval_ = callHelper_(start);
	if (retval != 0) {
		*retval = retval_;
	}

	haltEbp_ = haltEbp;
	haltEsp_ = haltEsp;

	return amx_->error;
}

int JIT::exec(int index, cell *retval) {
	cell address = amx_.getPublicAddress(index);
	if (address == 0) {
		return AMX_ERR_INDEX;
	}

	// Push size of arguments and reset parameter count.
	amx_.push(amx_->paramcount * sizeof(cell));
	amx_->paramcount = 0;

	return call(address, retval);
}

bool JIT::canOverwriteRegister(cell address, AMXRegister reg) const {
	AMXDisassembler disas(amx_);
	disas.setIp(address);

	AMXInstruction instr;
	while (disas.decode(instr)) {
		if (instr.inputRegisters() & reg) {
			// The registers is read by one or more of subsequent instructions, so
			// it can't be overwritten.
			return false;
		}
		if (instr.outputRegisters() & reg) {
			// OK - something overwrites it anyway, it must be safe to modify it.
			return true;
		}
	}

	// Nothing reads or writes the register.
	return true;
}

bool JIT::getJumpRefs(std::set<cell> &refs) const {
	AMXDisassembler disas(amx_);
	disas.setOpcodeTable(opcodeTable_);

	AMXInstruction instr;
	bool error = false;

	while (disas.decode(instr, &error)) {
		AMXOpcode opcode = instr.opcode();
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
				refs.insert(instr.operand() - reinterpret_cast<int>(amx_.code()));
				break;
			case OP_CASETBL: {
				int n = instr.numOperands();
				for (int i = 1; i < n; i += 2) {
					refs.insert(instr.operand(i) - reinterpret_cast<int>(amx_.code()));
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

Label &JIT::L(X86Assembler *as, cell address) {
	return L(as, address, std::string());
}

Label &JIT::L(X86Assembler *as, cell address, const std::string &name) {
	std::pair<cell, std::string> key = std::make_pair(address, name);
	LabelMap::iterator iterator = labelMap_.find(key);
	if (iterator != labelMap_.end()) {
		return iterator->second;
	} else {
		std::pair<LabelMap::iterator, bool> where =
				labelMap_.insert(std::make_pair(key, as->newLabel()));
		return where.first->second;
	}
}

void JIT::jump(cell address, void *stackBase, void *stackPtr) {
	CodeMap::const_iterator it = codeMap_.find(address);

	if (it != codeMap_.end()) {
		void *dest = getInstrPtr(address);

		if (unlikely(jumpHelper_ == 0)) {
			X86Assembler as;

			as.mov(eax, dword_ptr(esp, 4));  // dest
			as.mov(ebp, dword_ptr(esp, 8));  // stackBase
			as.mov(esp, dword_ptr(esp, 12)); // stackPtr
			as.jmp(eax);

			jumpHelper_ = (JumpHelper)as.make();
		}

		jumpHelper_(dest, stackPtr, stackBase);
	}
}

// static
void JIT_CDECL JIT::doJump(JIT *jit, cell address, void *stackBase, void *stackPtr) {
	jit->jump(address, stackBase, stackPtr);
	doHalt(jit, AMX_ERR_INVINSTR);
}

void JIT::halt(int error) {
	if (unlikely(haltHelper_ == 0)) {
		X86Assembler as;

		as.mov(eax, dword_ptr(esp, 4));
		as.mov(dword_ptr_abs(reinterpret_cast<void*>(&amx_->error)), eax);
		as.mov(esp, dword_ptr_abs(reinterpret_cast<void*>(&haltEsp_)));
		as.mov(ebp, dword_ptr_abs(reinterpret_cast<void*>(&haltEbp_)));
		as.ret();

		haltHelper_ = (HaltHelper)as.make();
	}

	haltHelper_(error);
}

// static
void JIT_CDECL JIT::doHalt(JIT *jit, int error) {
	jit->halt(error);
}

void JIT::sysreqC(cell index, void *stackBase, void *stackPtr) {
	cell address = amx_.getNativeAddress(index);
	if (address == 0) {
		halt(AMX_ERR_NOTFOUND);
	}
	sysreqD(address, stackBase, stackPtr);
}

// static
void JIT_CDECL JIT::doSysreqC(JIT *jit, cell index, void *stackBase, void *stackPtr) {
	jit->sysreqC(index, stackBase, stackPtr);
}

void JIT::sysreqD(cell address, void *stackBase, void *stackPtr) {
	if (unlikely(sysreqHelper_ == 0)) {
		X86Assembler as;

		as.mov(eax, dword_ptr(esp, 4));   // address
		as.mov(ebp, dword_ptr(esp, 8));   // stackBase
		as.mov(esp, dword_ptr(esp, 12));  // stackPtr
		as.mov(ecx, esp);                 // params
		as.mov(ebx, dword_ptr(esp, -20)); // return address

		as.lea(edx, dword_ptr(ebp, -reinterpret_cast<int>(amx_.data())));
		as.mov(dword_ptr_abs(&amx_->frm), edx);
		as.mov(ebp, dword_ptr_abs(&ebp_));
		as.lea(edx, dword_ptr(esp, -reinterpret_cast<int>(amx_.data())));
		as.mov(dword_ptr_abs(&amx_->stk), edx);
		as.mov(esp, dword_ptr_abs(&esp_));

		as.push(ecx);
		as.push(reinterpret_cast<int>(amx_.amx()));
		as.call(eax);
		as.add(esp, 8);

		as.mov(dword_ptr_abs(&ebp_), ebp);
		as.mov(edx, dword_ptr_abs(&amx_->frm));
		as.lea(ebp, dword_ptr(edx, reinterpret_cast<int>(amx_.data())));
		as.mov(dword_ptr_abs(&esp_), esp);
		as.mov(edx, dword_ptr_abs(&amx_->stk));
		as.lea(esp, dword_ptr(edx, reinterpret_cast<int>(amx_.data())));
		
		as.push(ebx);
		as.mov(ebx, reinterpret_cast<int>(amx_.data()));
		as.ret();

		sysreqHelper_ = (SysreqHelper)as.make();
	}

	sysreqHelper_(address, stackBase, stackPtr);
}

// static
void JIT_CDECL JIT::doSysreqD(JIT *jit, cell address, void *stackBase, void *stackPtr) {
	jit->sysreqD(address, stackBase, stackPtr);
}

void JIT::native_float(X86Assembler *as) {
	as->fild(dword_ptr(esp, 4));
	as->sub(esp, 4);
	as->fstp(dword_ptr(esp));
	as->mov(eax, dword_ptr(esp));
	as->add(esp, 4);
}

void JIT::native_floatabs(X86Assembler *as) {
	as->fld(dword_ptr(esp, 4));
	as->fabs();
	as->sub(esp, 4);
	as->fstp(dword_ptr(esp));
	as->mov(eax, dword_ptr(esp));
	as->add(esp, 4);
}

void JIT::native_floatadd(X86Assembler *as) {
	as->fld(dword_ptr(esp, 4));
	as->fadd(dword_ptr(esp, 8));
	as->sub(esp, 4);
	as->fstp(dword_ptr(esp));
	as->mov(eax, dword_ptr(esp));
	as->add(esp, 4);
}

void JIT::native_floatsub(X86Assembler *as) {
	as->fld(dword_ptr(esp, 4));
	as->fsub(dword_ptr(esp, 8));
	as->sub(esp, 4);
	as->fstp(dword_ptr(esp));
	as->mov(eax, dword_ptr(esp));
	as->add(esp, 4);
}

void JIT::native_floatmul(X86Assembler *as) {
	as->fld(dword_ptr(esp, 4));
	as->fmul(dword_ptr(esp, 8));
	as->sub(esp, 4);
	as->fstp(dword_ptr(esp));
	as->mov(eax, dword_ptr(esp));
	as->add(esp, 4);
}

void JIT::native_floatdiv(X86Assembler *as) {
	as->fld(dword_ptr(esp, 4));
	as->fdiv(dword_ptr(esp, 8));
	as->sub(esp, 4);
	as->fstp(dword_ptr(esp));
	as->mov(eax, dword_ptr(esp));
	as->add(esp, 4);
}

void JIT::native_floatsqroot(X86Assembler *as) {
	as->fld(dword_ptr(esp, 4));
	as->fsqrt();
	as->sub(esp, 4);
	as->fstp(dword_ptr(esp));
	as->mov(eax, dword_ptr(esp));
	as->add(esp, 4);
}

void JIT::native_floatlog(X86Assembler *as) {
	as->fld1();
	as->fld(dword_ptr(esp, 8));
	as->fyl2x();
	as->fld1();
	as->fdivrp(st(1));
	as->fld(dword_ptr(esp, 4));
	as->fyl2x();
	as->sub(esp, 4);
	as->fstp(dword_ptr(esp));
	as->mov(eax, dword_ptr(esp));
	as->add(esp, 4);
}

} // namespace jit
