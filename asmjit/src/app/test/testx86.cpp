// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Dependencies - AsmJit]
#include <asmjit/x86.h>

// [Dependencies - C]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace AsmJit;

// ============================================================================
// [X86Test]
// ============================================================================

//! @brief Interface used to test X86Compiler.
struct X86Test
{
  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  virtual ~X86Test() {}
  virtual const char* getName() const = 0;

  virtual void compile(X86Compiler& c) = 0;
  virtual bool run(void* func, StringBuilder& output, StringBuilder& expected) = 0;
};

// ============================================================================
// [X86Test_FuncAlign]
// ============================================================================

struct X86Test_FuncAlign : public X86Test
{
  X86Test_FuncAlign(uint argsCount, uint varsCount, bool naked, bool pushPop) :
    _argsCount(argsCount),
    _varsCount(varsCount),
    _naked(naked),
    _pushPop(pushPop)
  {
    _name.setFormat("FuncAlign - Args=%u, Vars=%u, Naked=%s, PushPop=%s",
      argsCount,
      varsCount,
      naked   ? "true" : "false",
      pushPop ? "true" : "false");
  }

  virtual ~X86Test_FuncAlign() {}

  virtual const char* getName() const
  {
    return _name.getData();
  }

  virtual void compile(X86Compiler& c)
  {
    switch (_argsCount)
    {
      case 0: c.newFunc(kX86FuncConvDefault, FuncBuilder0<int>()); break;
      case 1: c.newFunc(kX86FuncConvDefault, FuncBuilder1<int, int>()); break;
      case 2: c.newFunc(kX86FuncConvDefault, FuncBuilder2<int, int, int>()); break;
      case 3: c.newFunc(kX86FuncConvDefault, FuncBuilder3<int, int, int, int>()); break;
    }

    if (_naked  ) c.getFunc()->setHint(kFuncHintNaked, true);
    if (_pushPop) c.getFunc()->setHint(kX86FuncHintPushPop, true);

    GpVar gvar(c.newGpVar());
    XmmVar xvar(c.newXmmVar(kX86VarTypeXmm));

    // Alloc, use and spill preserved registers.
    if (_varsCount)
    {
      uint var = 0;
      uint index = 0;
      uint mask = 1;
      uint preserved = c.getFunc()->getDecl()->getGpPreservedMask();

      do {
        if ((preserved & mask) != 0 && (index != kX86RegIndexEsp && index != kX86RegIndexEbp))
        {
          GpVar somevar(c.newGpVar(kX86VarTypeGpd));
          c.alloc(somevar, index);
          c.mov(somevar, imm(0));
          c.spill(somevar);
          var++;
        }

        index++;
        mask <<= 1;
      } while (var < _varsCount && index < kX86RegNumGp);
    }

    c.alloc(gvar, zax);
    c.lea(gvar, xvar.m());
    c.and_(gvar, imm(15));
    c.ret(gvar);
    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef intptr_t (*Func0)();
    typedef intptr_t (*Func1)(int);
    typedef intptr_t (*Func2)(int, int);
    typedef intptr_t (*Func3)(int, int, int);

    intptr_t resultRet = 0;
    intptr_t expectedRet = 0;

    switch (_argsCount)
    {
      case 0:
        resultRet = asmjit_cast<Func0>(_func)();
        break;
      case 1:
        resultRet = asmjit_cast<Func1>(_func)(1);
        break;
      case 2:
        resultRet = asmjit_cast<Func2>(_func)(1, 2);
        break;
      case 3:
        resultRet = asmjit_cast<Func3>(_func)(1, 2, 3);
        break;
    }

    result.setFormat("%i", static_cast<int>(resultRet));
    expected.setFormat("%i", static_cast<int>(expectedRet));

    return resultRet == expectedRet;
  }

  StringBuilder _name;
  uint _argsCount;
  uint _varsCount;

  bool _naked;
  bool _pushPop;
};

// ============================================================================
// [X86Test_Func1]
// ============================================================================

struct X86Test_Func1 : public X86Test
{
  virtual const char* getName() const { return "Func1 - Many function arguments"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, 
      FuncBuilder8<Void, void*, void*, void*, void*, void*, void*, void*, void*>());

    GpVar p1(c.getGpArg(0));
    GpVar p2(c.getGpArg(1));
    GpVar p3(c.getGpArg(2));
    GpVar p4(c.getGpArg(3));
    GpVar p5(c.getGpArg(4));
    GpVar p6(c.getGpArg(5));
    GpVar p7(c.getGpArg(6));
    GpVar p8(c.getGpArg(7));

