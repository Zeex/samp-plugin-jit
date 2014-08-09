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
#include <cstdarg>
#include <string>

#include "configreader.h"
#include "jit.h"
#include "logprintf.h"
#include "plugin.h"

#if JIT_ASMJIT
  #include "amxjit/compiler-asmjit.h"
#endif
#if JIT_LLVM
  #include "amxjit/compiler-llvm.h"
#endif
#include "amxjit/disasm.h"

#define logprintf Use_Printf_isntead_of_logprintf

namespace {

void Printf(const char *format, ...) {
  std::va_list va;
  va_start(va, format);

  std::string new_format;
  new_format.append("[jit] ");
  new_format.append(format);

  vlogprintf(new_format.c_str(), va);
  va_end(va);
}

class ErrorHandler: public amxjit::CompileErrorHandler {
 public:
  virtual void Execute(const amxjit::Instruction &instr) {
    Printf("Invalid or unsupported instruction at address %p:",
              instr.address());
    Printf("  => %s", instr.ToString().c_str());
  }
};

cell OnJITCompile(AMX *amx) {
  int index;
  if (amx_FindPublic(amx, "OnJITCompile", &index) == AMX_ERR_NONE) {
    cell retval;
    amx_Exec(amx, &retval, index);
    return retval;
  }
  return 1;
}

amxjit::CompileOutput *Compile(AMX *amx) {
  if (!OnJITCompile(amx)) {
    Printf("Compilation was disabled");
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
    ErrorHandler error_handler;
    compiler->SetErrorHandler(&error_handler);
    output = compiler->Compile(amx);
    delete compiler;
  } else {
    Printf("Unrecognized backend '%s'", backend.c_str());
  }

  return output;
}

} // anonymous namespace

JIT::JIT(AMX *amx):
  AMXService<JIT>(amx),
  compiled_(false),
  code_()
{
}

JIT::~JIT() {
  if (code_ != 0) {
    code_->Delete();
  }
}

int JIT::Exec(cell *retval, int index) {
  if (!compiled_) {
    code_ = Compile(amx());
    compiled_ = true;
  }
  if (code_ != 0) {
    amxjit::EntryPoint entry_point = code_->GetEntryPoint();
    return entry_point(index, retval);
  } else {
    return amx_Exec(amx(), retval, index);
  }
}
