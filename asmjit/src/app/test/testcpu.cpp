// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Dependencies - AsmJit]
#include <asmjit/asmjit.h>

// [Dependencies - C]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace AsmJit;

struct BitDescription
{
  uint32_t mask;
  const char* description;
};

#if defined(ASMJIT_X86) || defined(ASMJIT_X64)
static const BitDescription x86Features[] = 
{
  { kX86FeatureRdtsc              , "RDTSC" },
  { kX86FeatureRdtscP             , "RDTSCP" },
  { kX86FeatureCMov               , "CMOV" },
  { kX86FeatureCmpXchg8B          , "CMPXCHG8B" },
  { kX86FeatureCmpXchg16B         , "CMPXCHG16B" },
  { kX86FeatureClFlush            , "CLFLUSH" },
  { kX86FeaturePrefetch           , "PREFETCH" },
  { kX86FeatureLahfSahf           , "LAHF/SAHF" },
  { kX86FeatureFXSR               , "FXSAVE/FXRSTOR" },
  { kX86FeatureFFXSR              , "FXSAVE/FXRSTOR Optimizations" },
  { kX86FeatureMmx                , "MMX" },
  { kX86FeatureMmxExt             , "MMX Extensions" },
  { kX86Feature3dNow              , "3dNow!" },
  { kX86Feature3dNowExt           , "3dNow! Extensions" },
  { kX86FeatureSse                , "SSE" },
  { kX86FeatureSse2               , "SSE2" },
  { kX86FeatureSse3               , "SSE3" },
  { kX86FeatureSsse3              , "SSSE3" },
  { kX86FeatureSse4A              , "SSE4A" },
  { kX86FeatureSse41              , "SSE4.1" },
  { kX86FeatureSse42              , "SSE4.2" },
  { kX86FeatureAvx                , "AVX" },
  { kX86FeatureMSse               , "Misaligned SSE" },
  { kX86FeatureMonitorMWait       , "MONITOR/MWAIT" },
  { kX86FeatureMovBE              , "MOVBE" },
  { kX86FeaturePopCnt             , "POPCNT" },
  { kX86FeatureLzCnt              , "LZCNT" },
  { kX86FeaturePclMulDQ           , "PCLMULDQ" },
  { kX86FeatureMultiThreading     , "Multi-Threading" },
  { kX86FeatureExecuteDisableBit  , "Execute-Disable Bit" },
  { kX86Feature64Bit              , "64-Bit Processor" },
  { 0, NULL }
};
#endif // ASMJIT_X86 || ASMJIT_X64

static void printBits(const char* msg, uint32_t mask, const BitDescription* d)
{
  for (; d->mask; d++)
  {
    if (mask & d->mask)
      printf("%s%s\n", msg, d->description);
  }
}

int main(int argc, char* argv[])
{
  const CpuInfo* cpu = CpuInfo::getGlobal();

  // --------------------------------------------------------------------------
  // [Core Features]
  // --------------------------------------------------------------------------

  printf("CPU Detection\n");
  printf("=============\n");

  printf("\nBasic info\n");
  printf("  Vendor string         : %s\n", cpu->getVendorString());
  printf("  Brand string          : %s\n", cpu->getBrandString());
  printf("  Family                : %u\n", cpu->getFamily());
  printf("  Model                 : %u\n", cpu->getModel());
  printf("  Stepping              : %u\n", cpu->getStepping());
  printf("  Number of Processors  : %u\n", cpu->getNumberOfProcessors());
  printf("  Features              : 0x%08X\n", cpu->getFeatures());
  printf("  Bugs                  : 0x%08X\n", cpu->getBugs());

  // --------------------------------------------------------------------------
  // [X86 Features]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_X86) || defined(ASMJIT_X64)
  const X86CpuInfo* x86Cpu = static_cast<const X86CpuInfo*>(cpu);

  printf("\nX86/X64 Extended Info:\n");
  printf("  Processor Type        : %u\n", x86Cpu->getProcessorType());
  printf("  Brand Index           : %u\n", x86Cpu->getBrandIndex());
  printf("  CL Flush Cache Line   : %u\n", x86Cpu->getFlushCacheLineSize());
  printf("  Max logical Processors: %u\n", x86Cpu->getMaxLogicalProcessors());
  printf("  APIC Physical ID      : %u\n", x86Cpu->getApicPhysicalId());

  printf("\nX86/X64 Features:\n");
  printBits("  ", cpu->getFeatures(), x86Features);
#endif // ASMJIT_X86 || ASMJIT_X64

  return 0;
}