    c.add(p1, 1);
    c.add(p2, 2);
    c.add(p3, 3);
    c.add(p4, 4);
    c.add(p5, 5);
    c.add(p6, 6);
    c.add(p7, 7);
    c.add(p8, 8);

    // Move some data into buffer provided by arguments so we can verify if it
    // really works without looking into assembler output.
    c.add(byte_ptr(p1), imm(1));
    c.add(byte_ptr(p2), imm(2));
    c.add(byte_ptr(p3), imm(3));
    c.add(byte_ptr(p4), imm(4));
    c.add(byte_ptr(p5), imm(5));
    c.add(byte_ptr(p6), imm(6));
    c.add(byte_ptr(p7), imm(7));
    c.add(byte_ptr(p8), imm(8));

    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef void (*Func)(void*, void*, void*, void*, void*, void*, void*, void*);
    Func func = asmjit_cast<Func>(_func);

    uint8_t resultBuf[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t expectedBuf[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

    func(resultBuf, resultBuf, resultBuf, resultBuf,
         resultBuf, resultBuf, resultBuf, resultBuf);

    result.setFormat("buf={%d, %d, %d, %d, %d, %d, %d, %d, %d}",
      resultBuf[0], resultBuf[1], resultBuf[2], resultBuf[3],
      resultBuf[4], resultBuf[5], resultBuf[6], resultBuf[7],
      resultBuf[8]);
    expected.setFormat("buf={%d, %d, %d, %d, %d, %d, %d, %d, %d}",
      expectedBuf[0], expectedBuf[1], expectedBuf[2], expectedBuf[3],
      expectedBuf[4], expectedBuf[5], expectedBuf[6], expectedBuf[7],
      expectedBuf[8]);

    return memcmp(resultBuf, expectedBuf, 9) == 0;
  }
};

// ============================================================================
// [X86Test_Func2]
// ============================================================================

struct X86Test_Func2 : public X86Test
{
  virtual const char* getName() const { return "Func2 - Register arguments handling (FastCall/X64)"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder1<int, int>());
    c.getFunc()->setHint(kFuncHintNaked, true);

    // Call a function.
    GpVar address(c.newGpVar());
    GpVar var(c.getGpArg(0));

    c.mov(address, imm((sysint_t)(void*)calledFunc));
    X86CompilerFuncCall* ctx;

    ctx = c.call(address);
    ctx->setPrototype(kX86FuncConvCompatFastCall, FuncBuilder1<int, int>());
    ctx->setArgument(0, var);
    ctx->setReturn(var);

    ctx = c.call(address);
    ctx->setPrototype(kX86FuncConvCompatFastCall, FuncBuilder1<int, int>());
    ctx->setArgument(0, var);
    ctx->setReturn(var);

    c.ret(var);
    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef int (*Func)(int);
    Func func = asmjit_cast<Func>(_func);

    int resultRet = func(9);
    int expectedRet = 9*9 * 9*9;

    result.setFormat("ret=%d", resultRet);
    expected.setFormat("ret=%d", expectedRet);

    return resultRet == expectedRet;
  }

  // Function that is called inside the generated one. Because this test is 
  // mainly about register arguments, we need to use the fastcall calling 
  // convention under 32-bit mode.
  static int ASMJIT_FASTCALL calledFunc(int a) { return a * a; }
};

// ============================================================================
// [X86Test_Func3]
// ============================================================================

struct X86Test_Func3 : public X86Test
{
  virtual const char* getName() const { return "Func3 - Memcpy32 function"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder3<Void, uint32_t*, const uint32_t*, size_t>());
    c.getFunc()->setHint(kFuncHintNaked, true);    // Omit unnecessary prolog/epilog.

    Label L_Loop = c.newLabel();                   // Create base labels we use
    Label L_Exit = c.newLabel();                   // in our function.

    GpVar dst(c.getGpArg(0));                      // Get reference to function
    GpVar src(c.getGpArg(1));                      // arguments.
    GpVar cnt(c.getGpArg(2));

    c.alloc(dst);                                  // Allocate all registers now,
    c.alloc(src);                                  // because we want to keep them
    c.alloc(cnt);                                  // in physical registers only.

    c.test(cnt, cnt);                              // Exit if length is zero.
    c.jz(L_Exit);

    c.bind(L_Loop);                                // Bind the loop label here.

    GpVar tmp(c.newGpVar(kX86VarTypeGpd));         // Copy a single dword (4 bytes).
    c.mov(tmp, dword_ptr(src));
    c.mov(dword_ptr(dst), tmp);

