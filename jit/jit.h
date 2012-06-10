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
#else
	#error Unsupported compiler
#endif

namespace jit {

// REG_NONE is defined in WinNT.h
#ifdef REG_NONE
	#undef REG_NONE
#endif

enum AMXRegister {
	REG_NONE = 0,
	REG_PRI = (2 << 0),
	REG_ALT = (2 << 1),
	REG_COD = (2 << 2),
	REG_DAT = (2 << 3),
	REG_HEA = (2 << 4),
	REG_STP = (2 << 5),
	REG_STK = (2 << 6),
	REG_FRM = (2 << 7),
	REG_CIP = (2 << 8),
};

enum AMXOpcode {
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
	OP_SYMTAG,       OP_BREAK
};

// The total number of opcodes.
const int NUM_AMX_OPCODES = OP_BREAK + 1;

class AMXInstruction {
public:
	AMXInstruction();

public:
	inline int size() const {
		return sizeof(cell) * (1 + operands_.size());
	}

	inline const cell address() const {
		return address_;
	}
	inline void setAddress(cell address) {
		address_ = address;
	}

	inline AMXOpcode opcode() const {
		return opcode_;
	}
	inline void setOpcode(AMXOpcode opcode) {
		opcode_ = opcode;
	}

	inline cell operand(unsigned int index = 0u) {
		assert(index >= 0 && index < operands_.size());
		return operands_[index];
	}

	inline std::vector<cell> &operands() {
		return operands_;
	}
	inline const std::vector<cell> &operands() const {
		return operands_;
	}
	inline void setOperands(std::vector<cell> operands) {
		operands_ = operands;
	}

	inline void addOperand(cell value) {
		operands_.push_back(value);
	}

	inline int numOperands() const {
		return operands_.size();
	}

public:
	const char *name() const;

	int inputRegisters() const;
	int outputRegisters() const;

	std::string string() const;

private:
	cell address_;
	AMXOpcode opcode_;
	std::vector<cell> operands_;

private:
	struct StaticInfoTableEntry {
		const char *name;
		int inputRegisters;
		int outputRegisters;
	};
	static const StaticInfoTableEntry info[NUM_AMX_OPCODES];
};

class AMXScript {
public:
	AMXScript(AMX *amx);

public:
	inline AMX *amx() const {
		return amx_;
	}
	inline AMX_HEADER *amxHeader() const {
		return reinterpret_cast<AMX_HEADER*>(amx_->base);
	}

	inline unsigned char *code() const {
		return amx_->base + amxHeader()->cod;
	}
	inline std::size_t codeSize() const {
		return amxHeader()->dat - amxHeader()->cod;
	}

	inline unsigned char *data() const {
		return amx_->data != 0 ? amx_->data : amx_->base + amxHeader()->dat;
	}
	inline std::size_t dataSize() const {
		return amxHeader()->hea - amxHeader()->dat;
	}

	inline int numPublics() const {
		return (amxHeader()->natives - amxHeader()->publics) / amxHeader()->defsize;
	}
	inline int numNatives() const {
		return (amxHeader()->libraries - amxHeader()->natives) / amxHeader()->defsize;
	}

	inline AMX_FUNCSTUBNT *publics() const {
		return reinterpret_cast<AMX_FUNCSTUBNT*>(amxHeader()->publics + amx_->base);
	}
	inline AMX_FUNCSTUBNT *natives() const {
		return reinterpret_cast<AMX_FUNCSTUBNT*>(amxHeader()->natives + amx_->base);
	}

public:
	inline operator AMX*() {
		return amx();
	}
	inline AMX *operator->() {
		return amx();
	}

public:
	cell getPublicAddress(int index) const;
	cell getNativeAddress(int index) const;

	int getPublicIndex(cell address) const;
	int getNativeIndex(cell address) const;

	const char *getPublicName(int index) const;
	const char *getNativeName(int index) const;

public:
	inline cell *stack() const {
		return reinterpret_cast<cell*>(data() + amx_->stk);
	}
	inline cell stackSize() const {
		return amxHeader()->stp - amxHeader()->hea;
	}

	cell *push(cell value);
	cell  pop();
	void  pop(int ncells);

private:
	AMX *amx_;
};

class AMXDisassembler {
public:
	AMXDisassembler(const AMXScript &amx);

public:
	inline void setOpcodeTable(cell *opcodeTable) {
		opcodeTable_ = opcodeTable;
	}

