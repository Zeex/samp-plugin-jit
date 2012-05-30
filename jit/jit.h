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

#ifndef JIT_H
#define JIT_H

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <utility>

#include <amx/amx.h>
#include <AsmJit/AsmJit.h>

#if defined _MSC_VER
	#define JIT_CDECL __cdecl
	#define JIT_STDCALL __stdcall
#elif defined __GNUC__
	#define JIT_CDECL __attribute__((cdecl))
	#define JIT_STDCALL __attribute__((stdcall))
#endif

namespace jit {

enum AmxOpcode {
	OP_NONE,         OP_LOAD_PRI,     OP_LOAD_ALT,     OP_LOAD_S_PRI,
	OP_LOAD_S_ALT,   OP_LREF_PRI,     OP_LREF_ALT,     OP_LREF_S_PRI,
	OP_LREF_S_ALT,   OP_LOAD_I,       OP_LODB_I,       OP_CONST_PRI,
	OP_CONST_ALT,    OP_ADDR_PRI,     OP_ADDR_ALT,     OP_STOR_PRI,
	OP_STOR_ALT,     OP_STOR_S_PRI,   OP_STOR_S_ALT,   OP_SREF_PRI,
	OP_SREF_ALT,     OP_SREF_S_PRI,   OP_SREF_S_ALT,   OP_STOR_I,
	OP_STRB_I,       OP_LIDX,         OP_LIDX_B,       OP_IDXADDR,
	OP_IDXADDR_B,    OP_ALIGN_PRI,    OP_ALIGN_ALT,    OP_LCTRL,
	OP_SCTRL,        OP_MOVE_PRI,     OP_MOVE_ALT,     OP_XCHG,
	OP_PUSH_PRI,     OP_PUSH_ALT,     OP_PUSH_R,       OP_PUSH_C,
	OP_PUSH,         OP_PUSH_S,       OP_POP_PRI,      OP_POP_ALT,
	OP_STACK,        OP_HEAP,         OP_PROC,         OP_RET,
	OP_RETN,         OP_CALL,         OP_CALL_PRI,     OP_JUMP,
	OP_JREL,         OP_JZER,         OP_JNZ,          OP_JEQ,
	OP_JNEQ,         OP_JLESS,        OP_JLEQ,         OP_JGRTR,
	OP_JGEQ,         OP_JSLESS,       OP_JSLEQ,        OP_JSGRTR,
	OP_JSGEQ,        OP_SHL,          OP_SHR,          OP_SSHR,
	OP_SHL_C_PRI,    OP_SHL_C_ALT,    OP_SHR_C_PRI,    OP_SHR_C_ALT,
	OP_SMUL,         OP_SDIV,         OP_SDIV_ALT,     OP_UMUL,
	OP_UDIV,         OP_UDIV_ALT,     OP_ADD,          OP_SUB,
	OP_SUB_ALT,      OP_AND,          OP_OR,           OP_XOR,
	OP_NOT,          OP_NEG,          OP_INVERT,       OP_ADD_C,
	OP_SMUL_C,       OP_ZERO_PRI,     OP_ZERO_ALT,     OP_ZERO,
	OP_ZERO_S,       OP_SIGN_PRI,     OP_SIGN_ALT,     OP_EQ,
	OP_NEQ,          OP_LESS,         OP_LEQ,          OP_GRTR,
	OP_GEQ,          OP_SLESS,        OP_SLEQ,         OP_SGRTR,
	OP_SGEQ,         OP_EQ_C_PRI,     OP_EQ_C_ALT,     OP_INC_PRI,
	OP_INC_ALT,      OP_INC,          OP_INC_S,        OP_INC_I,
	OP_DEC_PRI,      OP_DEC_ALT,      OP_DEC,          OP_DEC_S,
	OP_DEC_I,        OP_MOVS,         OP_CMPS,         OP_FILL,
	OP_HALT,         OP_BOUNDS,       OP_SYSREQ_PRI,   OP_SYSREQ_C,
	OP_FILE,         OP_LINE,         OP_SYMBOL,       OP_SRANGE,
	OP_JUMP_PRI,     OP_SWITCH,       OP_CASETBL,      OP_SWAP_PRI,
	OP_SWAP_ALT,     OP_PUSH_ADR,     OP_NOP,          OP_SYSREQ_D,
	OP_SYMTAG,       OP_BREAK,
	NUM_AMX_OPCODES
};

class AmxInstruction {
public:
	AmxInstruction();

	inline int getSize() const {
		return sizeof(cell) * (1 + operands_.size());
	}

	inline const cell getAddress() const {
		return address_; 
	}
	inline void setAddress(cell address) {
		address_ = address;
	}

	inline AmxOpcode getOpcode() const {
		return opcode_;
	}
	inline void setOpcode(AmxOpcode opcode) {
		opcode_ = opcode;
	}

	inline cell getOperand(unsigned int index = 0u) {
		assert(index >= 0 && index < operands_.size());
		return operands_[index];
	}

	inline std::vector<cell> &getOperands() {
		return operands_;
	}
	inline const std::vector<cell> &getOperands() const {
		return operands_;
	}
	inline void setOperands(std::vector<cell> operands) {
		operands_ = operands;
	}

	inline void addOperand(cell value) {
		operands_.push_back(value);
	}

	inline int getNumOperands() const {
		return operands_.size();
	}

	const char *getName() const;

	// Convert to a string: <name> <operands> ...
	std::string asString() const;

private:
	cell address_;
	AmxOpcode opcode_;
	std::vector<cell> operands_;

private:
	static const char *opcodeNames[];
};

class AmxVm {
public:
	AmxVm(AMX *amx) : amx_(amx) {}

	AMX *operator->() {
		return amx_;
	}

