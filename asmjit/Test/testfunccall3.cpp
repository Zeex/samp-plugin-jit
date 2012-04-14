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
typedef int (*MyFn)(void);

// Function that is called inside the generated one. Because this test is 
// mainly about register arguments, we need to use the fastcall calling 
// convention under 32-bit mode.
static int oneFunc(void) { return 1; }

int main(int argc, char* argv[])
{
  using namespace AsmJit;

  // ==========================================================================
  // Create compiler.
  Compiler c;

  // Log compiler output.
  FileLogger logger(stderr);
  c.setLogger(&logger);

  c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder1<int,int>());
  c.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
  Label L0(c.newLabel());
  Label L1(c.newLabel());
  GPVar x(c.newGP());
  GPVar y(c.newGP());
  c.mov(x, 0);
  c.jnz(L0);
  c.mov(y, c.argGP(0));
  c.jmp(L1);
  c.bind(L0);
  ECall *ctx = c.call((void*)oneFunc);
  ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<int,int>());
  ctx->setArgument(0, c.argGP(0));
  ctx->setReturn(y);
  c.bind(L1);
  c.add(x, y);
  c.endFunction();

  // TODO: Remove
#if 0
  c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<int>());
  c.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
  GPVar x(c.newGP());
  GPVar y(c.newGP());
  ECall *ctx = c.call((void*)oneFunc);
  ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder0<int>());
  ctx->setReturn(y);
  c.mov(x, 0);
  c.add(x, y);
  c.ret(x);
  c.endFunction();
#endif
  // ==========================================================================

  // ==========================================================================
  // Make the function.
  MyFn fn = function_cast<MyFn>(c.make());

  //uint result = fn();
  //bool success = result == 1;

  //printf("Result %u (expected 1)\n", result);
  //printf("Status: %s\n", success ? "Success" : "Failure");

  // Free the generated function if it's not needed anymore.
  MemoryManager::getGlobal()->free((void*)fn);
  // ==========================================================================

  return 0;
}
