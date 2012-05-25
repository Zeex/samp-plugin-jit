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
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

#include <amx/amx.h>

#include "jit.h"
#include "jump-x86.h"
#include "plugin.h"
#include "version.h"

#ifdef WIN32
	#include <Windows.h>
#else
	#ifndef _GNU_SOURCE
		#define _GNU_SOURCE 1 // for dladdr()
	#endif
	#include <dlfcn.h>
#endif

extern void *pAMXFunctions;

typedef void (*logprintf_t)(const char *format, ...);
static logprintf_t logprintf;

typedef std::map<AMX*, jit::Jitter*> AmxToJitterMap;
static AmxToJitterMap amx2jitter;

static JumpX86 amx_Exec_hook;
static cell *opcodeTable = 0;

static std::string InstrToString(const jit::AmxInstruction &instr) {
	std::stringstream stream;
	if (instr.getName() != 0) {
		stream << instr.getName();
	} else {
		stream << std::setw(8) << std::setfill('0') << std::hex << instr.getOpcode();
	}
	std::vector<cell> opers = instr.getOperands();
	for (std::vector<cell>::const_iterator it = opers.begin(); it != opers.end(); ++it) {
		stream << ' ' << std::setw(8) << std::setfill('0') << std::hex << *it;
	}
	return stream.str();
}

static void CompileError(const jit::AmxVm &vm, const jit::AmxInstruction &instr) {
	logprintf("JIT compilation error at %08x:", instr.getAddress());
	logprintf("  %s", InstrToString(instr).c_str());
}

static void RuntimeError(int errorCode) {
	static const char *errorStrings[] = {
		/* AMX_ERR_NONE      */ "(none)",
		/* AMX_ERR_EXIT      */ "Forced exit",
		/* AMX_ERR_ASSERT    */ "Assertion failed",
		/* AMX_ERR_STACKERR  */ "Stack/heap collision (insufficient stack size)",
		/* AMX_ERR_BOUNDS    */ "Array index out of bounds",
		/* AMX_ERR_MEMACCESS */ "Invalid memory access",
		/* AMX_ERR_INVINSTR  */ "Invalid instruction",
		/* AMX_ERR_STACKLOW  */ "Stack underflow",
		/* AMX_ERR_HEAPLOW   */ "Heap underflow",
		/* AMX_ERR_CALLBACK  */ "No (valid) native function callback",
		/* AMX_ERR_NATIVE    */ "Native function failed",
		/* AMX_ERR_DIVIDE    */ "Divide by zero",
		/* AMX_ERR_SLEEP     */ "(sleep mode)",
		/* 13 */                "(reserved)",
		/* 14 */                "(reserved)",
		/* 15 */                "(reserved)",
		/* AMX_ERR_MEMORY    */ "Out of memory",
		/* AMX_ERR_FORMAT    */ "Invalid/unsupported P-code file format",
		/* AMX_ERR_VERSION   */ "File is for a newer version of the AMX",
		/* AMX_ERR_NOTFOUND  */ "File or function is not found",
		/* AMX_ERR_INDEX     */ "Invalid index parameter (bad entry point)",
		/* AMX_ERR_DEBUG     */ "Debugger cannot run",
		/* AMX_ERR_INIT      */ "AMX not initialized (or doubly initialized)",
		/* AMX_ERR_USERDATA  */ "Unable to set user data field (table full)",
		/* AMX_ERR_INIT_JIT  */ "Cannot initialize the JIT",
		/* AMX_ERR_PARAMS    */ "Parameter error",
		/* AMX_ERR_DOMAIN    */ "Domain error, expression result does not fit in range",
		/* AMX_ERR_GENERAL   */ "General error (unknown or unspecific error)"
    };
	logprintf("JIT runtime error %d: \"%s\"", errorCode, errorStrings[errorCode]);
}

