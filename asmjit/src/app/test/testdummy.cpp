// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// This file is used as a dummy test. It's changed during development.

// [Dependencies - AsmJit]
#include <asmjit/asmjit.h>

// [Dependencies - C]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// This is type of function we will generate
typedef void (*MyFn)(void);

static void dummyFunc(void) {}

int main(int argc, char* argv[])
{
  using namespace AsmJit;

  // ==========================================================================
  // Log compiler output.
  FileLogger logger(stderr);
  logger.setLogBinary(true);

  // Create compiler.
  /*
  X86Compiler c;
  c.setLogger(&logger);

  c.newFunc(kX86FuncConvDefault, FuncBuilder0<Void>());
  c.getFunc()->setHint(kFuncHintNaked, true);

  X86CompilerFuncCall* ctx = c.call((void*)dummyFunc);
  ctx->setPrototype(kX86FuncConvDefault, FuncBuilder0<Void>());

  c.endFunc();
  */

  X86Compiler c;
  c.setLogger(&logger);

  c.newFunc(kX86FuncConvDefault, FuncBuilder0<void>());
  c.getFunc()->setHint(kFuncHintNaked, true);
  
  Label l91 = c.newLabel();
  Label l92 = c.newLabel();
  Label l93 = c.newLabel();
  Label l94 = c.newLabel();
  Label l95 = c.newLabel();
  Label l96 = c.newLabel();
  Label l97 = c.newLabel();
  c.bind(l92);

  GpVar _var91(c.newGpVar());
  GpVar _var92(c.newGpVar());
  
  c.bind(l93);
  c.jmp(l91);
  c.bind(l95);
  c.mov(_var91, imm(0));
  c.bind(l96);
  c.jmp(l93);
  c.mov(_var92, imm(1));
  c.jmp(l91);
  c.bind(l94);
  c.jmp(l92);
  c.bind(l97);
  c.add(_var91, _var92);
  c.bind(l91);
  c.ret();
  c.endFunc();
  
  typedef void (*Func9)(void);
  Func9 func9 = asmjit_cast<Func9>(c.make());
  // ==========================================================================

  // ==========================================================================
  // Make the function.
  // MyFn fn = asmjit_cast<MyFn>(c.make());

  // Call it.
  // printf("Result %llu\n", (unsigned long long)fn());

  // Free the generated function if it's not needed anymore.
  //MemoryManager::getGlobal()->free((void*)fn);
  // ==========================================================================

  return 0;
}
