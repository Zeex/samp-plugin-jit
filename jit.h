// Copyright (c) 2012, Sergey Zolotarev
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

#include <cstdlib>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include <AsmJit/Assembler.h>
#include <AsmJit/MemoryManager.h>
#include <AsmJit/Operand.h>

#include "amx/amx.h"

namespace jit {

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
	OP_SYMTAG,       OP_BREAK,
	NUM_AMX_OPCODES
};

class AMXInstruction {
public:
	AMXInstruction(AMXOpcode opcode, const cell *ip) : opcode_(opcode), ip_(ip) {}

	inline const cell *GetIP() const
		{ return ip_; }
	inline AMXOpcode GetOpcode() const 
		{ return opcode_; }
	inline cell GetOperand(unsigned int index = 0u) const
		{ return *(ip_ + 1 + index); }

private:
	AMXOpcode opcode_;
	const cell *ip_;
};

// Base class for JIT exceptions.
class JITError {};

class InstructionError : public JITError {
public:
	InstructionError(const AMXInstruction &instr) : instr_(instr) {}
	inline const AMXInstruction &GetInstruction() const { return instr_; }
private:
	AMXInstruction instr_;
};

class UnsupportedInstructionError : public InstructionError {
public:
	UnsupportedInstructionError(const AMXInstruction &instr) : InstructionError(instr) {}
};

class InvalidInstructionError : public InstructionError {
public:
	InvalidInstructionError(const AMXInstruction &instr) : InstructionError(instr) {}
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

inline bool operator<(const TaggedAddress &left, const TaggedAddress &right) {
	if (left.GetAddress() != right.GetAddress()) {
		return left.GetAddress() < right.GetAddress();
	}
	return left.GetTag() < right.GetTag();
}

class StackBuffer {
public:
	StackBuffer() 
		: ptr_(0)
		, top_(0)
		, size_(0)
	{}

	~StackBuffer() { Deallocate(); }

	inline void Allocate(std::size_t size) {
		if (ptr_ == 0) {
			size_ = size;
			ptr_ = std::malloc(size_);
			top_ = reinterpret_cast<char*>(ptr_) + size_ - 4;
		}
	}

	inline void Deallocate() {
		if (ptr_ != 0) {
			std::free(ptr_);
			ptr_ = top_ = 0;
			size_ = 0;
		}
	}

	inline bool IsReady() const { return ptr_ != 0; }
	inline void *GetTop() const { return top_; }
	inline void *GetPtr() const { return ptr_; }

private:
	// Disable copying.
	StackBuffer(const StackBuffer &);
	StackBuffer &operator=(const StackBuffer &);

	void *ptr_;
	void *top_;
	std::size_t size_;
};

// CodeMap maps AMX instructions to corresponding JIT code.
class CodeMap {
public:
	inline void Map(cell amx_ip, sysint_t native_ip) {
		amx_map_.insert(std::make_pair(amx_ip, native_ip));
	}
	
	inline void *GetInstrPtr(cell amx_ip, void *code_ptr) {
		sysint_t native_ip = GetInstrOffset(amx_ip);
		if (native_ip >= 0) {
			return reinterpret_cast<void*>(reinterpret_cast<sysint_t>(code_ptr) + native_ip);
		}
		return 0;
	}

	inline sysint_t GetInstrOffset(cell amx_ip) {
		AmxMapType::const_iterator iterator = amx_map_.find(amx_ip);
		if (iterator != amx_map_.end()) {
			return iterator->second;
		}
		return -1;
	}

	inline cell GetAmxInstr(sysint_t native_ip) {
		NativeMapType::const_iterator iterator = native_map_.find(native_ip);
		if (iterator != native_map_.end()) {
			return iterator->second;
		}
		return -1;
	}

private:
	// amx_ip => native_ip
	typedef std::map<cell, sysint_t> AmxMapType;
	AmxMapType amx_map_;

	// native_ip => amx_ip
	typedef std::map<sysint_t, cell> NativeMapType;
	NativeMapType native_map_;
};

class ProcMap {
public:
	~ProcMap() {
		for (MapType::iterator iterator = map_.begin(); iterator != map_.end(); ++iterator) {
			AsmJit::MemoryManager::getGlobal()->free(iterator->second);
		}
	}