	inline cell ip() const {
		return ip_;
	}
	inline void setIp(cell ip) {
		ip_ = ip;
	}

public:
	bool decode(AMXInstruction &instr, bool *error = 0);

private:
	AMXScript amx_;
	cell *opcodeTable_;
	cell ip_;
};

class JIT {
public:
	JIT(AMXScript amx);
	~JIT();

private:
	JIT(const JIT &);
	JIT &operator=(const JIT &);

public:
	inline void *code() const {
		return code_;
	}
	inline std::size_t codeSize() const {
		return codeSize_;
	}

	void *getInstrPtr(cell address) const;
	int   getInstrOffset(cell address) const;

	inline void setOpcodeTable(cell *opcodeTable) {
		opcodeTable_ = opcodeTable;
	}
	inline cell *opcodeTable() const {
		return opcodeTable_;
	}

	// Sets an assembler to be used by compile().
	inline void setAssembler(AsmJit::X86Assembler *assembler) {
		assembler_ = assembler;
	}
	inline AsmJit::X86Assembler *assembler() const {
		return assembler_;
	}

public:
	typedef void (*CompileErrorHandler)(
		const AMXScript &amx,
		const AMXInstruction &instr
	);
	bool compile(CompileErrorHandler errorHandler = 0);

public:
	int call(cell address, cell *retval);
	int exec(cell index, cell *retval);

private:
	// Get code locations referred to by JUMP/JCC/CALL instructions.
	bool getJumpRefs(std::set<cell> &refs) const;

	// See if we can safely write to a register because it is overwritten any way by
	// following instructions.
	bool canOverwriteRegister(cell address, AMXRegister reg) const;

	// Sets a label at the specified address. Used for complex instructions involving
	// conditional jumps e.g. SWITCH, BOUNDS, etc.
	AsmJit::Label &L(AsmJit::X86Assembler *as, cell address);
	AsmJit::Label &L(AsmJit::X86Assembler *as, cell address, const std::string &name);

	void jump(cell address, void *stackBase, void *stackPtr);
	static void JIT_CDECL doJump(JIT *jit, cell address, void *stackBase, void *stackPtr);

	void halt(int error);
	static void JIT_CDECL doHalt(JIT *jit, int error);

	void sysreqC(cell index, void *stackBase, void *stackPtr);
	static void JIT_CDECL doSysreqC(JIT *jit, cell index, void *stackBase, void *stackPtr);

	void sysreqD(cell address, void *stackBase, void *stackPtr);
	static void JIT_CDECL doSysreqD(JIT *jit, cell address, void *stackBase, void *stackPtr);

private:
	typedef void (JIT::*IntrinsicImpl)(AsmJit::X86Assembler *as);

	struct Intrinsic {
		std::string   name;
		IntrinsicImpl impl;
	};

	static Intrinsic intrinsics_[];

	// Optimized version of AMX floating-point library natives.
	void native_float(AsmJit::X86Assembler *as);
	void native_floatabs(AsmJit::X86Assembler *as);
	void native_floatadd(AsmJit::X86Assembler *as);
	void native_floatsub(AsmJit::X86Assembler *as);
	void native_floatmul(AsmJit::X86Assembler *as);
	void native_floatdiv(AsmJit::X86Assembler *as);
	void native_floatsqroot(AsmJit::X86Assembler *as);
	void native_floatlog(AsmJit::X86Assembler *as);

private:
	AMXScript amx_;

	// This is the opcode relocation table. It exists only in GCC version of AMX,
	// consult the AMX source code for details (hint: lookup "opcode_list" variable).
	cell *opcodeTable_;

	AsmJit::X86Assembler *assembler_;

	void        *code_;
	std::size_t  codeSize_;

	void *ebp_;
	void *esp_;

	void *haltEbp_;
	void *haltEsp_;

private:
	AsmJit::Label L_halt_;

private:
	typedef void (JIT_CDECL *HaltHelper)(int error);
	HaltHelper haltHelper_;

	typedef void (JIT_CDECL *JumpHelper)(void *dest, void *stackBase, void *stackPtr);
	JumpHelper jumpHelper_;

	typedef cell (JIT_CDECL *CallHelper)(void *start);
	CallHelper callHelper_;

	typedef cell (JIT_CDECL *SysreqHelper)(cell address, void *stackBase, void *stackPtr);
	SysreqHelper sysreqHelper_;

	typedef std::map<cell, int> CodeMap;
	CodeMap codeMap_;

	typedef std::map<std::pair<cell, std::string>, AsmJit::Label> LabelMap;
	LabelMap labelMap_;
};

} // namespace jit

#endif // !JIT_H
