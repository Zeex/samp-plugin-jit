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
typedef int (*MyFn)(int, int, int);

// Function that is called inside the generated one.
static int calledFn(int a, int b, int c) { return (a + b) * c; }

int main(int argc, char* argv[])
{
  using namespace AsmJit;

  // ==========================================================================
  // Create compiler.
  Compiler c;

  // Log compiler output.
  FileLogger logger(stderr);
  c.setLogger(&logger);

  c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder3<int, int, int, int>());

  GPVar v0(c.argGP(0));
  GPVar v1(c.argGP(1));
  GPVar v2(c.argGP(2));

  // Just do something;)
  c.shl(v0, imm(1));
  c.shl(v1, imm(1));
  c.shl(v2, imm(1));

  // Call function.
  GPVar address(c.newGP());
  c.mov(address, imm((sysint_t)(void*)calledFn));

  ECall* ctx = c.call(address);
  ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder3<Void, int, int, int>());
  ctx->setArgument(0, v2);
  ctx->setArgument(1, v1);
  ctx->setArgument(2, v0);

  //ctx->setReturn(v0);
  //c.ret(v0);

  c.endFunction();
  // ==========================================================================

  // ==========================================================================
  // Make the function.
  MyFn fn = function_cast<MyFn>(c.make());

  uint result = fn(3, 2, 1);
  bool success = result == 36;

  printf("Result %u (expected 36)\n", result);
  printf("Status: %s\n", success ? "Success" : "Failure");

  // Free the generated function if it's not needed anymore.
  MemoryManager::getGlobal()->free((void*)fn);
  // ==========================================================================

  return 0;
}
