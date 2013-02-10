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

#ifndef JIT_COMPILER_H
#define JIT_COMPILER_H

#include "amxptr.h"
#include "macros.h"

namespace jit {

class AMXInstruction;
class Backend;
class BackendOutput;
class CompileErrorHandler;

// Compiler output represents the output blob of the Compiler.
// The only requirement to it is that the first 4 bytes must point to the
// exec() function which is invoked by the JIT at any time later on.
//
// WARNING: Do not copy the code into another buffer! There are hard-coded
// data references in it - they simply will be not valid in the new buffer.
class CompilerOutput {
 public:
  CompilerOutput(BackendOutput *backend_output);
  ~CompilerOutput();

  void *code() const;
  std::size_t code_size() const;

 private:
  BackendOutput *backend_output_;

 private:
  JIT_DISALLOW_COPY_AND_ASSIGN(CompilerOutput);
};

class Compiler {
 public:
  Compiler(Backend *backend = 0);

  // Gets or sets compiler backend.
  Backend *backend() const { return backend_; }
  void set_backend(Backend *backend) { backend_ = backend; }

  // Compiles the specified AMX script. The optional error hander is either
  // never called or called only once - on first compilation error (if any).
  CompilerOutput *compile(AMXPtr amx, CompileErrorHandler *error_handler = 0);

 private:
  Backend *backend_;
  
 private:
  JIT_DISALLOW_COPY_AND_ASSIGN(Compiler);
};

class CompileErrorHandler {
public:
  virtual ~CompileErrorHandler() {}
  virtual void execute(const AMXInstruction &instr) = 0;
};

} // namespace jit

#endif // !JIT_COMPILER_H
