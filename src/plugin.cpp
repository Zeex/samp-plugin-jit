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
#include <fstream>
#include <map>
#include <string>

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

static int AMXAPI amx_Exec_JIT(AMX *amx, cell *retval, int index) {
	#if defined __GNUC__ && !defined WIN32
		if ((amx->flags & AMX_FLAG_BROWSE) == AMX_FLAG_BROWSE) {
			assert(::opcode_list != 0);
			*retval = reinterpret_cast<cell>(::opcode_list);
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
		return jitter->callPublicFunction(index, retval);
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

static void CompileError(const jit::AmxVm &vm, const jit::AmxInstruction &instr) {
	logprintf("JIT compilation error occured at %08x:", instr.getRelPtr(vm.getAmx()));
	const cell *ip = instr.getPtr();
	logprintf("  %08x %08x %08x %08x %08x %08x ...",
			*ip, *(ip + 1), *(ip + 2), *(ip + 3), *(ip + 4), *(ip + 5));
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

	if (!amx_Exec_hook.IsInstalled()) {
		amx_Exec_hook.Install(
			(void*)amx_Exec,
			(void*)amx_Exec_JIT);
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
		if (::opcode_list == 0) {
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
