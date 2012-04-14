// AsmJit - Complete JIT Assembler for C++ Language.

// Copyright (c) 2008-2012, Petr Kobalicek <kobalicek.petr@gmail.com>
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// this file is used to test cpu detection.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AsmJit/AsmJit.h>

using namespace AsmJit;

int main(int argc, char* argv[])
{
  printf("AsmJit size test\n");
  printf("================\n");
  printf("\n");

  printf("Variable sizes:\n");
  printf("  uint8_t                   : %u\n", (uint32_t)sizeof(uint8_t));
  printf("  uint16_t                  : %u\n", (uint32_t)sizeof(uint16_t));
  printf("  uint32_t                  : %u\n", (uint32_t)sizeof(uint32_t));
  printf("  uint64_t                  : %u\n", (uint32_t)sizeof(uint64_t));
  printf("  sysuint_t                 : %u\n", (uint32_t)sizeof(sysuint_t));
  printf("  void*                     : %u\n", (uint32_t)sizeof(void*));
  printf("\n");

  printf("Structure sizes:\n");
  printf("  AsmJit::Operand           : %u\n", (uint32_t)sizeof(Operand));
  printf("  AsmJit::Operand::BaseData : %u\n", (uint32_t)sizeof(Operand::BaseData));
  printf("  AsmJit::Operand::ImmData  : %u\n", (uint32_t)sizeof(Operand::ImmData));
  printf("  AsmJit::Operand::LblData  : %u\n", (uint32_t)sizeof(Operand::LblData));
  printf("  AsmJit::Operand::MemData  : %u\n", (uint32_t)sizeof(Operand::MemData));
  printf("  AsmJit::Operand::RegData  : %u\n", (uint32_t)sizeof(Operand::RegData));
  printf("  AsmJit::Operand::VarData  : %u\n", (uint32_t)sizeof(Operand::VarData));
  printf("  AsmJit::Operand::BinData  : %u\n", (uint32_t)sizeof(Operand::BinData));
  printf("\n");

  printf("  AsmJit::Assembler         : %u\n", (uint32_t)sizeof(Assembler));
  printf("  AsmJit::Compiler          : %u\n", (uint32_t)sizeof(Compiler));
  printf("  AsmJit::FunctionDefinition: %u\n", (uint32_t)sizeof(FunctionDefinition));
  printf("\n");

  printf("  AsmJit::Emittable         : %u\n", (uint32_t)sizeof(Emittable));
  printf("  AsmJit::EAlign            : %u\n", (uint32_t)sizeof(EAlign));
  printf("  AsmJit::ECall             : %u\n", (uint32_t)sizeof(ECall));
  printf("  AsmJit::EComment          : %u\n", (uint32_t)sizeof(EComment));
  printf("  AsmJit::EData             : %u\n", (uint32_t)sizeof(EData));
  printf("  AsmJit::EEpilog           : %u\n", (uint32_t)sizeof(EEpilog));
  printf("  AsmJit::EFunction         : %u\n", (uint32_t)sizeof(EFunction));
  printf("  AsmJit::EFunctionEnd      : %u\n", (uint32_t)sizeof(EFunctionEnd));
  printf("  AsmJit::EInstruction      : %u\n", (uint32_t)sizeof(EInstruction));
  printf("  AsmJit::EJmp              : %u\n", (uint32_t)sizeof(EJmp));
  printf("  AsmJit::EProlog           : %u\n", (uint32_t)sizeof(EProlog));
  printf("  AsmJit::ERet              : %u\n", (uint32_t)sizeof(ERet));
  printf("\n");

  printf("  AsmJit::VarData           : %u\n", (uint32_t)sizeof(VarData));
  printf("  AsmJit::VarAllocRecord    : %u\n", (uint32_t)sizeof(VarAllocRecord));
  printf("  AsmJit::StateData         : %u\n", (uint32_t)sizeof(StateData));
  printf("\n");

  return 0;
}