    c.add(src, 4);                                 // Increment dst/src pointers.
    c.add(dst, 4);

    c.dec(cnt);                                    // Loop until cnt isn't zero.
    c.jnz(L_Loop);

    c.bind(L_Exit);                                // Bind the exit label here.
    c.endFunc();                                   // End of function.
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef void (*Func)(uint32_t*, const uint32_t*, size_t);
    Func func = asmjit_cast<Func>(_func);

    enum { kBufferSize = 32 };
    uint32_t dstBuffer[kBufferSize];
    uint32_t srcBuffer[kBufferSize];
  
    uint i;
    for (i = 0; i < kBufferSize; i++)
    {
      dstBuffer[i] = 0;
      srcBuffer[i] = i;
    }
    
    func(dstBuffer, srcBuffer, kBufferSize);

    result.setString("buf={");
    expected.setString("buf={");

    for (i = 0; i < kBufferSize; i++)
    {
      if (i != 0)
      {
        result.appendString(", ");
        expected.appendString(", ");
      }

      result.appendFormat("%u", static_cast<uint>(dstBuffer[i]));
      expected.appendFormat("%u", static_cast<uint>(srcBuffer[i]));
    }

    result.appendString("}");
    expected.appendString("}");

    return memcmp(dstBuffer, srcBuffer, kBufferSize * sizeof(uint32_t)) == 0;
  }
};

// ============================================================================
// [X86Test_Func4]
// ============================================================================

struct X86Test_Func4 : public X86Test
{
  virtual const char* getName() const { return "Func4 - Simple function call"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder3<int, int, int, int>());

    GpVar v0(c.getGpArg(0));
    GpVar v1(c.getGpArg(1));
    GpVar v2(c.getGpArg(2));

    // Just do something;)
    c.shl(v0, imm(1));
    c.shl(v1, imm(1));
    c.shl(v2, imm(1));

    // Call function.
    GpVar address(c.newGpVar());
    c.mov(address, imm((sysint_t)(void*)calledFunc));

    X86CompilerFuncCall* fCall = c.call(address);
    fCall->setPrototype(kX86FuncConvDefault, FuncBuilder3<Void, int, int, int>());
    fCall->setArgument(0, v2);
    fCall->setArgument(1, v1);
    fCall->setArgument(2, v0);

    // TODO: FuncCall return value.
    // fCall->setReturn(v0);
    // c.ret(v0);

    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef int (*Func)(int, int, int);
    Func func = asmjit_cast<Func>(_func);

    int resultRet = func(3, 2, 1);
    int expectedRet = 36;

    result.setFormat("ret=%d", resultRet);
    expected.setFormat("ret=%d", expectedRet);

    return resultRet == expectedRet;
  }

  static int calledFunc(int a, int b, int c) { return (a + b) * c; }
};

// ============================================================================
// [X86Test_Func5]
// ============================================================================

struct X86Test_Func5 : public X86Test
{
  virtual const char* getName() const { return "Func5 - Conditional function call"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder3<int, int, int, int>());

    GpVar x(c.getGpArg(0));
    GpVar y(c.getGpArg(1));
    GpVar op(c.getGpArg(2));

    GpVar result;
    X86CompilerFuncCall* fCall;

    Label opAdd(c.newLabel());
    Label opMul(c.newLabel());

    c.cmp(op, 0);
    c.jz(opAdd);
    c.cmp(op, 1);
    c.jz(opMul);

    result = c.newGpVar();
    c.mov(result, imm(0));
    c.ret(result);

    c.bind(opAdd);
    result = c.newGpVar();

    fCall = c.call((void*)calledFuncAdd);
    fCall->setPrototype(kX86FuncConvDefault, FuncBuilder2<int, int, int>());
    fCall->setArgument(0, x);
    fCall->setArgument(1, y);
    fCall->setReturn(result);
    c.ret(result);

    c.bind(opMul);
    result = c.newGpVar();

    fCall = c.call((void*)calledFuncMul);
    fCall->setPrototype(kX86FuncConvDefault, FuncBuilder2<int, int, int>());
    fCall->setArgument(0, x);
    fCall->setArgument(1, y);
    fCall->setReturn(result);
    c.ret(result);

    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef int (*Func)(int, int, int);
    Func func = asmjit_cast<Func>(_func);

    int arg1 = 4;
    int arg2 = 8;

    int resultAdd = func(arg1, arg2, 0);
    int expectedAdd = calledFuncAdd(arg1, arg2);

    int resultMul = func(arg1, arg2, 1);
    int expectedMul = calledFuncMul(arg1, arg2);

