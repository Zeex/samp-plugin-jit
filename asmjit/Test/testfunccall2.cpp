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

// This file is used as a function call test (calling functions inside
// the generated code).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AsmJit/AsmJit.h>

// Type of generated function.
typedef void (*MyFn)(void);

// Function that is called inside the generated one. Because this test is 
// mainly about register arguments, we need to use the fastcall calling 
// convention under 32-bit mode.
static void ASMJIT_FASTCALL simpleFn(int a) {}

int main(int argc, char* argv[])
{
  using namespace AsmJit;

  // ==========================================================================
  // Create compiler.
  Compiler c;

  // Log compiler output.
  FileLogger logger(stderr);
  c.setLogger(&logger);

  ECall* ctx;

  c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<Void>());
  c.getFunction()->setHint(FUNCTION_HINT_NAKED, true);

  // Call a function.
  GPVar address(c.newGP());
  GPVar argument(c.newGP(VARIABLE_TYPE_GPD));

  c.mov(address, imm((sysint_t)(void*)simpleFn));

  c.mov(argument, imm(1));
  ctx = c.call(address);
  ctx->setPrototype(CALL_CONV_COMPAT_FASTCALL, FunctionBuilder1<Void, int>());
  ctx->setArgument(0, argument);
  c.unuse(argument);

  c.mov(argument, imm(2));
  ctx = c.call(address);
  ctx->setPrototype(CALL_CONV_COMPAT_FASTCALL, FunctionBuilder1<Void, int>());
  ctx->setArgument(0, argument);

  c.endFunction();
  // ==========================================================================

  // ==========================================================================
  // Make the function.
  MyFn fn = function_cast<MyFn>(c.make());

  fn();

  // Free the generated function if it's not needed anymore.
  MemoryManager::getGlobal()->free((void*)fn);
  // ==========================================================================

  return 0;
}