static int AMXAPI amx_Exec_JIT(AMX *amx, cell *retval, int index) {
	#if defined __GNUC__ && !defined WIN32
		if ((amx->flags & AMX_FLAG_BROWSE) == AMX_FLAG_BROWSE) {
			assert(::opcodeTable != 0);
			*retval = reinterpret_cast<cell>(::opcodeTable);
			return AMX_ERR_NONE;
		}
	#endif
	AmxToJitterMap::iterator iterator = ::amx2jitter.find(amx);
	if (iterator == ::amx2jitter.end()) {
		// Compilation previously failed, call normal amx_Exec().
		JumpX86::ScopedRemove r(&amx_Exec_hook);
		return amx_Exec(amx, retval, index);
	} else {
		jit::Jitter *jitter = iterator->second;
		int error = jitter->exec(index, retval);
		if (error != AMX_ERR_NONE) {
			RuntimeError(error);
		}
		return error;
	}
}

static std::string GetModuleNameBySymbol(void *symbol) {
	char module[FILENAME_MAX] = "";
	if (symbol != 0) {
		#ifdef WIN32
			MEMORY_BASIC_INFORMATION mbi;
			VirtualQuery(symbol, &mbi, sizeof(mbi));
			GetModuleFileName((HMODULE)mbi.AllocationBase, module, FILENAME_MAX);
		#else
			Dl_info info;
			dladdr(symbol, &info);
			strcpy(module, info.dli_fname);
		#endif
	}
	return std::string(module);
}

static std::string GetFileName(const std::string &path) {
	std::string::size_type lastSep = path.find_last_of("/\\");
	if (lastSep != std::string::npos) {
		return path.substr(lastSep + 1);
	}
	return path;
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
	logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
	pAMXFunctions = reinterpret_cast<void*>(ppData[PLUGIN_DATA_AMX_EXPORTS]);

	void *funAddr = JumpX86::GetTargetAddress(((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Exec]);
	if (funAddr != 0) {
		std::string module = GetFileName(GetModuleNameBySymbol(funAddr));
		if (!module.empty() && module != "samp-server.exe" && module != "samp03svr") {
			logprintf("  JIT must be loaded before %s", module.c_str());
			return false;
		}
	}

	logprintf("  JIT plugin v%s is OK.", PLUGIN_VERSION_STRING);
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
	for (AmxToJitterMap::iterator it = amx2jitter.begin(); it != amx2jitter.end(); ++it) {
		delete it->second;
	}
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
	typedef int (AMXAPI *amx_Exec_t)(AMX *amx, cell *retval, int index);
	amx_Exec_t amx_Exec = (amx_Exec_t)((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Exec];

	typedef int (AMXAPI *amx_GetAddr_t)(AMX *amx, cell amx_addr, cell **phys_addr);
	amx_GetAddr_t amx_GetAddr = (amx_GetAddr_t)((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_GetAddr];

	#if defined __GNUC__ && !defined WIN32
		// Get opcode list before we hook amx_Exec().
		if (::opcodeTable == 0) {
			amx->flags |= AMX_FLAG_BROWSE;
			amx_Exec(amx, reinterpret_cast<cell*>(&::opcodeTable), 0);
			amx->flags &= ~AMX_FLAG_BROWSE;
		}
	#endif

	jit::Jitter *jitter = new jit::Jitter(amx, ::opcodeTable);
	if (!jitter->compile(CompileError)) {
		delete jitter;
	} else {
		::amx2jitter.insert(std::make_pair(amx, jitter));
	}

	if (!amx_Exec_hook.IsInstalled()) {
		amx_Exec_hook.Install(
			(void*)amx_Exec,
			(void*)amx_Exec_JIT);
	}

	return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
	AmxToJitterMap::iterator it = amx2jitter.find(amx);
	if (it != amx2jitter.end()) {
		delete it->second;
		amx2jitter.erase(it);
	}
	return AMX_ERR_NONE;
}
