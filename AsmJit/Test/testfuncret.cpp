// AsmJit - Complete JIT Assembler for C++ Language.

// Copyright (c) 2008-2009, Petr Kobalicek <kobalicek.petr@gmail.com>
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

// This file is used to test function with many arguments. Bug originally
// reported by Tilo Nitzsche for X64W and X64U calling conventions.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AsmJit/AsmJit.h>
// This is type of function we will generate.
typedef int (*MyFn)(int, int, int);

static int FuncA(int x, int y) { return x + y; }
static int FuncB(int x, int y) { return x * y; }

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
  {
    GPVar x(c.argGP(0));
    GPVar y(c.argGP(1));
    GPVar op(c.argGP(2));

    Label opAdd(c.newLabel());
    Label opMul(c.newLabel());

    c.cmp(op, 0);
    c.jz(opAdd);

    c.cmp(op, 1);
    c.jz(opMul);

    {
      GPVar result(c.newGP());
      c.mov(result, imm(0));
      c.ret(result);
    }

    {
      c.bind(opAdd);

      GPVar result(c.newGP());
      ECall* ctx = c.call((void*)FuncA);
      ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int, int, int>());
      ctx->setArgument(0, x);
      ctx->setArgument(1, y);
      ctx->setReturn(result);
      c.ret(result);
    }

    {
      c.bind(opMul);

      GPVar result(c.newGP());
      ECall* ctx = c.call((void*)FuncB);
      ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int, int, int>());
      ctx->setArgument(0, x);
      ctx->setArgument(1, y);
      ctx->setReturn(result);
      c.ret(result);
    }
  }
  c.endFunction();
  // ==========================================================================

  // ==========================================================================
  // Make the function.
  MyFn fn = function_cast<MyFn>(c.make());
  int result = fn(4, 8, 1);

  printf("Result from JIT function: %d (Expected 32) \n", result);
  printf("Status: %s\n", result == 32 ? "Success" : "Failure");

  // Free the generated function if it's not needed anymore.
  MemoryManager::getGlobal()->free((void*)fn);
  // ==========================================================================

  return 0;
}
