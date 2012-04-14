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

// This is type of function we will generate
typedef sysint_t (*MyFn)();

int main(int argc, char* argv[])
{
  using namespace AsmJit;

  // ==========================================================================
  // Create compiler.
  Compiler c;

  // Log compiler output.
  FileLogger logger(stderr);
  c.setLogger(&logger);

  c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<sysint_t>());

  GPVar v0(c.newGP());
  GPVar v1(c.newGP());
  GPVar v2(c.newGP());
  GPVar v3(c.newGP());
  GPVar v4(c.newGP());

  c.xor_(v0, v0);

  c.mov(v1, 1);
  c.mov(v2, 2);
  c.mov(v3, 3);
  c.mov(v4, 4);

  c.add(v0, v1);
  c.add(v0, v2);
  c.add(v0, v3);
  c.add(v0, v4);

  c.ret(v0);
  c.endFunction();
  // ==========================================================================

  // ==========================================================================
  // Make the function.
  MyFn fn = function_cast<MyFn>(c.make());
  int result = (int)fn();

  printf("Result from JIT function: %d\n", result);
  printf("Status: %s\n", result == 10 ? "Success" : "Failure");

  // Free the generated function if it's not needed anymore.
  MemoryManager::getGlobal()->free((void*)fn);
  // ==========================================================================

  return 0;
}