	inline AMX *getAmx() const {
		return amx_;
	}
	inline AMX_HEADER *getHeader() const {
		return reinterpret_cast<AMX_HEADER*>(amx_->base);
	}
	inline unsigned char *getData() const {
		return amx_->data != 0 ? amx_->data : amx_->base + getHeader()->dat;
	}
	inline unsigned char *getCode() const {
		return amx_->base + getHeader()->cod;
	}

	inline int getNumPublics() const {
		return (getHeader()->natives - getHeader()->publics) / getHeader()->defsize;
	}
	inline int getNumNatives() const {
		return (getHeader()->libraries - getHeader()->natives) / getHeader()->defsize;
	}

	inline AMX_FUNCSTUBNT *getPublics() const {
		return reinterpret_cast<AMX_FUNCSTUBNT*>(getHeader()->publics + amx_->base);
	}
	inline AMX_FUNCSTUBNT *getNatives() const {
		return reinterpret_cast<AMX_FUNCSTUBNT*>(getHeader()->natives + amx_->base);
	}

	cell getPublicAddress(int index) const;
	cell getNativeAddress(int index) const;

	int getPublicIndex(cell address) const;
	int getNativeIndex(cell address) const;

	const char *getPublicName(int index) const;
	const char *getNativeName(int index) const;

	inline cell *getStack() const {
		return reinterpret_cast<cell*>(getData() + amx_->stk);
	}

	cell *pushStack(cell value);
	cell  popStack();
	void  popStack(int ncells);

private:
	AMX *amx_;
};

class AmxDisassembler {
public:
	AmxDisassembler(AmxVm vm);

	inline void setOpcodeTable(cell *opcodeTable) {
		opcodeTable_ = opcodeTable;
	}

	inline void setIp(cell ip) {
		ip_ = ip;
	}
	inline cell getIp() const {
		return ip_;
	}

	bool decode(AmxInstruction &instr, bool *error = 0);

private:
	AmxVm vm_;
	cell *opcodeTable_;
	cell ip_;
};

class Jitter {
public:
	typedef void (*CompileErrorHandler)(const AmxVm &vm, const AmxInstruction &instr);

public:
	Jitter(AMX *amx);
	~Jitter();

	inline void *getCode() const {
		return code_;
	}
	inline std::size_t getCodeSize() const {
		return codeSize_;
	}

	void *getInstrPtr(cell address) const;
	int   getInstrOffset(cell address) const;

	inline void setOpcodeTable(cell *opcodeTable) {
		opcodeTable_ = opcodeTable;
	}
	inline cell *getOpcodeTable() const {
		return opcodeTable_;
	}

	inline void setAssembler(AsmJit::X86Assembler *assembler) {
		assembler_ = assembler;
	}
	inline AsmJit::X86Assembler *getAssembler() const {
		return assembler_;
	}

	bool compile(CompileErrorHandler errorHandler = 0);

	void halt(int error);
	void jump(cell address, void *stack);

	int call(cell address, cell *retval);
	int exec(cell index, cell *retval);

	int sysreqC(cell index, cell *params, cell *retval);
	int sysreqD(cell address, cell *params, cell *retval);

private:
	Jitter(const Jitter &);
	Jitter &operator=(const Jitter &);

private:
	bool getJumpRefs(std::set<cell> &refs) const;

	AsmJit::Label &L(AsmJit::X86Assembler *as, cell address);
	AsmJit::Label &L(AsmJit::X86Assembler *as, cell address, const std::string &name);

	static void JIT_STDCALL doJump(Jitter *jitter, cell address, void *stack);
	static cell JIT_STDCALL doSysreqC(Jitter *jitter, cell index, cell *params);
	static cell JIT_STDCALL doSysreqD(Jitter *jitter, cell address, cell *params);
	static void JIT_STDCALL doHalt(Jitter *jitter, int error);

private:
	typedef void (Jitter::*IntrinsicImpl)(AsmJit::X86Assembler *as);

	struct Intrinsic {
		std::string   name;
		IntrinsicImpl impl;
	};

	static Intrinsic intrinsics_[];

	void native_float(AsmJit::X86Assembler *as);
	void native_floatabs(AsmJit::X86Assembler *as);
	void native_floatadd(AsmJit::X86Assembler *as);
	void native_floatsub(AsmJit::X86Assembler *as);
	void native_floatmul(AsmJit::X86Assembler *as);
	void native_floatdiv(AsmJit::X86Assembler *as);
	void native_floatsqroot(AsmJit::X86Assembler *as);
	void native_floatlog(AsmJit::X86Assembler *as);

private:
	AsmJit::Label L_halt_;
	void halt(AsmJit::X86Assembler *as, cell errorCode);

	void beginJitCode(AsmJit::X86Assembler *as);
	void endJitCode(AsmJit::X86Assembler *as);

private:
	AmxVm vm_;
	cell *opcodeTable_;

	AsmJit::X86Assembler *assembler_;

	void *code_;
	std::size_t codeSize_;

	void *ebp_;
	void *esp_;

	void *haltEbp_;
	void *haltEsp_;

	typedef void (JIT_CDECL *JumpHelper)(void *dest, void *stack);
	JumpHelper jumpHelper_;

	typedef void (JIT_CDECL *HaltHelper)(int error);
	HaltHelper haltHelper_;

	typedef cell (JIT_CDECL *CallHelper)(void *start);
	CallHelper callHelper_;

	typedef cell (JIT_CDECL *SysreqHelper)(cell address, cell *params);
	SysreqHelper sysreqHelper_;

	typedef std::map<cell, int> CodeMap;
	CodeMap codeMap_;

	typedef std::map<std::pair<cell, std::string>, AsmJit::Label> LabelMap;
	LabelMap labelMap_;
};

} // namespace jit

#endif // !JIT_H
