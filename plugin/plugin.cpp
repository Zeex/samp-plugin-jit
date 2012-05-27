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
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>

#include "amxpathfinder.h"
#include "jit.h"
#include "jump-x86.h"
#include "options.h"
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
			assert(::opcodeTable != 0);
			*retval = reinterpret_cast<cell>(::opcodeTable);
			return AMX_ERR_NONE;
		}
	#endif
	AmxToJitterMap::iterator iterator = ::amx2jitter.find(amx);
	if (iterator == ::amx2jitter.end()) {
		JumpX86::ScopedRemove r(&amx_Exec_hook);
		return amx_Exec(amx, retval, index);
	} else {
		jit::Jitter *jitter = iterator->second;
		return jitter->exec(index, retval);
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
	logprintf("JIT failed to compile instruction at %08x:", instr.getAddress());
	logprintf("  %s", instr.asString().c_str());
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

#if defined WIN32
	#define SAMP_SERVER_BINARY "samp-server.exe"
#else
	#define SAMP_SERVER_BINARY "samp03svr"
#endif

static void *AMXAPI amx_Align(void *v) { return v; }

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
	logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
	pAMXFunctions = reinterpret_cast<void*>(ppData[PLUGIN_DATA_AMX_EXPORTS]);

	((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Align16] = (void*)amx_Align;
	((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Align32] = (void*)amx_Align;
	((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Align64] = (void*)amx_Align;

	void *ptr = JumpX86::GetTargetAddress(((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Exec]);
	if (ptr != 0) {
		std::string module = GetFileName(GetModuleNameBySymbol(ptr));
		if (!module.empty() && module != SAMP_SERVER_BINARY) {
			logprintf("  JIT must be loaded before %s", module.c_str());
			return false;
		}
	}

	#if defined __GNUC__ && !defined WIN32
		// Get opcode list before we hook amx_Exec().
		AMX amx = {0};
		amx.flags |= AMX_FLAG_BROWSE;
		amx_Exec(&amx, reinterpret_cast<cell*>(&::opcodeTable), 0);
		amx.flags &= ~AMX_FLAG_BROWSE;
	#endif

	amx_Exec_hook.Install(
		((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Exec],
		(void*)amx_Exec_JIT);

	logprintf("  JIT plugin v%s is OK.", PLUGIN_VERSION_STRING);
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
	for (AmxToJitterMap::iterator it = amx2jitter.begin(); it != amx2jitter.end(); ++it) {
		delete it->second;
	}
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
	jit::Jitter *jitter = new jit::Jitter(amx);
	#if defined __GNUC__ && defined LINUX
		jitter->setOpcodeTable(::opcodeTable);
	#endif

	AsmJit::X86Assembler as;
	jitter->setAssembler(&as);

	AsmJit::FileLogger logger;
	as.setLogger(&logger);

	AMXPathFinder finder;
	finder.AddSearchPath("./");
	finder.AddSearchPath("gamemodes/");
	finder.AddSearchPath("filterscripts/");

	std::string amxPath = finder.FindAMX(amx);

	if (Options::Get().dump_asm()) {
		std::string asmPath = amxPath + ".asm";
		logger.setStream(std::fopen(asmPath.c_str(), "w"));
	} else {
		as.setLogger(0);
	}
	
	if (!jitter->compile(CompileError)) {
		delete jitter;
	} else {
		if (logger.getStream() != 0) {
			std::fclose(logger.getStream());
		}
		jitter->setAssembler(0);

		if (Options::Get().dump_bin()) {
			std::string binPath = amxPath + ".bin";
			std::FILE *bin = std::fopen(binPath.c_str(), "w");
			if (bin != 0) {
				std::fwrite(jitter->getCode(), jitter->getCodeSize(), 1, bin);
				std::fclose(bin);
			}
		}

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
