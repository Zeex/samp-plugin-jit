// Copyright (c) 2013 Zeex
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

#ifndef JIT_COMPILER_H
#define JIT_COMPILER_H

#include "amxptr.h"
#include "callconv.h"
#include "macros.h"

namespace jit {

class AMXInstruction;

typedef int (JIT_CDECL *EntryPoint)(cell index, cell *retval);

class CompileErrorHandler {
public:
  virtual ~CompileErrorHandler() {}
  virtual void execute(const AMXInstruction &instr) = 0;
};

class CompilerOutput {
 public:
  virtual ~CompilerOutput() {}

  // Returns a pointer to the code buffer.
  virtual void *code() const = 0;

  // Returns the size of the code in bytes.
  virtual std::size_t code_size() const = 0;

  // Returns a pointer to the entry point function.
  virtual EntryPoint entry_point() const = 0;
};

class Compiler {
 public:
  virtual ~Compiler() {}

  // Compiles the specified AMX script. The optional error hander is called at
  // most only once - on first compile error.
  virtual CompilerOutput *compile(AMXPtr amx, CompileErrorHandler *error_handler = 0) = 0;
};

} // namespace jit

#endif // !JIT_COMPILER_H
