// Copyright (c) 2012-2018 Zeex
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

#ifndef AMXJIT_COMPILER_H
#define AMXJIT_COMPILER_H

#include "amxref.h"
#include "macros.h"

namespace amxjit {

class CompilerImpl;
class Instruction;
class Logger;

typedef int (AMXAPI *CodeEntryPoint)(cell index, cell *retval);

class CompileErrorHandler {
 public:
  virtual ~CompileErrorHandler() {}
  virtual void Execute(const Instruction &instr) = 0;
};

class CodeBuffer {
 public:
  CodeBuffer(void *code);
  virtual ~CodeBuffer();

  CodeEntryPoint GetEntryPoint() const;
  void Delete();

 private:
  void *code_;

 private:
  AMXJIT_DISALLOW_COPY_AND_ASSIGN(CodeBuffer);
};

class Compiler {
 public:
  Compiler();
  virtual ~Compiler();

  void SetLogger(Logger *logger);
  void SetErrorHandler(CompileErrorHandler *error_handler);
  void SetSysreqDEnabled(bool is_enabled);

  CodeBuffer *Compile(AMXRef amx);

 private:
  CompilerImpl *impl_;

 private:
  AMXJIT_DISALLOW_COPY_AND_ASSIGN(Compiler);
};

} // namespace amxjit

#endif // !AMXJIT_COMPILER_H
