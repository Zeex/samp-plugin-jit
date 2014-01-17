// Copyright (c) 2012-2014 Zeex
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
#include <map>
#include <string>
#include <utility>
#include <subhook.h>
#include "configreader.h"
#include "os.h"
#include "plugin.h"
#include "version.h"
#if JIT_ASMJIT
  #include "amxjit/compiler-asmjit.h"
#endif
#if JIT_LLVM
  #include "amxjit/compiler-llvm.h"
#endif
#include "amxjit/disasm.h"

extern void *pAMXFunctions;

typedef void (*logprintf_t)(const char *format, ...);
static logprintf_t logprintf;

typedef std::map<AMX*, amxjit::CompileOutput*> AmxMap;
static AmxMap amx_map;

static SubHook amx_Exec_hook;
#ifdef LINUX
  static cell *opcode_table = 0;
#endif

class CompileErrorHandler : public amxjit::CompileErrorHandler {
 public:
  virtual void Execute(const amxjit::Instruction &instr) {
    logprintf("[jit] Invalid or unsupported instruction at address %p:",
              instr.address());
    logprintf("[jit]   => %s", instr.ToString().c_str());
  }
};

static cell OnJITCompile(AMX *amx) {
  int index;
  if (amx_FindPublic(amx, "OnJITCompile", &index) == AMX_ERR_NONE) {
    cell retval;
    amx_Exec(amx, &retval, index);
    return retval;
  }
  return 1;
}

static amxjit::CompileOutput *Compile(AMX *amx) {
  if (!OnJITCompile(amx)) {
    logprintf("[jit] Compilation was disabled");
    return 0;
  }

  amxjit::Compiler *compiler = 0;
  amxjit::CompileOutput *output = 0;

  std::string backend = "asmjit";
  ConfigReader("server.cfg").GetOption("jit_backend", backend);
  
  #if JIT_ASMJIT
    if (backend == "asmjit") {
      compiler = new amxjit::CompilerAsmjit;
    }
  #endif
  #if JIT_LLVM
    if (backend == "llvm") {
      compiler = new amxjit::CompilerLLVM;
    }
  #endif

  if (compiler != 0) {
    CompileErrorHandler error_handler;
    compiler->SetErrorHandler(&error_handler);
    output = compiler->Compile(amx);
    delete compiler;
  } else {
    logprintf("[jit] Unrecognized backend '%s'", backend.c_str());
  }

  return output;
}

static std::string GetFileName(const std::string &path) {
  std::string::size_type pos = path.find_last_of("/\\");
  if (pos != std::string::npos) {
    return path.substr(pos + 1);
  }
  return path;
}

static int AMXAPI amx_Exec_JIT(AMX *amx, cell *retval, int index) {
  #ifdef LINUX
    if ((amx->flags & AMX_FLAG_BROWSE) == AMX_FLAG_BROWSE) {
      assert(::opcode_table != 0);
      *retval = reinterpret_cast<cell>(::opcode_table);
      return AMX_ERR_NONE;
    }
  #endif

  amxjit::CompileOutput *code = 0;
  if (index == AMX_EXEC_MAIN) {
    code = Compile(amx);
    ::amx_map.insert(std::make_pair(amx, code));
  } else {
    AmxMap::iterator it = ::amx_map.find(amx);
    if (it != ::amx_map.end()) {
      code = it->second;
    }
  }

  if (code != 0) {
    amxjit::EntryPoint entry_point = code->GetEntryPoint();
    return entry_point(index, retval);
  }

  SubHook::ScopedRemove r(&amx_Exec_hook);
  return amx_Exec(amx, retval, index);
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
  return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
  logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
  pAMXFunctions = reinterpret_cast<void*>(ppData[PLUGIN_DATA_AMX_EXPORTS]);

  void *ptr = SubHook::ReadDst(((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Exec]);
  if (ptr != 0) {
    std::string module = GetFileName(os::GetModuleName(ptr));
    if (!module.empty()) {
      logprintf("  JIT must be loaded before '%s'", module.c_str());
      return false;
    }
  }

  #ifdef LINUX
    // Get the opcode map before we hook amx_Exec().
    AMX amx = {0};
    amx.flags |= AMX_FLAG_BROWSE;
    amx_Exec(&amx, reinterpret_cast<cell*>(&::opcode_table), 0);
    amx.flags &= ~AMX_FLAG_BROWSE;
  #endif

  void **exports = reinterpret_cast<void**>(pAMXFunctions);
  amx_Exec_hook.Install(exports[PLUGIN_AMX_EXPORT_Exec], (void*)amx_Exec_JIT);

  logprintf("  JIT plugin v%s is OK.", PROJECT_VERSION_STRING);
  return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
  for (AmxMap::iterator it = ::amx_map.begin();
       it != ::amx_map.end(); it++) {
    it->second->Delete();
  }
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
  return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
  AmxMap::iterator it = ::amx_map.find(amx);
  if (it != ::amx_map.end()) {
    it->second->Delete();
    ::amx_map.erase(it);
  }
  return AMX_ERR_NONE;
}