    result.setFormat("add=%d, mul=%d", resultAdd, resultMul);
    expected.setFormat("add=%d, mul=%d", expectedAdd, expectedMul);

    return (resultAdd == expectedAdd) && (resultMul == expectedMul);
  }

  static int calledFuncAdd(int x, int y) { return x + y; }
  static int calledFuncMul(int x, int y) { return x * y; }
};

// ============================================================================
// [X86Test_Func6]
// ============================================================================

struct X86Test_Func6 : public X86Test
{
  virtual const char* getName() const { return "Func6 - Recursive function call"; }

  virtual void compile(X86Compiler& c)
  {
    Label skip(c.newLabel());

    X86CompilerFuncDecl* func = c.newFunc(kX86FuncConvDefault, FuncBuilder1<int, int>());
    func->setHint(kFuncHintNaked, true);

    GpVar var(c.getGpArg(0));
    c.cmp(var, imm(1));
    c.jle(skip);

    GpVar tmp(c.newGpVar(kX86VarTypeInt32));
    c.mov(tmp, var);
    c.dec(tmp);

    X86CompilerFuncCall* fCall = c.call(func->getEntryLabel());
    fCall->setPrototype(kX86FuncConvDefault, FuncBuilder1<int, int>());
    fCall->setArgument(0, tmp);
    fCall->setReturn(tmp);
    c.mul(c.newGpVar(kX86VarTypeInt32), var, tmp);

    c.bind(skip);
    c.ret(var);
    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef int (*Func)(int);
    Func func = asmjit_cast<Func>(_func);

    int resultRet = func(5);
    int expectedRet = 1 * 2 * 3 * 4 * 5;

    result.setFormat("ret=%d", resultRet);
    expected.setFormat("ret=%d", expectedRet);

    return resultRet == expectedRet;
  }
};

// ============================================================================
// [X86Test_Func7]
// ============================================================================

struct X86Test_Func7 : public X86Test
{
  virtual const char* getName() const { return "Func7 - Many function calls"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvCompatFastCall, FuncBuilder1<int, int*>());
    c.getFunc()->setHint(kFuncHintNaked, true);

    GpVar buf(c.getGpArg(0));
    GpVar acc0(c.newGpVar(kX86VarTypeGpd));
    GpVar acc1(c.newGpVar(kX86VarTypeGpd));

    c.mov(acc0, 0);
    c.mov(acc1, 0);

    uint i;
    for (i = 0; i < 4; i++)
    {
      {
        GpVar ret = c.newGpVar(kX86VarTypeGpd);
        GpVar ptr = c.newGpVar(kX86VarTypeGpz);
        GpVar idx = c.newGpVar(kX86VarTypeGpd);

        c.mov(ptr, buf);
        c.mov(idx, imm(i));

        X86CompilerFuncCall* fCall = c.call((void*)calledFunc);
        fCall->setPrototype(kX86FuncConvCompatFastCall, FuncBuilder2<int, int*, int>());
        fCall->setArgument(0, ptr);
        fCall->setArgument(1, idx);
        fCall->setReturn(ret);

        c.add(acc0, ret);
      }

      {
        GpVar ret = c.newGpVar(kX86VarTypeGpd);
        GpVar ptr = c.newGpVar(kX86VarTypeGpz);
        GpVar idx = c.newGpVar(kX86VarTypeGpd);

        c.mov(ptr, buf);
        c.mov(idx, imm(i));

        X86CompilerFuncCall* fCall = c.call((void*)calledFunc);
        fCall->setPrototype(kX86FuncConvCompatFastCall, FuncBuilder2<int, int*, int>());
        fCall->setArgument(0, ptr);
        fCall->setArgument(1, idx);
        fCall->setReturn(ret);

        c.sub(acc1, ret);
      }
    }

    GpVar ret(c.newGpVar());
    c.mov(ret, acc0);
    c.add(ret, acc1);
    c.ret(ret);
    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef int (*Func)(int*);
    Func func = asmjit_cast<Func>(_func);

    int buffer[4] = { 127, 87, 23, 17 };

    int resultRet = func(buffer);
    int expectedRet = 0;

    result.setFormat("ret=%d", resultRet);
    expected.setFormat("ret=%d", expectedRet);

    return resultRet == expectedRet;
  }

  static int ASMJIT_FASTCALL calledFunc(int* pInt, int index)
  {
    return pInt[index];
  }
};

// ============================================================================
// [X86Test_Jump1]
// ============================================================================

struct X86Test_Jump1 : public X86Test
{
  virtual const char* getName() const { return "Jump1 - Unrecheable code"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder0<Void>());

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
    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef void (*Func)(void);
    Func func = asmjit_cast<Func>(_func);

