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

#ifndef JIT_H
#define JIT_H

#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include <amx/amx.h>
#include <asmjit/X86/X86Assembler.h>

namespace jit {

// List of AMX opcodes.
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

enum AmxOpcode;

class AmxInstruction {
public:
	AmxInstruction(AmxOpcode opcode, const cell *ip) : opcode_(opcode), ip_(ip) {}

	inline const cell *GetIP() const
		{ return ip_; }
	inline AmxOpcode GetOpcode() const
		{ return opcode_; }
	inline cell GetOperand(unsigned int index = 0u) const
		{ return *(ip_ + 1 + index); }

private:
	AmxOpcode opcode_;
	const cell *ip_;
};

// Base class for JIT exceptions.
class JitError {};

class CompileError : public JitError {
public:
	CompileError(const AmxInstruction &instr) : instr_(instr) {}
	inline const AmxInstruction &GetInstruction() const { return instr_; }
private:
	AmxInstruction instr_;
};

class UnsupportedInstructionError : public CompileError {
public:
	UnsupportedInstructionError(const AmxInstruction &instr) : CompileError(instr) {}
};

class InvalidInstructionError : public CompileError {
public:
	InvalidInstructionError(const AmxInstruction &instr) : CompileError(instr) {}
};

class ObsoleteInstructionError : public CompileError {
public:
	ObsoleteInstructionError(const AmxInstruction &instr) : CompileError(instr) {}
};

class TaggedAddress {
public:
	TaggedAddress(cell address, std::string tag = std::string())
		: address_(address), tag_(tag)
	{}

	inline cell GetAddress() const { return address_; }
	inline std::string GetTag() const { return tag_; }

private:
	ucell address_;
	std::string tag_;
};

static inline bool operator<(const TaggedAddress &left, const TaggedAddress &right) {
	if (left.GetAddress() != right.GetAddress()) {
		return left.GetAddress() < right.GetAddress();
	}
	return left.GetTag() < right.GetTag();
}

class CallContext {
public:
	explicit CallContext(AMX *amx);
	~CallContext();

	inline cell *GetParams() const {
		return params_;
	}

private:
	unsigned char *GetDataPtr();
	cell *GetStackPtr();

	cell *PushStack(cell value);

	cell PopStack();
	void PopStack(int ncells);

private:
	AMX *amx_;
	cell *params_;
};

class Jitter {
public:
	Jitter(AMX *amx, cell *opcode_list = 0);
	virtual ~Jitter();

	// Get AMX instance associated with this Jitter.
	inline AMX *GetAmx() const {
		return amx_;
	}

	// Get pointer to AMX header.
	inline AMX_HEADER *GetAmxHeader() const {
		return amxhdr_;
	}

	// Get pointer to AMX data.
	inline unsigned char *GetAmxData() const {
		return amx_->data != 0 ? amx_->data : amx_->base + amxhdr_->dat;
	}

	// Get pointer to AMX code.
	inline unsigned char *GetAmxCode() const {
		return amx_->base + amxhdr_->cod;
	}

	// Get pointer to native code buffer.
	inline void *GetCode() const {
		return code_;
	}

	// Get address of native code corresponding to AMX code.
	inline void *GetInstrPtr(cell amx_ip, void *code_ptr) {
		sysint_t native_ip = GetInstrOffset(amx_ip);
		if (native_ip >= 0) {
			return reinterpret_cast<void*>(reinterpret_cast<sysint_t>(code_ptr) + native_ip);
		}
		return 0;
	}

	// Get offset to native code corresponding to AMX code.
	inline sysint_t GetInstrOffset(cell amx_ip) {
		assert(code_map_ != 0);
		if (code_map_ != 0) {
			CodeMap::const_iterator iterator = code_map_->find(amx_ip);
			if (iterator != code_map_->end()) {
				return iterator->second;
			}
		}
		return -1;
	}

	// JIT-compile whole AMX script and optionally output assembly
	// code listing to a stream.
	virtual void Compile(std::FILE *list_stream = 0);

	// Unconditional jump to the specified AMX address.
	virtual void Jump(cell ip, void *stack_ptr);

	// Call a public function.
	virtual int CallPublicFunction(int index, cell *retval);

private:
	// Turn raw AMX code into a sequence of AmxInstruction's.
	void ParseCode(cell start, cell end, std::vector<AmxInstruction> &instructions) const;

	// Call a function.
	// Must not be called for non-public functions.
	int CallFunction(cell address, cell *params, cell *retval);

private:
	Jitter(const Jitter &);
	Jitter &operator=(const Jitter &);

private:
	AMX        *amx_;
	AMX_HEADER *amxhdr_;

	cell *opcode_list_;

	bool compiled_;
	void *code_;

	void *halt_ebp_;
	void *halt_esp_;

	typedef std::map<cell, sysint_t> CodeMap;
	CodeMap *code_map_;

	typedef std::map<TaggedAddress, AsmJit::Label> LabelMap;
	LabelMap *label_map_;

	AsmJit::Label &Label(AsmJit::X86Assembler &as,
	                     LabelMap *label_map,
	                     cell address,
	                     const std::string &name = std::string());

protected: // native overrides
	typedef void (Jitter::*NativeOverride)(AsmJit::X86Assembler &as);
	std::map<std::string, NativeOverride> native_overrides_;

	void native_float(AsmJit::X86Assembler &as);
	void native_floatabs(AsmJit::X86Assembler &as);
	void native_floatadd(AsmJit::X86Assembler &as);
	void native_floatsub(AsmJit::X86Assembler &as);
	void native_floatmul(AsmJit::X86Assembler &as);
	void native_floatdiv(AsmJit::X86Assembler &as);
	void native_floatsqroot(AsmJit::X86Assembler &as);
	void native_floatlog(AsmJit::X86Assembler &as);

protected: // code snippets
	// Halt current function (jump to the point of call).
	// Affects registers: EBP, ESP.
	void halt(AsmJit::X86Assembler &as, cell error_code);

	// Save stk/frm and switch to real stack.
	// Affects registers: EDX, EBP, ESP.
	void begin_alien_code(AsmJit::X86Assembler &as);

	// Save ebp_/esp_ and switch to AMX stack.
	// Affects registers: EDX, EBP, ESP.
	void end_alien_code(AsmJit::X86Assembler &as);

private: // static members
	static void *ebp_;
	static void *esp_;
};

} // namespace jit

#endif // !JIT_H
