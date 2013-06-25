// Copyright (c) 2012-2013 Zeex
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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#ifdef _WIN32
  #include <windows.h>
#else
  #define _GNU_SOURCE
  #include <dlfcn.h>
#endif
#include <subhook.h>
#include "plugin.h"
#include "version.h"
#include "amxjit/compiler.h"
#if JIT_ASMJIT
  #include "amxjit/compiler-asmjit.h"
#endif
#if JIT_LLVM
  #include "amxjit/compiler-llvm.h"
#endif
#include "amxjit/disasm.h"
#include "amxjit/jit.h"

#if defined __GNUC__ && !defined __MINGW32__
  #define USE_OPCODE_MAP 1
#endif

extern void *pAMXFunctions;

typedef void (*LogprintfType)(const char *format, ...);
static LogprintfType logprintf;

typedef std::map<AMX*, amxjit::JIT*> AmxToJitMap;
static AmxToJitMap amxToJit;

static SubHook execHook;
#if USE_OPCODE_MAP
  static cell *opcodeMap = 0;
#endif

class CompileErrorHandler : public amxjit::CompileErrorHandler {
 public:
  virtual void Execute(const amxjit::Instruction &instr) {
    logprintf("[jit] Invalid or unsupported instruction at address %p:",
              instr.GetAddress());
    logprintf("[jit]   => %s", instr.AsString().c_str());
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

static bool Compile(AMX *amx, amxjit::JIT *jit) {
  bool result = false;

  if (!OnJITCompile(amx)) {
    logprintf("[jit] Compilation was disabled");
  } else {
    const char *backendString = getenv("JIT_BACKEND");
    if (backendString == 0) {
      backendString = "asmjit";
    }

    amxjit::Compiler *compiler = 0;
    #if JIT_ASMJIT
      if (std::strcmp(backendString, "asmjit") == 0) {
        compiler = new amxjit::CompilerAsmjit;
      }
    #endif
    #if JIT_LLVM
      if (std::strcmp(backendString, "llvm") == 0) {
        compiler = new amxjit::CompilerLLVM;
      }
    #endif

    if (compiler != 0) {
      CompileErrorHandler errorHandler;
      result = jit->Compile(compiler, &errorHandler);
      delete compiler;
    } else {
      logprintf("[jit] Unknown backend '%s'", backendString);
    }
  }

  return result;
}

static std::string GetModulePath(void *address,
                                 std::size_t maxLength = FILENAME_MAX)
{
  #ifdef _WIN32
    std::vector<char> name(maxLength + 1);
    if (address != 0) {
      MEMORY_BASIC_INFORMATION mbi;
      VirtualQuery(address, &mbi, sizeof(mbi));
      GetModuleFileName((HMODULE)mbi.AllocationBase, &name[0], maxLength);
    }
    return std::string(&name[0]);
  #else
    std::vector<char> name(maxLength + 1);
    if (address != 0) {
      Dl_info info;
      dladdr(address, &info);
      strncpy(&name[0], info.dli_fname, maxLength);
    }  
    return std::string(&name[0]);
  #endif
}

static std::string GetFileName(const std::string &path) {
  std::string::size_type pos = path.find_last_of("/\\");
  if (pos != std::string::npos) {
    return path.substr(pos + 1);
  }
  return path;
}

static int AMXAPI amx_Exec_JIT(AMX *amx, cell *retval, int index) {
  #if USE_OPCODE_MAP
    if ((amx->flags & AMX_FLAG_BROWSE) == AMX_FLAG_BROWSE) {
      assert(::opcodeMap != 0);
      *retval = reinterpret_cast<cell>(::opcodeMap);
      return AMX_ERR_NONE;
    }
  #endif

  amxjit::JIT *jit = 0;

  if (index == AMX_EXEC_MAIN) {
    jit = new amxjit::JIT(amx);
    if (Compile(amx, jit)) {
      ::amxToJit[amx] = jit;
    } else {
      delete jit;
      jit = 0;
    }
  } else {
    AmxToJitMap::iterator iterator = ::amxToJit.find(amx);
    if (iterator != ::amxToJit.end()) {
      jit = iterator->second;
    }
  }

  if (jit != 0) {
    return jit->Exec(index, retval);
  } else {
    SubHook::ScopedRemove r(&execHook);
    return amx_Exec(amx, retval, index);
  }
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
  return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
  logprintf = (LogprintfType)ppData[PLUGIN_DATA_LOGPRINTF];
  pAMXFunctions = reinterpret_cast<void*>(ppData[PLUGIN_DATA_AMX_EXPORTS]);

  void *ptr = SubHook::ReadDst(((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Exec]);
  if (ptr != 0) {
    std::string module = GetFileName(GetModulePath(ptr));
    if (!module.empty()) {
      logprintf("  JIT must be loaded before '%s'", module.c_str());
      return false;
    }
  }

  #if USE_OPCODE_MAP
    // Get the opcode map before we hook amx_Exec().
    AMX amx = {0};
    amx.flags |= AMX_FLAG_BROWSE;
    amx_Exec(&amx, reinterpret_cast<cell*>(&::opcodeMap), 0);
    amx.flags &= ~AMX_FLAG_BROWSE;
  #endif

  void **exports = reinterpret_cast<void**>(pAMXFunctions);
  execHook.Install(exports[PLUGIN_AMX_EXPORT_Exec], (void*)amx_Exec_JIT);

  logprintf("  JIT plugin v%s is OK.", PROJECT_VERSION_STRING);
  return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
  for (AmxToJitMap::iterator iterator = amxToJit.begin();
       iterator != amxToJit.end(); iterator++) {
    delete iterator->second;
  }
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
  return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
  AmxToJitMap::iterator iterator = amxToJit.find(amx);
  if (iterator != amxToJit.end()) {
    delete iterator->second;
    amxToJit.erase(iterator);
  }
  return AMX_ERR_NONE;
}
