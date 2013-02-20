// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Dependencies - AsmJit]
#include <asmjit/core.h>

#if defined(ASMJIT_X86) || defined(ASMJIT_X64)
#include <asmjit/x86.h>
#endif // ASMJIT_X86 || ASMJIT_X64

// [Dependencies - C]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace AsmJit;

int main(int argc, char* argv[])
{
  printf("AsmJit size test\n");
  printf("================\n");
  printf("\n");

  // --------------------------------------------------------------------------
  // [C++]
  // --------------------------------------------------------------------------

  printf("Variable sizes:\n");
  printf("  uint8_t                    : %u\n", (uint32_t)sizeof(uint8_t));
  printf("  uint16_t                   : %u\n", (uint32_t)sizeof(uint16_t));
  printf("  uint32_t                   : %u\n", (uint32_t)sizeof(uint32_t));
  printf("  uint64_t                   : %u\n", (uint32_t)sizeof(uint64_t));
  printf("  size_t                     : %u\n", (uint32_t)sizeof(size_t));
  printf("  uintptr_t                  : %u\n", (uint32_t)sizeof(uintptr_t));
  printf("  float                      : %u\n", (uint32_t)sizeof(float));
  printf("  double                     : %u\n", (uint32_t)sizeof(double));
  printf("  void*                      : %u\n", (uint32_t)sizeof(void*));
  printf("\n");

  // --------------------------------------------------------------------------
  // [Core]
  // --------------------------------------------------------------------------

  printf("Structure sizes:\n");
  printf("  AsmJit::Operand            : %u\n", (uint32_t)sizeof(Operand));
  printf("\n");

  printf("  AsmJit::Assembler          : %u\n", (uint32_t)sizeof(Assembler));
  printf("  AsmJit::Compiler           : %u\n", (uint32_t)sizeof(Compiler));
  printf("  AsmJit::CompilerAlign      : %u\n", (uint32_t)sizeof(CompilerAlign));
  printf("  AsmJit::CompilerComment    : %u\n", (uint32_t)sizeof(CompilerComment));
  printf("  AsmJit::CompilerEmbed      : %u\n", (uint32_t)sizeof(CompilerEmbed));
  printf("  AsmJit::CompilerFuncCall   : %u\n", (uint32_t)sizeof(CompilerFuncCall));
  printf("  AsmJit::CompilerFuncDecl   : %u\n", (uint32_t)sizeof(CompilerFuncDecl));
  printf("  AsmJit::CompilerFuncEnd    : %u\n", (uint32_t)sizeof(CompilerFuncEnd));
  printf("  AsmJit::CompilerInst       : %u\n", (uint32_t)sizeof(CompilerInst));
  printf("  AsmJit::CompilerItem       : %u\n", (uint32_t)sizeof(CompilerItem));
  printf("  AsmJit::CompilerState      : %u\n", (uint32_t)sizeof(CompilerState));
  printf("  AsmJit::CompilerVar        : %u\n", (uint32_t)sizeof(CompilerVar));
  printf("  AsmJit::FuncArg            : %u\n", (uint32_t)sizeof(FuncArg));
  printf("  AsmJit::FuncDecl           : %u\n", (uint32_t)sizeof(FuncDecl));
  printf("  AsmJit::FuncPrototype      : %u\n", (uint32_t)sizeof(FuncPrototype));
  printf("\n");

  // --------------------------------------------------------------------------
  // [X86]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_X86) || defined(ASMJIT_X64)
  printf("  AsmJit::X86Assembler       : %u\n", (uint32_t)sizeof(X86Assembler));
  printf("  AsmJit::X86Compiler        : %u\n", (uint32_t)sizeof(X86Compiler));
  printf("  AsmJit::X86CompilerAlign   : %u\n", (uint32_t)sizeof(X86CompilerAlign));
  printf("  AsmJit::X86CompilerFuncCall: %u\n", (uint32_t)sizeof(X86CompilerFuncCall));
  printf("  AsmJit::X86CompilerFuncDecl: %u\n", (uint32_t)sizeof(X86CompilerFuncDecl));
  printf("  AsmJit::X86CompilerFuncEnd : %u\n", (uint32_t)sizeof(X86CompilerFuncEnd));
  printf("  AsmJit::X86CompilerInst    : %u\n", (uint32_t)sizeof(X86CompilerInst));
  printf("  AsmJit::X86CompilerJmpInst : %u\n", (uint32_t)sizeof(X86CompilerJmpInst));
  printf("  AsmJit::X86CompilerState   : %u\n", (uint32_t)sizeof(X86CompilerState));
  printf("  AsmJit::X86CompilerVar     : %u\n", (uint32_t)sizeof(X86CompilerVar));
  printf("  AsmJit::X86FuncDecl        : %u\n", (uint32_t)sizeof(X86FuncDecl));
  printf("\n");
#endif // ASMJIT_X86 || ASMJIT_X64

  return 0;
}