    func();
    return true;
  }
};

// ============================================================================
// [X86Test_Special1]
// ============================================================================

struct X86Test_Special1 : public X86Test
{
  virtual const char* getName() const { return "Special1 - imul"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder4<Void, int*, int*, int, int>());
    c.getFunc()->setHint(kFuncHintNaked, true);

    GpVar dst0_hi(c.getGpArg(0));
    GpVar dst0_lo(c.getGpArg(1));

    GpVar v0_hi(c.newGpVar(kX86VarTypeGpd));
    GpVar v0_lo(c.getGpArg(2));

    GpVar src0(c.getGpArg(3));
    c.imul(v0_hi, v0_lo, src0);

    c.mov(dword_ptr(dst0_hi), v0_hi);
    c.mov(dword_ptr(dst0_lo), v0_lo);
    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef void (*Func)(int*, int*, int, int);
    Func func = asmjit_cast<Func>(_func);

    int v0 = 4;
    int v1 = 4;

    int resultHi;
    int resultLo;

    int expectedHi = 0;
    int expectedLo = v0 * v1;

    func(&resultHi, &resultLo, v0, v1);

    result.setFormat("hi=%d, lo=%d", resultHi, resultLo);
    expected.setFormat("hi=%d, lo=%d", expectedHi, expectedLo);

    return resultHi == expectedHi && resultLo == expectedLo;
  }
};

// ============================================================================
// [X86Test_Special2]
// ============================================================================

struct X86Test_Special2 : public X86Test
{
  virtual const char* getName() const { return "Special2 - shl and ror"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder4<Void, int*, int, int, int>());

    GpVar dst0(c.getGpArg(0));
    GpVar v0(c.getGpArg(1));

    c.shl(v0, c.getGpArg(2));
    c.ror(v0, c.getGpArg(3));
    
    c.mov(dword_ptr(dst0), v0);
    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef void (*Func)(int*, int, int, int);
    Func func = asmjit_cast<Func>(_func);

    int v0 = 0x000000FF;

    int resultRet;
    int expectedRet = 0x0000FF00;

    func(&resultRet, v0, 16, 8);

    result.setFormat("ret=%d", resultRet);
    expected.setFormat("ret=%d", expectedRet);

    return resultRet == expectedRet;
  }
};

// ============================================================================
// [X86Test_Special3]
// ============================================================================

struct X86Test_Special3 : public X86Test
{
  virtual const char* getName() const { return "Special3 - rep movsb"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder3<Void, void*, void*, size_t>());
    c.getFunc()->setHint(kFuncHintNaked, true);

    GpVar dst(c.getGpArg(0));
    GpVar src(c.getGpArg(1));
    GpVar cnt(c.getGpArg(2));

    c.rep_movsb(dst, src, cnt);
    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef void (*Func)(void*, void*, size_t);
    Func func = asmjit_cast<Func>(_func);

    char dst[20];
    char src[20] = "Hello AsmJit!";
    func(dst, src, strlen(src) + 1);

    result.setFormat("ret=\"%s\"", dst);
    expected.setFormat("ret=\"%s\"", src);

    return ::memcmp(dst, src, strlen(src) + 1) == 0;
  }
};

// ============================================================================
// [X86Test_Special4]
// ============================================================================

struct X86Test_Special4 : public X86Test
{
  virtual const char* getName() const { return "Special4 - setz"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder3<Void, int, int, char*>());
    c.getFunc()->setHint(kFuncHintNaked, true);

    GpVar src0(c.getGpArg(0));
    GpVar src1(c.getGpArg(1));
    GpVar dst0(c.getGpArg(2));

    c.cmp(src0, src1);
    c.setz(byte_ptr(dst0));

    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef void (*Func)(int, int, char*);
    Func func = asmjit_cast<Func>(_func);

    char resultBuf[4];
    char expectedBuf[4] = { 1, 0, 0, 1 };

    func(0, 0, &resultBuf[0]); // We are expecting 1 (0 == 0).
    func(0, 1, &resultBuf[1]); // We are expecting 0 (0 != 1).
    func(1, 0, &resultBuf[2]); // We are expecting 0 (1 != 0).
    func(1, 1, &resultBuf[3]); // We are expecting 1 (1 == 1).

    result.setFormat("out={%d, %d, %d, %d}", resultBuf[0], resultBuf[1], resultBuf[2], resultBuf[3]);
    expected.setFormat("out={%d, %d, %d, %d}", expectedBuf[0], expectedBuf[1], expectedBuf[2], expectedBuf[3]);

