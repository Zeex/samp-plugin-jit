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

#ifndef AMXJIT_JIT_H
#define AMXJIT_JIT_H

#include <amx/amx.h>
#include "amxptr.h"
#include "macros.h"

namespace amxjit {

class Compiler;
class CompilerOutput;
class CompileErrorHandler;

class JIT {
 public:
  JIT(AMXPtr amx);
  ~JIT();

  // Compiles the AMX script. If something goes wrong it calls the
  // error handler. The handler is called only once as compilation
  // stops after the first error.
  bool Compile(Compiler *compiler,
               CompileErrorHandler *error_handler = 0);

  // Executes a public function and returns one of AMX error codes.
  // Use this method as a drop-in replacement for amx_Exec().
  int Exec(cell index, cell *retval);

 private:
  AMXPtr amx;
  CompilerOutput *output;

 private:
  AMXJIT_DISALLOW_COPY_AND_ASSIGN(JIT);
};

} // namespace amxjit

#endif // !AMXJIT_JIT_H
