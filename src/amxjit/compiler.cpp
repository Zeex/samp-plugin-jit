// Copyright (c) 2012-2019 Zeex
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
#include "compiler.h"
#include "compiler_impl.h"

namespace amxjit {

Compiler::Compiler():
  impl_(new CompilerImpl)
{
}

Compiler::~Compiler() {
  delete impl_;
}

void Compiler::SetLogger(Logger *logger) {
  impl_->SetLogger(logger);
}

void Compiler::SetErrorHandler(CompileErrorHandler *error_handler) {
  impl_->SetErrorHandler(error_handler);
}

void Compiler::SetSysreqDEnabled(bool flag) {
  impl_->SetSysreqDEnabled(flag);
}

void Compiler::SetSleepEnabled(bool flag) {
  impl_->SetSleepEnabled(flag);
}

void Compiler::SetDebugFlags(unsigned int flags) {
  impl_->SetDebugFlags(flags);
}

CodeBuffer *Compiler::Compile(AMXRef amx) {
  return impl_->Compile(amx);
}

} // namespace amxjit