    return resultBuf[0] == expectedBuf[0] &&
           resultBuf[1] == expectedBuf[1] &&
           resultBuf[2] == expectedBuf[2] &&
           resultBuf[3] == expectedBuf[3] ;
  }
};

// ============================================================================
// [X86Test_Special5]
// ============================================================================

struct X86Test_Special5 : public X86Test
{
  virtual const char* getName() const { return "Special5 - Complex imul"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder2<Void, int*, const int*>());
    c.getFunc()->setHint(kFuncHintNaked, true);

    GpVar dst = c.getGpArg(0);
    GpVar src = c.getGpArg(1);

    for (uint i = 0; i < 4; i++)
    {
      GpVar x = c.newGpVar(kX86VarTypeGpd);
      GpVar y = c.newGpVar(kX86VarTypeGpd);
      GpVar hi = c.newGpVar(kX86VarTypeGpd);

      c.mov(x, dword_ptr(src, 0));
      c.mov(y, dword_ptr(src, 4));

      c.imul(hi, x, y);
      c.add(dword_ptr(dst, 0), hi);
      c.add(dword_ptr(dst, 4), x);
    }

    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef void (*Func)(int*, const int*);
    Func func = asmjit_cast<Func>(_func);

    int src[2] = { 4, 9 };
    int resultRet[2] = { 0, 0 };
    int expectedRet[2] = { 0, (4 * 9) * 4 };

    func(resultRet, src);

    result.setFormat("ret={%d, %d}", resultRet[0], resultRet[1]);
    expected.setFormat("ret={%d, %d}", expectedRet[0], expectedRet[1]);

    return resultRet[0] == expectedRet[0] && resultRet[1] == expectedRet[1];
  }
};

// ============================================================================
// [X86Test_Var1]
// ============================================================================

struct X86Test_Var1 : public X86Test
{
  virtual const char* getName() const { return "Var1 - Simple register alloc/spill"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder0<int>());

    GpVar v0(c.newGpVar(kX86VarTypeGpd));
    GpVar v1(c.newGpVar(kX86VarTypeGpd));
    GpVar v2(c.newGpVar(kX86VarTypeGpd));
    GpVar v3(c.newGpVar(kX86VarTypeGpd));
    GpVar v4(c.newGpVar(kX86VarTypeGpd));

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
    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef int (*Func)(void);
    Func func = asmjit_cast<Func>(_func);

    int resultRet = func();
    int expectedRet = 1 + 2 + 3 + 4;

    result.setFormat("ret=%d", resultRet);
    expected.setFormat("ret=%d", expectedRet);

    return resultRet == expectedRet;
  }
};

// ============================================================================
// [X86Test_Var2]
// ============================================================================

struct X86Test_Var2 : public X86Test
{
  virtual const char* getName() const { return "Var2 - Controlled register alloc/spill"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder0<int>());
    c.getFunc()->setHint(kFuncHintNaked, false);

    GpVar v0(c.newGpVar(kX86VarTypeGpd));
    GpVar v1(c.newGpVar(kX86VarTypeGpd));
    GpVar cnt(c.newGpVar(kX86VarTypeGpd));

    c.xor_(v0, v0);
    c.xor_(v1, v1);
    c.spill(v0);
    c.spill(v1);

    Label L(c.newLabel());
    c.mov(cnt, imm(32));
    c.bind(L);

    c.inc(v1);
    c.add(v0, v1);

    c.dec(cnt);
    c.jnz(L);

    c.ret(v0);
    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef int (*Func)(void);
    Func func = asmjit_cast<Func>(_func);

    int resultRet = func();
    int expectedRet = 
      0  +  1 +  2 +  3 +  4 +  5 +  6 +  7 +  8 +  9 +
      10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 +
      20 + 21 + 22 + 23 + 24 + 25 + 26 + 27 + 28 + 29 +
      30 + 31 + 32;

    result.setFormat("ret=%d", resultRet);
    expected.setFormat("ret=%d", expectedRet);

    return resultRet == expectedRet;
  }
};

// ============================================================================
// [X86Test_Var3]
// ============================================================================

struct X86Test_Var3 : public X86Test
{
  virtual const char* getName() const { return "Var3 - 8 variables at once"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder2<Void, int*, int*>());

    // Function arguments.
    GpVar a1(c.getGpArg(0));
    GpVar a2(c.getGpArg(1));

