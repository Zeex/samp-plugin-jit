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
#include <string>

#include "jit.h"
#include "logprintf.h"
#include "os.h"
#include "plugin.h"
#include "version.h"
#include <subhook.h>

typedef int (AMXAPI *AMX_EXEC)(AMX *amx, cell *retval, int index);

extern void *pAMXFunctions;
static SubHook exec_hook;

#ifdef LINUX
  static cell *opcode_table = 0;
#endif

static void *GetAmxFunction(int index) {
  return static_cast<void**>(pAMXFunctions)[index];
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
  int error = JIT::GetInstance(amx)->Exec(retval, index);
  if (error == AMX_ERR_INIT_JIT) {
    AMX_EXEC exec = (AMX_EXEC)exec_hook.GetTrampoline();
    return exec(amx, retval, index);
  }
  return error;
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
  return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
  logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
  pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];

  void *exec_start = GetAmxFunction(PLUGIN_AMX_EXPORT_Exec);
  void *other_guy = SubHook::ReadDst(exec_start);

  if (other_guy != 0) {
    std::string module = GetFileName(os::GetModuleName(other_guy));
    if (!module.empty()) {
      logprintf("  JIT must be loaded before '%s'", module.c_str());
    } else {
      logprintf("  Sorry, your server is messed up");
    }
    return false;
  }

  #ifdef LINUX
    // Get the opcode table before we hook amx_Exec().
    AMX amx = {0};
    amx.flags |= AMX_FLAG_BROWSE;
    amx_Exec(&amx, reinterpret_cast<cell*>(&::opcode_table), 0);
    amx.flags &= ~AMX_FLAG_BROWSE;
  #endif
  exec_hook.Install(exec_start, (void *)amx_Exec_JIT);

  logprintf("  JIT plugin v%s is OK.", PROJECT_VERSION_STRING);
  return true;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
  JIT::CreateInstance(amx);
  return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
 JIT::DestroyInstance(amx);
 return AMX_ERR_NONE;
}
