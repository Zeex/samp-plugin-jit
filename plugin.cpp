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

#include <cstddef>
#include <map>

#include "jit.h"
#include "jump-x86.h"
#include "plugin.h"
#include "pluginversion.h"

using namespace jit;

typedef void (*logprintf_t)(const char *format, ...);
static logprintf_t logprintf;

typedef std::map<AMX*, JIT*> JITMap;
static JITMap jit_map;

static void **ppPluginData;

static int AMXAPI amx_GetAddr_JIT(AMX *amx, cell amx_addr, cell **phys_addr) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	*phys_addr = reinterpret_cast<cell*>(amx->base + hdr->dat + amx_addr);
	return AMX_ERR_NONE;
}

static int AMXAPI amx_Exec_JIT(AMX *amx, cell *retval, int index) {
	if (index != AMX_EXEC_CONT) {
		JITMap::const_iterator it = jit_map.find(amx);
		if (it != jit_map.end()) {
			return it->second->CallPublicFunction(index, retval);
		}
	}
	return AMX_ERR_NONE;
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
	ppPluginData = ppData;
	logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
	logprintf("  JIT plugin v%s is OK.", PLUGIN_VERSION_STRING);
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
	for (JITMap::iterator it = jit_map.begin(); it != jit_map.end(); ++it) {
		delete it->second;
	}
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
	new JumpX86(((void**)ppPluginData[PLUGIN_DATA_AMX_EXPORTS])[PLUGIN_AMX_EXPORT_Exec], (void*)amx_Exec_JIT);
	new JumpX86(((void**)ppPluginData[PLUGIN_DATA_AMX_EXPORTS])[PLUGIN_AMX_EXPORT_GetAddr], (void*)amx_GetAddr_JIT);

	jit_map.insert(std::make_pair(amx, new JIT(amx)));

	return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
	JITMap::iterator it = jit_map.find(amx);
	if (it != jit_map.end()) {
		delete it->second;
		jit_map.erase(it);		
	}
	return AMX_ERR_NONE;
}