    // Create some variables.
    GpVar x1(c.newGpVar(kX86VarTypeGpd));
    GpVar x2(c.newGpVar(kX86VarTypeGpd));
    GpVar x3(c.newGpVar(kX86VarTypeGpd));
    GpVar x4(c.newGpVar(kX86VarTypeGpd));
    GpVar x5(c.newGpVar(kX86VarTypeGpd));
    GpVar x6(c.newGpVar(kX86VarTypeGpd));
    GpVar x7(c.newGpVar(kX86VarTypeGpd));
    GpVar x8(c.newGpVar(kX86VarTypeGpd));

    GpVar t(c.newGpVar(kX86VarTypeGpd));

    // Setup variables (use mov with reg/imm to se if register allocator works).
    c.mov(x1, 1);
    c.mov(x2, 2);
    c.mov(x3, 3);
    c.mov(x4, 4);
    c.mov(x5, 5);
    c.mov(x6, 6);
    c.mov(x7, 7);
    c.mov(x8, 8);

    // Make sum (addition).
    c.xor_(t, t);
    c.add(t, x1);
    c.add(t, x2);
    c.add(t, x3);
    c.add(t, x4);
    c.add(t, x5);
    c.add(t, x6);
    c.add(t, x7);
    c.add(t, x8);

    // Store result to a given pointer in first argument.
    c.mov(dword_ptr(a1), t);

    // Make sum (subtraction).
    c.xor_(t, t);
    c.sub(t, x1);
    c.sub(t, x2);
    c.sub(t, x3);
    c.sub(t, x4);
    c.sub(t, x5);
    c.sub(t, x6);
    c.sub(t, x7);
    c.sub(t, x8);

    // Store result to a given pointer in second argument.
    c.mov(dword_ptr(a2), t);

    // End of function.
    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef void (*Func)(int*, int*);
    Func func = asmjit_cast<Func>(_func);

    int resultX;
    int resultY;

    int expectedX =  36;
    int expectedY = -36;

    func(&resultX, &resultY);

    result.setFormat("x=%d, y=%d", resultX, resultY);
    expected.setFormat("x=%d, y=%d", expectedX, expectedY);

    return resultX == expectedX && resultY == expectedY;
  }
};

// ============================================================================
// [X86Test_Var4]
// ============================================================================

struct X86Test_Var4 : public X86Test
{
  virtual const char* getName() const { return "Var4 - 32 variables at once"; }

  virtual void compile(X86Compiler& c)
  {
    int i;
    GpVar var[32];

    c.newFunc(kX86FuncConvDefault, FuncBuilder1<Void, int*>());

    for (i = 0; i < ASMJIT_ARRAY_SIZE(var); i++)
    {
      var[i] = c.newGpVar(kX86VarTypeGpd);
      c.xor_(var[i], var[i]);
    }

    GpVar v0(c.newGpVar(kX86VarTypeGpd));
    Label L(c.newLabel());

    c.mov(v0, imm(32));
    c.bind(L);

    for (i = 0; i < ASMJIT_ARRAY_SIZE(var); i++)
    {
      c.add(var[i], imm(i));
    }

    c.dec(v0);
    c.jnz(L);

    GpVar a0(c.getGpArg(0));
    for (i = 0; i < ASMJIT_ARRAY_SIZE(var); i++)
    {
      c.mov(dword_ptr(a0, i * 4), var[i]);
    }

    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef void (*Func)(int*);
    Func func = asmjit_cast<Func>(_func);

    int i;
    int resultBuf[32];
    int expectedBuf[32];

    for (i = 0; i < ASMJIT_ARRAY_SIZE(resultBuf); i++)
      expectedBuf[i] = i * 32;

    bool success = true;
    func(resultBuf);

    for (i = 0; i < ASMJIT_ARRAY_SIZE(resultBuf); i++)
    {
      result.appendFormat("%d", resultBuf[i]);
      expected.appendFormat("%d", expectedBuf[1]);

      success &= (resultBuf[i] == expectedBuf[i]);
    }

    return success;
  }
};

// ============================================================================
// [X86Test_Dummy]
// ============================================================================

struct X86Test_Dummy : public X86Test
{
  virtual const char* getName() const { return "Dummy - Used to write new tests"; }

  virtual void compile(X86Compiler& c)
  {
    c.newFunc(kX86FuncConvDefault, FuncBuilder0<Void>());
    c.endFunc();
  }

  virtual bool run(void* _func, StringBuilder& result, StringBuilder& expected)
  {
    typedef void (*Func)(void);
    Func func = asmjit_cast<Func>(_func);

    func();
    return true;
  }
};

// ============================================================================
// [X86TestSuite]
// ============================================================================

