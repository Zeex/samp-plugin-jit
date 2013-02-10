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

#ifndef JIT_JIT_H
#define JIT_JIT_H

#include <amx/amx.h>
#include "amxptr.h"
#include "callconv.h"
#include "macros.h"

namespace jit {

class Compiler;
class CompilerOutput;
class CompileErrorHandler;

class JIT {
 public:
  typedef int (JIT_CDECL *EntryPoint)(cell index, cell *retval);

  JIT(AMXPtr amx);
  ~JIT();

  // This method JIT-compiles a script and stores the resulting code.
  // If an error occurs during compilation process it calls provided error
  // handler and returns false.
  bool compile(Compiler *compiler, CompileErrorHandler *error_handler = 0);

  // Executes a public function and returns one of the AMX error codes. Use
  // this method as a drop-in replacement for amx_Exec().
  int exec(cell index, cell *retval);

 private:
  AMXPtr amx_;
  CompilerOutput *output_;

 private:
  JIT_DISALLOW_COPY_AND_ASSIGN(JIT);
};

} // namespace jit

#endif // !JIT_JIT_H
