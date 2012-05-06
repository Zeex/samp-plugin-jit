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

#if defined _WIN32 || defined WIN32 || defined __WIN32__
	#define OS_WIN32
#elif defined __linux__ || defined linux || defined __linux
	#define OS_LINUX
#endif

#if defined _MSC_VER
	#define COMPILER_MSVC
#elif defined __GNUC__
	#define COMPILER_GCC
#else
	#error Unsupported compiler
#endif

#if defined COMPILER_MSVC
	#if !defined CDECL
		#define CDECL __cdecl
	#endif
	#if !defined STDCALL
		#define STDCALL __stdcall
	#endif
#elif defined COMPILER_GCC
	#if !defined CDECL
		#define CDECL __attribute__((cdecl))
	#endif
	#if !defined STDCALL
		#define STDCALL __attribute__((stdcall))
	#endif
#endif

namespace jit {

CallContext::CallContext(AMX *amx)
	: amx_(amx)
	, params_(0)
{
	params_ = PushStack(amx_->paramcount * sizeof(cell));
	amx_->paramcount = 0;
}

CallContext::~CallContext() {
	int paramcount = PopStack();
	PopStack(paramcount / sizeof(cell));
}

unsigned char *CallContext::GetDataPtr() {
	unsigned char *data = amx_->data;
	if (data == 0) {
		AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx_->base);
		data = amx_->base + hdr->dat;
	}
	return data;
}

cell *CallContext::GetStackPtr() {
	return reinterpret_cast<cell*>(GetDataPtr() + amx_->stk);
}

cell *CallContext::PushStack(cell value) {
	amx_->stk -= sizeof(cell);
	cell *stack = GetStackPtr();
	*stack = value;
	return stack;
}

cell CallContext::PopStack() {
	cell *stack = GetStackPtr();
	amx_->stk += sizeof(cell);
	return *stack;
}

void CallContext::PopStack(int ncells) {
	amx_->stk += ncells * sizeof(cell);
}

} // namespace jit

#define OVERRIDE_NATIVE(name) \
	do { native_overrides_[#name] = &Jitter::native_##name; } while (false);

namespace {

cell GetPublicAddress(AMX *amx, cell index) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

	AMX_FUNCSTUBNT *publics = reinterpret_cast<AMX_FUNCSTUBNT*>(amx->base + hdr->publics);
	int num_publics = (hdr->natives - hdr->publics) / hdr->defsize;

	if (index == AMX_EXEC_MAIN) {
		return hdr->cip;
	}
	if (index < 0 || index >= num_publics) {
		return 0;
	}
	return publics[index].address;
}

int GetPublicIndex(AMX *amx, cell address) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

	AMX_FUNCSTUBNT *publics = reinterpret_cast<AMX_FUNCSTUBNT*>(amx->base + hdr->publics);
	int num_publics = (hdr->natives - hdr->publics) / hdr->defsize;

	if (address == hdr->cip) {
		return AMX_EXEC_MAIN;
	}
	for (int i = 0; i < num_publics; i++) {
		if (publics[i].address == address) {
			return i;
		}
	}
	return -1;
}

inline bool IsPublic(AMX *amx, cell address) {
	int index = GetPublicIndex(amx, address);
	return index >= 0 || index == AMX_EXEC_MAIN;
}

cell GetNativeAddress(AMX *amx, int index) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

	AMX_FUNCSTUBNT *natives = reinterpret_cast<AMX_FUNCSTUBNT*>(amx->base + hdr->natives);
	int num_natives = (hdr->libraries - hdr->natives) / hdr->defsize;

	if (index < 0 || index >= num_natives) {
		return 0;
	}
	return natives[index].address;
}

const char *GetNativeName(AMX *amx, int index) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

	AMX_FUNCSTUBNT *natives = reinterpret_cast<AMX_FUNCSTUBNT*>(amx->base + hdr->natives);
	int num_natives = (hdr->libraries - hdr->natives) / hdr->defsize;

	if (index < 0 || index >= num_natives) {
		return 0;
	}
	return reinterpret_cast<char*>(amx->base + natives[index].nameofs);
}

int GetNativeIndex(AMX *amx, cell address) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

	AMX_FUNCSTUBNT *natives = reinterpret_cast<AMX_FUNCSTUBNT*>(amx->base + hdr->natives);
	int num_natives = (hdr->libraries - hdr->natives) / hdr->defsize;

	for (int i = 0; i < num_natives; i++) {
		if (natives[i].address == address) {
			return i;
		}
	}
	return -1;
}

void STDCALL Jump(jit::Jitter *jitter, cell ip, void *stack_ptr) {
	jitter->Jump(ip, stack_ptr);
}

} // unnamed namespace