struct X86TestSuite
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  X86TestSuite();
  ~X86TestSuite();

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  void run();

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  PodVector<X86Test*> testList;
  StringBuilder testOutput;

  int result;
};

X86TestSuite::X86TestSuite() :
  result(EXIT_SUCCESS)
{
  uint i, j;

  // --------------------------------------------------------------------------
  // [FuncAlign]
  // --------------------------------------------------------------------------

  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 4; j++)
    {
      // Performed always. If function is not naked (this means that standard
      // prolog/epilog sequence is always generated) then the alignment should
      // be always possible.
      if (true)
      {
        testList.append(new X86Test_FuncAlign(i, j, false, false));
        testList.append(new X86Test_FuncAlign(i, j, false, true ));
      }

      // If stack is aligned to 16 bytes by default then naked function should
      // support this alignment as well. We didn't use X64 mode only, because
      // on Linux/BSD the stack is aligned to 16 bytes by default (ABI). On
      // Windows the situation is a bit different, because it's not in ABI,
      // but some compilers (MinGW) perform the stack alignment.
      if (CompilerUtil::isStack16ByteAligned())
      {
        testList.append(new X86Test_FuncAlign(i, j, true, false));
        testList.append(new X86Test_FuncAlign(i, j, true, true ));
      }
    }
  }

  // --------------------------------------------------------------------------
  // [Func]
  // --------------------------------------------------------------------------

  testList.append(new X86Test_Func1());
  testList.append(new X86Test_Func2());
  testList.append(new X86Test_Func3());
  testList.append(new X86Test_Func4());
  testList.append(new X86Test_Func5());
  testList.append(new X86Test_Func6());
  testList.append(new X86Test_Func7());
  
  // --------------------------------------------------------------------------
  // [Jump]
  // --------------------------------------------------------------------------
  
  testList.append(new X86Test_Jump1());

  // --------------------------------------------------------------------------
  // [Special]
  // --------------------------------------------------------------------------
  
  testList.append(new X86Test_Special1());
  testList.append(new X86Test_Special2());
  testList.append(new X86Test_Special3());
  testList.append(new X86Test_Special4());
  testList.append(new X86Test_Special5());

  // --------------------------------------------------------------------------
  // [Var]
  // --------------------------------------------------------------------------

  testList.append(new X86Test_Var1());
  testList.append(new X86Test_Var2());
  testList.append(new X86Test_Var3());
  testList.append(new X86Test_Var4());

  // --------------------------------------------------------------------------
  // [Dummy]
  // --------------------------------------------------------------------------

  testList.append(new X86Test_Dummy());
}

X86TestSuite::~X86TestSuite()
{
  size_t i;
  size_t testCount = testList.getLength();

  for (i = 0; i < testCount; i++)
  {
    X86Test* test = testList[i];
    delete test;
  }
}

void X86TestSuite::run()
{
  size_t i;
  size_t testCount = testList.getLength();

  for (i = 0; i < testCount; i++)
  {
    X86Compiler compiler;
    StringLogger logger;

    logger.setLogBinary(true);
    compiler.setLogger(&logger);

    X86Test* test = testList[i];
    test->compile(compiler);

    void *func = compiler.make();

    // In case that compilation fails uncomment this section to log immediately
    // after "compiler.make()".
    //
    // fprintf(stdout, "%s\n", logger.getString());
    // fflush(stdout);

    if (func != NULL)
    {
      StringBuilder output;
      StringBuilder expected;

      if (test->run(func, output, expected))
      {
        fprintf(stdout, "[Success] %s.\n", test->getName());
      }
      else
      {
        fprintf(stdout, "[Failure] %s.\n", test->getName());
        fprintf(stdout, "-------------------------------------------------------------------------------\n");
        fprintf(stdout, "%s", logger.getString());
        fprintf(stdout, "\n");
        fprintf(stdout, "Result  : %s\n", output.getData());
        fprintf(stdout, "Expected: %s\n", expected.getData());
        fprintf(stdout, "-------------------------------------------------------------------------------\n");
      }

      MemoryManager::getGlobal()->free(func);
    }
    else
    {
      fprintf(stdout, "[Failure] %s.\n", test->getName());
      fprintf(stdout, "-------------------------------------------------------------------------------\n");
      fprintf(stdout, "%s\n", logger.getString());
      fprintf(stdout, "-------------------------------------------------------------------------------\n");
    }

    fflush(stdout);
  }

  fputs("\n", stdout);
  fputs(testOutput.getData(), stdout);
  fflush(stdout);
}

int main(int argc, char* argv[])
{
  X86TestSuite testSuite;
  testSuite.run();

  return testSuite.result;
}
