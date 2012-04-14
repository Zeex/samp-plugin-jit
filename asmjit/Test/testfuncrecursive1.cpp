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

// Recursive function call test.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AsmJit/AsmJit.h>

// Type of generated function.
typedef int (*MyFn)(int);

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
  Label skip(c.newLabel());

  EFunction* func = c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder1<int, int>());
  func->setHint(FUNCTION_HINT_NAKED, true);

  GPVar var(c.argGP(0));
  c.cmp(var, imm(1));
  c.jle(skip);

  GPVar tmp(c.newGP(VARIABLE_TYPE_INT32));
  c.mov(tmp, var);
  c.dec(tmp);

  ctx = c.call(func->getEntryLabel());
  ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<int, int>());
  ctx->setArgument(0, tmp);
  ctx->setReturn(tmp);
  c.mul(c.newGP(VARIABLE_TYPE_INT32), var, tmp);

  c.bind(skip);
  c.ret(var);
  c.endFunction();
  // ==========================================================================

  // ==========================================================================
  // Make the function.
  MyFn fn = function_cast<MyFn>(c.make());

  printf("Factorial 5 == %d\n", fn(5));

  // Free the generated function if it's not needed anymore.
  MemoryManager::getGlobal()->free((void*)fn);
  // ==========================================================================

  return 0;
}