namespace jit {

// Memory addressing
using AsmJit::byte_ptr;
using AsmJit::word_ptr;
using AsmJit::dword_ptr;
using AsmJit::dword_ptr_abs;

// X86 registers
using AsmJit::eax;
using AsmJit::ecx;
using AsmJit::edx;
using AsmJit::esi;
using AsmJit::edi;
using AsmJit::ebp;
using AsmJit::esp;
using AsmJit::ax;
using AsmJit::al;
using AsmJit::cl;

// FPU registers
using AsmJit::st;

Jitter::Jitter(AMX *amx, cell *opcode_list)
	: amx_(amx)
	, amxhdr_(reinterpret_cast<AMX_HEADER*>(amx->base))
	, opcode_list_(opcode_list)
	, compiled_(false)
	, halt_esp_(0)
	, halt_ebp_(0)
	, code_map_(0)
	, label_map_(0)
	, code_(0)
{
	OVERRIDE_NATIVE(float);
	OVERRIDE_NATIVE(floatabs);
	OVERRIDE_NATIVE(floatadd);
	OVERRIDE_NATIVE(floatsub);
	OVERRIDE_NATIVE(floatmul);
	OVERRIDE_NATIVE(floatdiv);
	OVERRIDE_NATIVE(floatsqroot);
	OVERRIDE_NATIVE(floatlog);
}

void Jitter::Compile(std::FILE *list_stream) {
	if (compiled_) {
		return;
	}

	std::vector<AmxInstruction> instrs;
	ParseCode(0, amxhdr_->dat - amxhdr_->cod, instrs);

	AsmJit::X86Assembler as;
	AsmJit::FileLogger logger(list_stream);
	as.setLogger(&logger);

	std::auto_ptr<CodeMap> code_map(new CodeMap);
	std::auto_ptr<LabelMap> label_map(new LabelMap);

	cell current_function = 0;

	for (std::vector<AmxInstruction>::iterator instr_iterator = instrs.begin();
			instr_iterator != instrs.end(); ++instr_iterator)
	{
		AmxInstruction &instr = *instr_iterator;

		cell cip = reinterpret_cast<cell>(instr.GetIP())
		         - reinterpret_cast<cell>(GetAmxCode());
		as.bind(Label(as, label_map.get(), cip));

		code_map->insert(std::make_pair(cip, as.getCodeSize()));

		switch (instr.GetOpcode()) {
		case OP_LOAD_PRI: // address
			// PRI = [address]
			as.mov(eax, dword_ptr_abs(reinterpret_cast<void*>(GetAmxData() + instr.GetOperand())));
			break;
		case OP_LOAD_ALT: // address
			// PRI = [address]
			as.mov(ecx, dword_ptr_abs(reinterpret_cast<void*>(GetAmxData() + instr.GetOperand())));
			break;
		case OP_LOAD_S_PRI: // offset
			// PRI = [FRM + offset]
			as.mov(eax, dword_ptr(ebp, instr.GetOperand()));
			break;
		case OP_LOAD_S_ALT: // offset
			// ALT = [FRM + offset]
			as.mov(ecx, dword_ptr(ebp, instr.GetOperand()));
			break;
		case OP_LREF_PRI: // address
			// PRI = [ [address] ]
			as.mov(edx, dword_ptr_abs(reinterpret_cast<void*>(GetAmxData() + instr.GetOperand())));
			as.mov(eax, dword_ptr(edx, reinterpret_cast<sysint_t>(GetAmxData())));
			break;
		case OP_LREF_ALT: // address
			// ALT = [ [address] ]
			as.mov(edx, dword_ptr_abs(reinterpret_cast<void*>(GetAmxData() + instr.GetOperand())));
			as.mov(ecx, dword_ptr(edx, reinterpret_cast<sysint_t>(GetAmxData())));
			break;
		case OP_LREF_S_PRI: // offset
			// PRI = [ [FRM + offset] ]
			as.mov(edx, dword_ptr(ebp, instr.GetOperand()));
			as.mov(eax, dword_ptr(edx, reinterpret_cast<sysint_t>(GetAmxData())));
			break;
		case OP_LREF_S_ALT: // offset
			// PRI = [ [FRM + offset] ]
			as.mov(edx, dword_ptr(ebp, instr.GetOperand()));
			as.mov(ecx, dword_ptr(edx, reinterpret_cast<sysint_t>(GetAmxData())));
			break;
		case OP_LOAD_I:
			// PRI = [PRI] (full cell)
			as.mov(eax, dword_ptr(eax, reinterpret_cast<sysint_t>(GetAmxData())));
			break;
		case OP_LODB_I: // number
			// PRI = "number" bytes from [PRI] (read 1/2/4 bytes)
			switch (instr.GetOperand()) {
			case 1:
				as.movzx(eax, byte_ptr(eax, reinterpret_cast<sysint_t>(GetAmxData())));
				break;
			case 2:
				as.movzx(eax, word_ptr(eax, reinterpret_cast<sysint_t>(GetAmxData())));
				break;
			case 4:
				as.mov(eax, dword_ptr(eax, reinterpret_cast<sysint_t>(GetAmxData())));
				break;
			}
			break;
		case OP_CONST_PRI: // value
			// PRI = value
			as.mov(eax, instr.GetOperand());
			break;
		case OP_CONST_ALT: // value
			// ALT = value
			as.mov(ecx, instr.GetOperand());
			break;
		case OP_ADDR_PRI: // offset
			// PRI = FRM + offset
			as.lea(eax, dword_ptr(ebp, instr.GetOperand() - reinterpret_cast<sysint_t>(GetAmxData())));
			break;
		case OP_ADDR_ALT: // offset
			// ALT = FRM + offset
			as.lea(ecx, dword_ptr(ebp, instr.GetOperand() - reinterpret_cast<sysint_t>(GetAmxData())));
			break;
		case OP_STOR_PRI: // address
			// [address] = PRI
			as.mov(dword_ptr_abs(reinterpret_cast<void*>(GetAmxData() + instr.GetOperand())), eax);
			break;
		case OP_STOR_ALT: // address
			// [address] = ALT
			as.mov(dword_ptr_abs(reinterpret_cast<void*>(GetAmxData() + instr.GetOperand())), ecx);
			break;
		case OP_STOR_S_PRI: // offset
			// [FRM + offset] = ALT
			as.mov(dword_ptr(ebp, instr.GetOperand()), eax);
			break;
		case OP_STOR_S_ALT: // offset
			// [FRM + offset] = ALT
			as.mov(dword_ptr(ebp, instr.GetOperand()), ecx);
			break;
		case OP_SREF_PRI: // address
			// [ [address] ] = PRI
			as.mov(edx, dword_ptr_abs(reinterpret_cast<void*>(GetAmxData() + instr.GetOperand())));
			as.mov(dword_ptr(edx, reinterpret_cast<sysint_t>(GetAmxData())), eax);
			break;
		case OP_SREF_ALT: // address
			// [ [address] ] = ALT
			as.mov(edx, dword_ptr_abs(reinterpret_cast<void*>(GetAmxData() + instr.GetOperand())));
			as.mov(dword_ptr(edx, reinterpret_cast<sysint_t>(GetAmxData())), ecx);
			break;
		case OP_SREF_S_PRI: // offset
			// [ [FRM + offset] ] = PRI
			as.mov(edx, dword_ptr(ebp, instr.GetOperand()));
			as.mov(dword_ptr(edx, reinterpret_cast<sysint_t>(GetAmxData())), eax);
			break;
		case OP_SREF_S_ALT: // offset
			// [ [FRM + offset] ] = ALT
			as.mov(edx, dword_ptr(ebp, instr.GetOperand()));
			as.mov(dword_ptr(edx, reinterpret_cast<sysint_t>(GetAmxData())), ecx);
			break;
		case OP_STOR_I:
			// [ALT] = PRI (full cell)
			as.mov(dword_ptr(ecx, reinterpret_cast<sysint_t>(GetAmxData())), eax);
			break;
		case OP_STRB_I: // number
			// "number" bytes at [ALT] = PRI (write 1/2/4 bytes)
			switch (instr.GetOperand()) {
			case 1:
				as.mov(byte_ptr(ecx, reinterpret_cast<sysint_t>(GetAmxData())), al);
				break;
			case 2:
				as.mov(word_ptr(ecx, reinterpret_cast<sysint_t>(GetAmxData())), ax);
				break;
			case 4:
				as.mov(dword_ptr(ecx, reinterpret_cast<sysint_t>(GetAmxData())), eax);
				break;
			}
			break;
		case OP_LIDX:
			// PRI = [ ALT + (PRI x cell size) ]
			as.mov(eax, dword_ptr(ecx, eax, 2, reinterpret_cast<sysint_t>(GetAmxData())));
			break;
		case OP_LIDX_B: // shift
			// PRI = [ ALT + (PRI << shift) ]
			as.mov(eax, dword_ptr(ecx, eax, instr.GetOperand(), reinterpret_cast<sysint_t>(GetAmxData())));
			break;
		case OP_IDXADDR:
			// PRI = ALT + (PRI x cell size) (calculate indexed address)
			as.lea(eax, dword_ptr(ecx, eax, 2));
			break;
		case OP_IDXADDR_B: // shift
			// PRI = ALT + (PRI << shift) (calculate indexed address)
			as.lea(eax, dword_ptr(ecx, eax, instr.GetOperand()));
			break;
		case OP_ALIGN_PRI: // number
			// Little Endian: PRI ^= cell size - number
			#if BYTE_ORDER == LITTLE_ENDIAN
				if (instr.GetOperand() < sizeof(cell)) {
					as.xor_(eax, sizeof(cell) - instr.GetOperand());
				}
			#endif
			break;
		case OP_ALIGN_ALT: // number
			// Little Endian: ALT ^= cell size - number
			#if BYTE_ORDER == LITTLE_ENDIAN
				if (instr.GetOperand() < sizeof(cell)) {
					as.xor_(ecx, sizeof(cell) - instr.GetOperand());
				}
			#endif
			break;
		case OP_LCTRL: // index
			// PRI is set to the current value of any of the special registers.
			// The index parameter must be: 0=COD, 1=DAT, 2=HEA,
			// 3=STP, 4=STK, 5=FRM, 6=CIP (of the next instruction)
			switch (instr.GetOperand()) {
			case 0:
				as.mov(eax, dword_ptr_abs(reinterpret_cast<void*>(&amxhdr_->cod)));
				break;
			case 1:
				as.mov(eax, dword_ptr_abs(reinterpret_cast<void*>(&amxhdr_->dat)));
				break;
			case 2:
				as.mov(eax, dword_ptr_abs(reinterpret_cast<void*>(&amx_->hea)));
				break;
			case 4:
				as.lea(eax, dword_ptr(esp, -reinterpret_cast<sysint_t>(GetAmxData())));
				break;
			case 5:
				as.lea(eax, dword_ptr(ebp, -reinterpret_cast<sysint_t>(GetAmxData())));
				break;
			case 6: {
				if (instr_iterator == instrs.end() - 1) {
					// Can't get address of next instruction since this one is the last.
					throw InvalidInstructionError(instr);
				}
				AmxInstruction &next_instr = *(instr_iterator + 1);
				as.mov(eax, reinterpret_cast<sysint_t>(next_instr.GetIP())
				            - reinterpret_cast<sysint_t>(GetAmxCode()));
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
			switch (instr.GetOperand()) {
			case 2:
				as.mov(dword_ptr_abs(reinterpret_cast<void*>(&amx_->hea)), eax);
				break;
			case 4:
				as.lea(esp, dword_ptr(eax, reinterpret_cast<sysint_t>(GetAmxData())));
				break;
			case 5:
				as.lea(ebp, dword_ptr(eax, reinterpret_cast<sysint_t>(GetAmxData())));
				break;
			case 6:
				as.push(esp);
				as.push(eax);
				as.push(reinterpret_cast<sysint_t>(this));
				as.call(reinterpret_cast<void*>(::Jump));
				// Didn't jump because of invalid address - exit with error.
				halt(as, AMX_ERR_INVINSTR);
				break;
			default:
				throw UnsupportedInstructionError(instr);
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
			// Exchange PRI and_ ALT
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
			as.push(instr.GetOperand());
			break;
		case OP_PUSH: // address
			// [STK] = [address], STK = STK - cell size
			as.push(dword_ptr_abs(reinterpret_cast<void*>(instr.GetOperand() + GetAmxData())));
			break;
		case OP_PUSH_S: // offset
			// [STK] = [FRM + offset], STK = STK - cell size
			as.push(dword_ptr(ebp, instr.GetOperand()));
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
			as.lea(ecx, dword_ptr(esp, -reinterpret_cast<sysint_t>(GetAmxData())));
			as.add(esp, instr.GetOperand());
			break;
		case OP_HEAP: // value
			// ALT = HEA, HEA = HEA + value
			as.mov(ecx, dword_ptr_abs(reinterpret_cast<void*>(&amx_->hea)));
			as.add(dword_ptr_abs(reinterpret_cast<void*>(&amx_->hea)), instr.GetOperand());
			break;
		case OP_PROC:
			// [STK] = FRM, STK = STK - cell size, FRM = STK
			as.lea(edx, dword_ptr(ebp, -reinterpret_cast<sysint_t>(GetAmxData())));
			as.push(edx);
			as.mov(ebp, esp);
			current_function = cip;
			break;
		case OP_RET:
		case OP_RETN:
			// STK = STK + cell size, FRM = [STK],
			// CIP = [STK], STK = STK + cell size
			// The RETN instruction removes a specified number of bytes
			// from the stack. The value to adjust STK with must be
			// pushed prior to the call.
			as.pop(ebp);
			as.add(ebp, reinterpret_cast<sysint_t>(GetAmxData()));
			if (IsPublic(amx_, current_function)) {
				// Publics must sync stk with esp before exiting.
				as.lea(edx, dword_ptr(esp, -reinterpret_cast<sysint_t>(GetAmxData()) + 4));
				as.mov(dword_ptr_abs(&amx_->stk), edx);
			}
			as.ret();
			break;
		case OP_CALL: { // offset
			// [STK] = CIP + 5, STK = STK - cell size
			// CIP = CIP + offset
			// The CALL instruction jumps to an address after storing the
			// address of the next sequential instruction on the stack.
			// The address jumped to is relative to the current CIP,
			// but the address on the stack is an absolute address.
			cell fn_addr = instr.GetOperand() - reinterpret_cast<cell>(GetAmxCode());
			as.call(Label(as, label_map.get(), fn_addr));
			as.add(esp, dword_ptr(esp));
			as.add(esp, 4);
			break;
		}
		case OP_CALL_PRI:
			// Same as CALL, the address to jump to is in PRI.
			as.call(eax);
			as.add(esp, dword_ptr(esp));
			as.add(esp, 4);
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
			cell dest = instr.GetOperand() - reinterpret_cast<cell>(GetAmxCode());
			AsmJit::Label &L_dest = Label(as, label_map.get(), dest);

			switch (instr.GetOpcode()) {
				case OP_JUMP: // offset
					// CIP = CIP + offset (jump to the address relative from
					// the current position)
					as.jmp(L_dest);
					break;
				case OP_JUMP_PRI:
					// CIP = PRI (indirect jump)
					as.push(esp);
					as.push(eax);
					as.push(reinterpret_cast<sysint_t>(this));
					as.call(reinterpret_cast<void*>(::Jump));
					// Didn't jump because of invalid address - exit with error.
					halt(as, AMX_ERR_INVINSTR);
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
			as.shl(eax, static_cast<unsigned char>(instr.GetOperand()));
			break;
		case OP_SHL_C_ALT: // value
			// ALT = ALT << value
			as.shl(ecx, static_cast<unsigned char>(instr.GetOperand()));
			break;
		case OP_SHR_C_PRI: // value
			// PRI = PRI >> value (without sign extension)
			as.shr(eax, static_cast<unsigned char>(instr.GetOperand()));
			break;
		case OP_SHR_C_ALT: // value
			// PRI = PRI >> value (without sign extension)
			as.shl(ecx, static_cast<unsigned char>(instr.GetOperand()));
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
			// or_:
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
			as.setz(cl);
			as.movzx(eax, cl);
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
			as.add(eax, instr.GetOperand());
			break;
		case OP_SMUL_C: // value
			// PRI = PRI * value
			as.imul(eax, instr.GetOperand());
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
			as.mov(dword_ptr_abs(reinterpret_cast<void*>(instr.GetOperand() + GetAmxData())), 0);
			break;
		case OP_ZERO_S: // offset
			// [FRM + offset] = 0
			as.mov(dword_ptr(ebp, instr.GetOperand()), 0);
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
			as.cmp(eax, instr.GetOperand());
			as.sete(al);
			as.movzx(eax, al);
			break;
		case OP_EQ_C_ALT: // value
			// PRI = ALT == value ? 1 : 0
			as.cmp(ecx, instr.GetOperand());
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
			as.inc(dword_ptr_abs(reinterpret_cast<void*>(GetAmxData() + instr.GetOperand())));
			break;
		case OP_INC_S: // offset
			// [FRM + offset] = [FRM + offset] + 1
			as.inc(dword_ptr(ebp, instr.GetOperand()));
			break;
		case OP_INC_I:
			// [PRI] = [PRI] + 1
			as.inc(dword_ptr(eax, reinterpret_cast<sysint_t>(GetAmxData())));
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
			as.dec(dword_ptr_abs(reinterpret_cast<void*>(GetAmxData() + instr.GetOperand())));
			break;
		case OP_DEC_S: // offset
			// [FRM + offset] = [FRM + offset] - 1
			as.dec(dword_ptr(ebp, instr.GetOperand()));
			break;
		case OP_DEC_I:
			// [PRI] = [PRI] - 1
			as.dec(dword_ptr(eax, reinterpret_cast<sysint_t>(GetAmxData())));
			break;
		case OP_MOVS: // number
			// Copy memory from [PRI] to [ALT]. The parameter
			// specifies the number of bytes. The blocks should not
			// overlap.
			as.lea(esi, dword_ptr(eax, reinterpret_cast<sysint_t>(GetAmxData())));
			as.lea(edi, dword_ptr(ecx, reinterpret_cast<sysint_t>(GetAmxData())));
			as.push(ecx);
			if (instr.GetOperand() % 4 == 0) {
				as.mov(ecx, instr.GetOperand() / 4);
				as.rep_movsd();
			} else if (instr.GetOperand() % 2 == 0) {
				as.mov(ecx, instr.GetOperand() / 2);
				as.rep_movsw();
			} else {
				as.mov(ecx, instr.GetOperand());
				as.rep_movsb();
			}
			as.pop(ecx);
			break;
		case OP_CMPS: // number
			// Compare memory blocks at [PRI] and_ [ALT]. The parameter
			// specifies the number of bytes. The blocks should not
			// overlap.
			as.push(instr.GetOperand());
			as.lea(edx, dword_ptr(ecx, reinterpret_cast<sysint_t>(GetAmxData())));
			as.push(edx);
			as.lea(edx, dword_ptr(eax, reinterpret_cast<sysint_t>(GetAmxData())));
			as.push(edx);
			as.call(reinterpret_cast<void*>(std::memcmp));
			as.call(edx);
			as.add(esp, 12);
			break;
		case OP_FILL: { // number
			// Fill memory at [ALT] with value in [PRI]. The parameter
			// specifies the number of bytes, which must be a multiple
			// of the cell size.
			AsmJit::Label &L_loop = Label(as, label_map.get(), cip, "loop");
			as.lea(edi, dword_ptr(ecx, reinterpret_cast<sysint_t>(GetAmxData())));                      // memory start
			as.lea(esi, dword_ptr(ecx, reinterpret_cast<sysint_t>(GetAmxData()) + instr.GetOperand())); // memory end
			as.bind(L_loop);
				as.mov(dword_ptr(edi), eax);
				as.add(edi, sizeof(cell));
				as.cmp(edi, esi);
			as.jl(L_loop); // if edi < esi fill next cell
			break;
		}
		case OP_HALT: // number
			// Abort execution (exit value in PRI), parameters other than 0
			// have a special meaning.
			halt(as, instr.GetOperand());
			break;
		case OP_BOUNDS: { // value
			// Abort execution if PRI > value or_ if PRI < 0.
			AsmJit::Label &L_halt = Label(as, label_map.get(), cip, "halt");
			AsmJit::Label &L_good = Label(as, label_map.get(), cip, "good");
				as.cmp(eax, instr.GetOperand());
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
			AsmJit::Label &L_halt = Label(as, label_map.get(), cip, "halt");
				as.push(eax);
				as.push(reinterpret_cast<sysint_t>(amx_));
				as.call(reinterpret_cast<void*>(GetNativeAddress));
				as.add(esp, 8);
				as.test(eax, eax);
				as.jz(L_halt);
				as.mov(edi, esp);
				begin_alien_code(as);
					as.push(edi);
					as.push(reinterpret_cast<sysint_t>(amx_));
					as.call(eax);
					as.add(esp, 8);
				end_alien_code(as);
			as.bind(L_halt);
				halt(as, AMX_ERR_NOTFOUND);
			break;
		}
		case OP_SYSREQ_C:   // index
		case OP_SYSREQ_D: { // address
			// call system service
			std::string native_name;
			switch (instr.GetOpcode()) {
				case OP_SYSREQ_C:
					native_name = GetNativeName(amx_, instr.GetOperand());
					break;
				case OP_SYSREQ_D: {
					int index = GetNativeIndex(amx_, instr.GetOperand());
					if (index != -1) {
						native_name = GetNativeName(amx_, index);
					}
					break;
				}
			}
			// Replace calls to various natives with their optimized equivalents.
			std::map<std::string, NativeOverride>::const_iterator it
					= native_overrides_.find(native_name);
			if (it != native_overrides_.end()) {
				(*this.*(it->second))(as);
				goto special_native;
			} else {
				goto ordinary_native;
			}
		ordinary_native:
			as.mov(edi, esp);
			begin_alien_code(as);
				as.push(edi);
				as.push(reinterpret_cast<sysint_t>(amx_));
				switch (instr.GetOpcode()) {
					case OP_SYSREQ_C:
						as.call(reinterpret_cast<void*>(GetNativeAddress(amx_, instr.GetOperand())));
						break;
					case OP_SYSREQ_D:
						as.call(reinterpret_cast<void*>(instr.GetOperand()));
						break;
				}
				as.add(esp, 8);
			end_alien_code(as);
		special_native:
			break;
		}
		case OP_SWITCH: { // offset
			// Compare PRI to the values in the case table (whose address
			// is passed as an offset from CIP) and_ jump to the associated
			// the address in the matching record.

			struct case_record {
				cell value;    // case value
				cell address;  // address to jump to (absolute)
			} *case_table;

			// Get pointer to the start of the case table.
			case_table = reinterpret_cast<case_record*>(instr.GetOperand() + sizeof(cell));

			// Get address of the "default" record.
			cell default_addr = case_table[0].address - reinterpret_cast<cell>(GetAmxCode());

			// The number of cases follows the CASETBL opcode (which follows the SWITCH).
			int num_cases = *(reinterpret_cast<cell*>(instr.GetOperand()) + 1);

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
				as.jl(Label(as, label_map.get(), default_addr));
				as.cmp(eax, *max_value);
				as.jg(Label(as, label_map.get(), default_addr));

				// OK now sequentially compare eax with each value.
				// This is pretty slow so I probably should optimize
				// this in future...
				for (int i = 0; i < num_cases; i++) {
					as.cmp(eax, case_table[i + 1].value);
					as.je(Label(as, label_map.get(), case_table[i + 1].address - reinterpret_cast<cell>(GetAmxCode())));
				}
			}

			// No match found - go for default case.
			as.jmp(Label(as, label_map.get(), default_addr));
			break;
		}
		case OP_CASETBL: // ...
			// A variable number of case records follows this opcode, where
			// each record takes two cells.
			break;
		case OP_SWAP_PRI:
			// [STK] = PRI and_ PRI = [STK]
			as.xchg(dword_ptr(esp), eax);
			break;
		case OP_SWAP_ALT:
			// [STK] = ALT and_ ALT = [STK]
			as.xchg(dword_ptr(esp), ecx);
			break;
		case OP_PUSH_ADR: // offset
			// [STK] = FRM + offset, STK = STK - cell size
			as.lea(edx, dword_ptr(ebp, instr.GetOperand() - reinterpret_cast<sysint_t>(GetAmxData())));
			as.push(edx);
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
	compiled_ = true;

	code_map_ = code_map.release();
	label_map_ = label_map.release();
}

void Jitter::halt(AsmJit::X86Assembler &as, cell error_code) {
	as.mov(dword_ptr_abs(reinterpret_cast<void*>(&GetAmx()->error)), error_code);
	as.mov(esp, dword_ptr_abs(reinterpret_cast<void*>(&halt_esp_)));
	as.mov(ebp, dword_ptr_abs(reinterpret_cast<void*>(&halt_ebp_)));
	as.ret();
}

void Jitter::begin_alien_code(AsmJit::X86Assembler &as) {
	as.lea(edx, dword_ptr(ebp, -reinterpret_cast<sysint_t>(GetAmxData())));
	as.mov(dword_ptr_abs(&amx_->frm), edx);
	as.mov(ebp, dword_ptr_abs(&ebp_));
	as.lea(edx, dword_ptr(esp, -reinterpret_cast<sysint_t>(GetAmxData())));
	as.mov(dword_ptr_abs(&amx_->stk), edx);
	as.mov(esp, dword_ptr_abs(&esp_));
}

void Jitter::end_alien_code(AsmJit::X86Assembler &as) {
	as.mov(dword_ptr_abs(&ebp_), ebp);
	as.mov(edx, dword_ptr_abs(&amx_->frm));
	as.lea(ebp, dword_ptr(edx, reinterpret_cast<sysint_t>(GetAmxData())));
	as.mov(dword_ptr_abs(&esp_), esp);
	as.mov(edx, dword_ptr_abs(&amx_->stk));
	as.lea(esp, dword_ptr(edx, reinterpret_cast<sysint_t>(GetAmxData())));
}

void Jitter::native_float(AsmJit::X86Assembler &as) {
	as.fild(dword_ptr(esp, 4));
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatabs(AsmJit::X86Assembler &as) {
	as.fld(dword_ptr(esp, 4));
	as.fabs();
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatadd(AsmJit::X86Assembler &as) {
	as.fld(dword_ptr(esp, 4));
	as.fadd(dword_ptr(esp, 8));
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatsub(AsmJit::X86Assembler &as) {
	as.fld(dword_ptr(esp, 4));
	as.fsub(dword_ptr(esp, 8));
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatmul(AsmJit::X86Assembler &as) {
	as.fld(dword_ptr(esp, 4));
	as.fmul(dword_ptr(esp, 8));
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatdiv(AsmJit::X86Assembler &as) {
	as.fld(dword_ptr(esp, 4));
	as.fdiv(dword_ptr(esp, 8));
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatsqroot(AsmJit::X86Assembler &as) {
	as.fld(dword_ptr(esp, 4));
	as.fsqrt();
	as.sub(esp, 4);
	as.fstp(dword_ptr(esp));
	as.mov(eax, dword_ptr(esp));
	as.add(esp, 4);
}

void Jitter::native_floatlog(AsmJit::X86Assembler &as) {
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

AsmJit::Label &Jitter::Label(AsmJit::X86Assembler &as, LabelMap *label_map, cell address, const std::string &name) {
	LabelMap::iterator iterator = label_map->find(TaggedAddress(address, name));
	if (iterator != label_map->end()) {
		return iterator->second;
	} else {
		std::pair<LabelMap::iterator, bool> where =
				label_map->insert(std::make_pair(TaggedAddress(address, name), as.newLabel()));
		return where.first->second;
	}
}

void *Jitter::ebp_;
void *Jitter::esp_;

void Jitter::ParseCode(cell start, cell end, std::vector<AmxInstruction> &instructions) const {
	const cell *cip = reinterpret_cast<cell*>(GetAmxCode() + start);

	while (cip < reinterpret_cast<cell*>(GetAmxCode() + end)) {
		cell opcode = *cip;

		if (opcode_list_ != 0) {
			for (int i = 0; i < NUM_AMX_OPCODES; i++) {
				if (opcode_list_[i] == opcode) {
					opcode = i;
					break;
				}
			}
		}

		AmxInstruction instr(static_cast<AmxOpcode>(opcode), cip);
		instructions.push_back(instr);

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
			cip += 2;
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
			cip++;
			break;

		// Special instructions.
		case OP_CASETBL: {
			cip++;
			int num = *cip++;
			cip += 2 * num + 1;
			break;
		}

		default:
			throw InvalidInstructionError(instr);
		}
	}
}

Jitter::~Jitter() {
	if (code_ != 0) {
		AsmJit::MemoryManager::getGlobal()->free(code_);
	}
}

void Jitter::Jump(cell ip, void *stack_ptr) {
	CodeMap::const_iterator it = code_map_->find(ip);
	if (it != code_map_->end()) {
		void *dest = it->second + reinterpret_cast<char*>(code_);
		#if defined COMPILER_MSVC
			__asm {
				mov esp, dword ptr [stack_ptr]
				jmp dword ptr [dest]
			}
		#elif defined COMPILER_GCC
			__asm__ __volatile__ (
				"movl %0, %%esp;"
				"jmpl *%1;"
					:
					: "r"(stack_ptr), "r"(dest)
					: );
		#endif
	}
}

int Jitter::CallFunction(cell address, cell *params, cell *retval) {
	int parambytes = params[0];
	int paramcount = parambytes / sizeof(cell);

	static void *start;
	start = GetInstrPtr(address, GetCode());

	static cell retval_;

	void *halt_esp = halt_esp_;
	void *halt_ebp = halt_ebp_;

	static void *amx_stack;
	amx_stack = GetAmxData() + amx_->stk;

	static void *amx_frame;
	amx_frame = GetAmxData() + amx_->frm;

	static Jitter *this_;
	this_ = this;

	amx_->error = AMX_ERR_NONE;

	#if defined COMPILER_MSVC
		__asm {
			push esi
			push edi

			mov dword ptr [ebp_], ebp
			mov dword ptr [esp_], esp

			mov ebp, dword ptr [amx_frame]
			mov esp, dword ptr [amx_stack]

			mov eax, dword ptr [this_]
			lea ecx, dword ptr [esp - 4]
			mov dword ptr [eax].halt_esp_, ecx
			mov dword ptr [eax].halt_ebp_, ebp

			call dword ptr [start]
			mov dword ptr [retval_], eax

			mov ebp, dword ptr [ebp_]
			mov esp, dword ptr [esp_]

			pop edi
			pop esi
		}
	#elif defined COMPILER_GCC
		__asm__ __volatile__ (
			"pushl %esi;"
			"pushl %edi;"
		);
		__asm__ __volatile__ (
			"movl %%esp, %0;"
			"movl %%ebp, %1;"
				: "=r"(esp_), "=r"(ebp_));
		__asm__ __volatile__ (
			"movl %0, %%ebp;"
			"movl %1, %%esp;"
				:
				: "r"(amx_frame), "r"(amx_stack));
		__asm__ __volatile__ (
			"leal -4(%%esp), %%eax;"
			"movl %%eax, (%0);"
			"movl %%ebp, (%1);"
				:
				: "r"(&this_->halt_esp_), "r"(&this_->halt_ebp_)
				: "%eax");
		__asm__ __volatile__ (
			"calll *%1;"
			"movl %%eax, %0;"
				: "=r"(retval_)
				: "r"(start)
				: "%ecx", "%edx", "%edi", "%esi");
		__asm__ __volatile__ (
			"movl %0, %%ebp;"
			"movl %1, %%esp;"
				:
				: "r"(ebp_), "r"(esp_));
		__asm__ __volatile__ (
			"popl %edi;"
			"popl %esi;");
	#endif

	halt_esp_ = halt_esp;
	halt_ebp_ = halt_ebp;

	if (retval != 0) {
		*retval = retval_;
	}

	return amx_->error;
}

int Jitter::CallPublicFunction(int index, cell *retval) {
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

	cell address = GetPublicAddress(amx_, index);
	if (address == 0) {
		// Bad public index - exit with error.
		return AMX_ERR_INDEX;
	}

	CallContext ctx(amx_);
	return CallFunction(address, ctx.GetParams(), retval);
}

} // namespace jit
