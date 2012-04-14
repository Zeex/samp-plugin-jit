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

// This file is used to test crossed jumps.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AsmJit/AsmJit.h>

using namespace AsmJit;
typedef void (*VoidFn)();

int main(int, char**)
{
  Compiler c;

  FileLogger logger(stderr);
  c.setLogger(&logger);

  c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<Void>());

  Label L_A = c.newLabel();
  Label L_B = c.newLabel();
  Label L_C = c.newLabel();

  c.jmp(L_B);

  c.bind(L_A);
  c.jmp(L_C);

  c.bind(L_B);
  c.jmp(L_A);

  c.bind(L_C);

  c.ret();
  c.endFunction();

  VoidFn fn = function_cast<VoidFn>(c.make());

  // Ensure that everything is ok.
  if (!fn)
  {
    printf("Error making jit function (%u).\n", c.getError());
    return 1;
  }

  // Free the JIT function if it's not needed anymore.
  MemoryManager::getGlobal()->free((void*)fn);

  return 0;
}
