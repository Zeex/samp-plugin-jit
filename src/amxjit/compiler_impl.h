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

#include <cstddef>
#include <map>
#include <asmjit/base.h>
#include <asmjit/x86.h>
#include "amxref.h"
#include "macros.h"

#ifndef AMXJIT_COMPILER_IMPL_H
#define AMXJIT_COMPILER_IMPL_H

namespace amxjit {

class CodeBuffer;
class CompileErrorHandler;
class Logger;
class Instruction;

class CompilerImpl {
 public:
  CompilerImpl();

  void SetLogger(Logger *logger) {
    logger_ = logger;
  }
  void SetErrorHandler(CompileErrorHandler *error_handler) {
    error_handler_ = error_handler;
  }
  void SetSysreqDEnabled(bool enable) {
    enable_sysreq_d_ = enable;
  }
  void SetDebugFlags(unsigned int flags) {
    debug_flags_ = flags;
  }

  CodeBuffer *Compile(AMXRef amx);

 private:
  bool EmitIntrinsic(const char *name);
  void float_();
  void floatabs();
  void floatadd();
  void floatsub();
  void floatmul();
  void floatdiv();
  void floatsqroot();
  void floatlog();
  void floatcmp();

  void clamp();
  void heapspace();
  void numargs();
  void min();
  void max();
  void swapchars();

 private:
  void EmitRuntimeInfo();
  void EmitInstrTable();
  void EmitExec();
  void EmitExecHelper();
  void EmitExecContHelper();
  void EmitHaltHelper();
  void EmitJumpLookup();
  void EmitReverseJumpLookup();
  void EmitJumpHelper();
  void EmitSysreqCHelper();
  void EmitSysreqDHelper();
  void EmitDebugPrint(const char *message);
  void EmitDebugBreakpoint();

 private:
  const asmjit::Label &GetLabel(cell address);

 private:
  AMXRef amx_;

  asmjit::X86Assembler asm_;
  asmjit::Label rib_start_label_;
  asmjit::Label exec_ptr_label_;
  asmjit::Label amx_ptr_label_;
  asmjit::Label ebp_label_;
  asmjit::Label esp_label_;
  asmjit::Label amx_ebp_label_;
  asmjit::Label amx_esp_label_;
  asmjit::Label reset_ebp_label_;
  asmjit::Label reset_esp_label_;
  asmjit::Label reset_stk_label_;
  asmjit::Label reset_hea_label_;
  asmjit::Label exec_label_;
  asmjit::Label exec_helper_label_;
  asmjit::Label exec_return_label_;
  asmjit::Label exec_cont_helper_label_;
  asmjit::Label halt_helper_label_;
  asmjit::Label jump_helper_label_;
  asmjit::Label jump_lookup_label_;
  asmjit::Label reverse_jump_lookup_label_;
  asmjit::Label sysreq_c_helper_label_;
  asmjit::Label sysreq_d_helper_label_;

  std::map<cell, asmjit::Label> label_map_;
  std::map<cell, std::ptrdiff_t> instr_map_;

  asmjit::Logger *asmjit_logger_;
  Logger *logger_;
  CompileErrorHandler *error_handler_;
  bool enable_sysreq_d_;
  unsigned int debug_flags_;
};

}  // namespace amxjit

#endif // !AMXJIT_COMPILER_IMPL_H
