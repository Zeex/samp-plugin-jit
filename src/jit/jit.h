// Copyright (c) 2012 Zeex
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

#ifndef JIT_COMPILER_H
#define JIT_COMPILER_H

#include <cassert>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>

#include <amx/amx.h>

#include <asmjit/core.h>
#include <asmjit/x86.h>

#include "amxdisasm.h"
#include "amxscript.h"

#if !defined _M_IX86 && !defined __i386__
	#error Unsupported architecture
#endif

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

class JITCompileErrorHandler;

class JIT {
public:
	JIT(AMXScript amx);
	~JIT();

private:
	// Disallow copying and assignment.
	JIT(const JIT &);
	JIT &operator=(const JIT &);

public:
	AMXScript &amx() { return amx_; }
	const AMXScript &amx() const { return amx_; }

	// These return pointer to JIT code buffer and its size.
	void *code() const { return code_; }
	std::size_t codeSize() const { return codeSize_; }

	// Returns a pointer to a machine instruction mapped to a given AMX
	// instruction. The address is realtive to start of the AMX code.
	void *getInstrPtr(cell address) const;

	// Same as above but returns an offset to code() instead of a pointer.
	int getInstrOffset(cell address) const;

	// Sets an opcode relocation table to be used. This table only exists in the
	// GCC version of AMX where they use a GCC extension to C's goto statement
	// instead of a simple switch to minimize opcode dispatching overhead.
	void setOpcodeMap(cell *opcodeMap) { opcodeMap_ = opcodeMap; }

	// Returns a pointer to the opcode relocation table if it was previously set
	// with setOpcodeMap() or NULL if not.
	cell *opcodeMap() const { return opcodeMap_; }

	// Sets a custom assembler to be used to compile() the code. If no assembler
	// is set or setAssembler() is called with NULL argument the default
	// assembler is used.
	void setAssembler(AsmJit::X86Assembler *as) { as_ = as; }

	// Returns currently set assembler or NULL if no assembler set.
	AsmJit::X86Assembler *assembler() const { return as_; }

public:
	// compile() is used to JIT-compile the AMX script to be able to calls its
	// function with call() and exec(). If an error occurs during compilation,
	// such as invalid instruction, an optional error handler is executed.
	bool compile(JITCompileErrorHandler *errorHandler = 0);

public:
	// Calls a function at the specified address and returns one of the
	// AMX_ERROR_* codes. Function arguments should be pushed onto the AMX stack
	// prior invoking this method, for example using amx_Push*() routines.
	int call(cell address, cell *retval);

	// This is basically the same as call() but for public functions. Can be
	// used as a drop-in replacemenet for amx_Exec().
	int exec(cell index, cell *retval);

private:
	// Get addresses of all functions and jump destinations.
	bool getJumpRefs(std::set<cell> &refs) const;

	// Sets a label at the specified AMX address. The address should be relative
	// to the start of the code section.
	AsmJit::Label &L(AsmJit::X86Assembler *as, cell address);
	AsmJit::Label &L(AsmJit::X86Assembler *as, cell address, const std::string &name);

	// Sets the EBP and ESP registers to stackBase and stackPtr repectively and
	// jumps to JIT code mapped to the specified AMX address. The address must
	// be relative to the start of the of code section.
	void jump(cell address, void *stackBase, void *stackPtr);

	// A static wrapper around jump() called from within JIT code.
	static void JIT_CDECL doJump(JIT *amxjit, cell address, void *stackBase, void *stackPtr);

	// Resets EBP and ESP and jumps to the place of the previous call()
	// invocation setting amx->error to error.
	void halt(int error);

	// A wrapper around halt() that is called from JIT code.
	static void JIT_CDECL doHalt(JIT *amxjit, int error);

	// Sets the EBP and ESP registers to stackBase and stackPtr repectively and
	// executes a native functions at the specified index. If there is no
	// function at that index or the index is out of native table bounds the
	// halt() method is called.
	void sysreqC(cell index, void *stackBase, void *stackPtr);

	// A wrapper around sysreqC() that is called from JIT code.
	static void JIT_CDECL doSysreqC(JIT *amxjit, cell index, void *stackBase, void *stackPtr);

	// Same as sysreqC() but takes an address instead of an index.
	void sysreqD(cell address, void *stackBase, void *stackPtr);

	// A wrapper around sysreqD() that is called from JIT code.
	static void JIT_CDECL doSysreqD(JIT *amxjit, cell address, void *stackBase, void *stackPtr);

private:
	// An "intrinsic" (couldn't find a better term for this) is a portion of
	// machine code that substitutes calls to certain native functions and is
	// generally is a highly optimized version of such.
	typedef void (JIT::*IntrinsicImpl)(AsmJit::X86Assembler *as);

	struct Intrinsic {
		std::string   name;
		IntrinsicImpl impl;
	};

	static Intrinsic intrinsics_[];

	// AMX floating-point natives.
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

	cell *opcodeMap_;
	AsmJit::X86Assembler *as_;

	void *code_;
	std::size_t codeSize_;

	void *ebp_;
	void *esp_;
	void *resetEbp_;
	void *resetEsp_;

private:
	typedef void (JIT_CDECL *HaltHelper)(int error);
	HaltHelper haltHelper_;

	typedef void (JIT_CDECL *JumpHelper)(void *dest, void *stackBase, void *stackPtr);
	JumpHelper jumpHelper_;

	typedef cell (JIT_CDECL *CallHelper)(void *start);
	CallHelper callHelper_;

	typedef cell (JIT_CDECL *SysreqHelper)(cell address, void *stackBase, void *stackPtr);
	SysreqHelper sysreqHelper_;

private:
	typedef std::map<cell, int> PcodeToNativeMap;
	PcodeToNativeMap pcodeToNative_;

	typedef std::map<std::pair<cell, std::string>, AsmJit::Label> AddressNameToLabel;
	AddressNameToLabel addressNameToLabel_;
};

// Inherit from this class to pass your custom error handler to JIT::compile().
class JITCompileErrorHandler {
public:
	virtual ~JITCompileErrorHandler() {}
	virtual void execute(const AMXInstruction &instr) = 0;
};

} // namespace jit

#endif // !JIT_COMPILER_H