	void Map(cell codestart, cell codeend, void *native_code) {
		ProcData data = {codestart, codeend};
		map_.insert(std::make_pair(data, native_code));
	}

	void *GetFunctionCode(cell fn_addr) const {
		ProcData fn_data = {fn_addr, 0};
		MapType::const_iterator iterator = map_.find(fn_data);
		if (iterator != map_.end()) {
			return iterator->second;
		}
		return 0;
	}

	cell FindFunction(cell address) const {
		for (MapType::const_iterator iterator = map_.begin(); iterator != map_.end(); ++iterator) {
			const ProcData &proc = iterator->first;
			if (address >= proc.codestart && address <= proc.codeend) {
				return proc.codestart;
			}
		}
		return 0;
	}

private:
	struct ProcData {
		cell codestart; // address of first instruction
		cell codeend;   // address of last instruction
	};

	friend bool operator<(const ProcData &left, const ProcData &right);

	typedef std::map<ProcData, void*> MapType;
	MapType map_;
};

inline bool operator<(const ProcMap::ProcData &left, const ProcMap::ProcData &right) {
	return left.codestart < right.codestart;
}

class JIT;

class JITAssembler : private AsmJit::Assembler {
public:
	JITAssembler(JIT *jit);

	// JIT-compile function at a given address.
	void *CompileFunction(cell address, CodeMap *code_map = 0);

private:
	void halt(cell code);

	// Floating point natives
	void native_float();
	void native_floatabs();
	void native_floatadd();
	void native_floatsub();
	void native_floatmul();
	void native_floatdiv();
	void native_floatsqroot();
	void native_floatlog();

	typedef void (JITAssembler::*NativeOverride)();
	std::map<std::string, NativeOverride> native_overrides_;

private:
	// Get the label mapped to given address.
	AsmJit::Label &L(cell address, const std::string &tag = "");

	typedef std::map<TaggedAddress, AsmJit::Label> LabelMap;
	LabelMap label_map_;

private:
	JIT *jit_;
};

class JIT {
	friend class JITAssembler;
public:
	// Set size of stack used by JIT code.
	static void SetStackSize(std::size_t stack_size);

	JIT(AMX *amx, cell *opcode_list = 0);
	
	// Get AMX instance associated with this JIT.
	inline AMX *GetAmx() { 
		return amx_; 
	}

	// Get pointer to AMX header.
	inline AMX_HEADER *GetAmxHeader() { 
		return amxhdr_; 
	}

	// Get pointer to AMX data segment.
	inline unsigned char *GetAmxData() {
		return data_; 
	}

	// Get pointer to AMX code segment.
	inline unsigned char *GetAmxCode() {
		return code_; 
	}

	// Get the procedure map.
	inline ProcMap &GetProcMap() {
		return proc_map_;
	}

	// Get the code map.
	inline CodeMap &GetCodeMap() {
		return code_map_;
	}

	// Turn raw AMX code into a sequence of AMXInstruction's.
	void AnalyzeFunction(cell address, std::vector<AMXInstruction> &instructions) const;

	// Get function's code (and compile if needed).
	void *GetFunction(cell address);

	// Call a function.
	void CallFunction(cell address, cell *params, cell *retval);

	// Call a public function.
	int  CallPublicFunction(int index, cell *retval);

	// Call a native function (used for SYSREQ.pri).
	cell CallNativeFunction(int index, cell *params);

private:
	// Disable copying.
	JIT(const JIT &);
	JIT &operator=(const JIT &);

private:
	AMX        *amx_;
	AMX_HEADER *amxhdr_;

	unsigned char *data_;
	unsigned char *code_;

	cell *opcode_list_;
	
	void *halt_ebp_;
	void *halt_esp_;

	ProcMap proc_map_;
	CodeMap code_map_;

	static void *esp_;
	static StackBuffer stack_;
	static int call_depth_;
};

} // namespace jit

#endif // !JIT_H
