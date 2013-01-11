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

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>

#include <subhook.h>

#include "amxjit.h"
#include "amxpathfinder.h"
#include "fileutils.h"
#include "options.h"
#include "os.h"
#include "plugin.h"
#include "version.h"

using namespace amxjit;

extern void *pAMXFunctions;

typedef void (*logprintf_t)(const char *format, ...);
static logprintf_t logprintf;

typedef std::map<AMX*, JIT*> AmxToJitMap;
static AmxToJitMap amx2jit;

static SubHook amx_Exec_hook;
static cell *opcodeTable = 0;

static int AMXAPI amx_Exec_JIT(AMX *amx, cell *retval, int index)
{
	#if defined __GNUC__ && !defined WIN32
		if ((amx->flags & AMX_FLAG_BROWSE) == AMX_FLAG_BROWSE) {
			assert(::opcodeTable != 0);
			*retval = reinterpret_cast<cell>(::opcodeTable);
			return AMX_ERR_NONE;
		}
	#endif
	AmxToJitMap::iterator iterator = ::amx2jit.find(amx);
	if (iterator == ::amx2jit.end()) {
		SubHook::ScopedRemove r(&amx_Exec_hook);
		return amx_Exec(amx, retval, index);
	} else {
		JIT *jit = iterator->second;
		return jit->exec(index, retval);
	}
}

class CompileErrorHandler : public JITCompileErrorHandler {
public:
	CompileErrorHandler(JIT *jit) : jit_(jit) {}

	virtual void execute(const AMXInstruction &instr) {
		logprintf("[jit] Invalid or unsupported instruction at address %p:", instr.address());
		logprintf("[jit]  => %s", instr.string().c_str());
	}

private:
	JIT *jit_;
};

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

static void *AMXAPI amx_Align(void *v) { return v; }

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
	logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
	pAMXFunctions = reinterpret_cast<void*>(ppData[PLUGIN_DATA_AMX_EXPORTS]);

	((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Align16] = (void*)amx_Align;
	((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Align32] = (void*)amx_Align;
	((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Align64] = (void*)amx_Align;

	void *ptr = subhook_read_destination(((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Exec]);
	if (ptr != 0) {
		std::string module = fileutils::GetFileName(os::GetModulePath(ptr));
		if (!module.empty()) {
			logprintf("  JIT must be loaded before '%s'", module.c_str());
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

	logprintf("  JIT plugin v%s is OK.", PROJECT_VERSION_STRING);
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	for (AmxToJitMap::iterator it = amx2jit.begin(); it != amx2jit.end(); ++it) {
		delete it->second;
	}
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx)
{
	JIT *jit = new JIT(amx);
	#if defined __GNUC__ && defined LINUX
		jit->setOpcodeMap(::opcodeTable);
	#endif

	AsmJit::X86Assembler as;
	jit->setAssembler(&as);

	AsmJit::FileLogger logger;
	as.setLogger(&logger);

	AMXPathFinder finder;
	finder.AddSearchPath(".");
	finder.AddSearchPath("gamemodes");
	finder.AddSearchPath("filterscripts");

	std::string amxPath = finder.FindAMX(amx);

	if (Options::Get().dump_asm()) {
		std::string asmPath = amxPath + ".asm";
		logger.setStream(std::fopen(asmPath.c_str(), "w"));
	} else {
		as.setLogger(0);
	}
	
	CompileErrorHandler errorHandler(jit);
	if (!jit->compile(&errorHandler)) {
		logprintf("[jit] Failed to compile script '%s'", amxPath.c_str());
		delete jit;
		return AMX_ERR_NONE;
	}

	jit->setAssembler(0);
	if (logger.getStream() != 0) {
		std::fclose(logger.getStream());
	}

	if (Options::Get().dump_bin()) {
		std::string binPath = amxPath + ".bin";
		std::FILE *bin = std::fopen(binPath.c_str(), "w");
		if (bin != 0) {
			std::fwrite(jit->code(), jit->codeSize(), 1, bin);
			std::fclose(bin);
		}
	}

	::amx2jit.insert(std::make_pair(amx, jit));
	return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx)
{
	AmxToJitMap::iterator it = amx2jit.find(amx);
	if (it != amx2jit.end()) {
		delete it->second;
		amx2jit.erase(it);
	}
	return AMX_ERR_NONE;
}
