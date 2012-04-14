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

// This file is used to test AsmJit memory manager.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AsmJit/AsmJit.h>

static int problems = 0;

static void gen(void* a, void* b, int i)
{
  int pattern = rand() % 256;
  *(int *)a  = i;
  *(int *)b  = i;
  memset((char*)a + sizeof(int), pattern, i - sizeof(int));
  memset((char*)b + sizeof(int), pattern, i - sizeof(int));
}

static void verify(void* a, void* b)
{
  int ai = *(int*)a;
  int bi = *(int*)b;
  if (ai != bi || memcmp(a, b, ai) != 0)
  {
    printf("Failed to verify %p\n", a);
    problems++;
  }
}

static void die()
{
  printf("Couldn't allocate virtual memory, this test needs at least 100MB of free virtual memory.\n");
  exit(1);
}

static void stats(const char* dumpFile)
{
  AsmJit::MemoryManager* memmgr = AsmJit::MemoryManager::getGlobal();

  printf("-- Used: %d\n", (int)memmgr->getUsedBytes());
  printf("-- Allocated: %d\n", (int)memmgr->getAllocatedBytes());

#if defined(ASMJIT_MEMORY_MANAGER_DUMP)
  reinterpret_cast<AsmJit::VirtualMemoryManager*>(memmgr)->dump(dumpFile);
#endif // ASMJIT_MEMORY_MANAGER_DUMP
}

static void shuffle(void **a, void **b, sysuint_t count)
{
  for (sysuint_t i = 0; i < count; ++i)
  {
    sysuint_t si = (sysuint_t)rand() % count;

    void *ta = a[i];
    void *tb = b[i];

    a[i] = a[si];
    b[i] = b[si];

    a[si] = ta;
    b[si] = tb;
  }
}

int main(int argc, char* argv[])
{
  AsmJit::MemoryManager* memmgr = AsmJit::MemoryManager::getGlobal();

  sysuint_t i;
  sysuint_t count = 200000;

  printf("Memory alloc/free test - %d allocations\n\n", (int)count);

  void** a = (void**)malloc(sizeof(void*) * count);
  void** b = (void**)malloc(sizeof(void*) * count);
  if (!a || !b) die();

  srand(100);
  printf("Allocating virtual memory...");

  for (i = 0; i < count; i++)
  {
    int r = (rand() % 1000) + 4;

    a[i] = memmgr->alloc(r);
    if (a[i] == NULL) die();

    memset(a[i], 0, r);
  }

  printf("done\n");
  stats("dump0.dot");

  printf("\n");
  printf("Freeing virtual memory...");

  for (i = 0; i < count; i++)
  {
    if (!memmgr->free(a[i]))
    {
      printf("Failed to free %p\n", b[i]);
      problems++;
    }
  }

  printf("done\n");
  stats("dump1.dot");

  printf("\n");
  printf("Verified alloc/free test - %d allocations\n\n", (int)count);

  printf("Alloc...");
  for (i = 0; i < count; i++)
  {
    int r = (rand() % 1000) + 4;

    a[i] = memmgr->alloc(r);
    b[i] = malloc(r);
    if (a[i] == NULL || b[i] == NULL) die();

    gen(a[i], b[i], r);
  }
  printf("done\n");
  stats("dump2.dot");

  printf("\n");
  printf("Shuffling...");
  shuffle(a, b, count);
  printf("done\n");

  printf("\n");
  printf("Verify and free...");
  for (i = 0; i < count/2; i++)
  {
    verify(a[i], b[i]);
    if (!memmgr->free(a[i]))
    {
      printf("Failed to free %p\n", b[i]);
      problems++;
    }
    free(b[i]);
  }
  printf("done\n");
  stats("dump3.dot");

  printf("\n");
  printf("Alloc...");
  for (i = 0; i < count/2; i++)
  {
    int r = (rand() % 1000) + 4;

    a[i] = memmgr->alloc(r);
    b[i] = malloc(r);
    if (a[i] == NULL || b[i] == NULL) die();

    gen(a[i], b[i], r);
  }
  printf("done\n");
  stats("dump4.dot");

  printf("\n");
  printf("Verify and free...");
  for (i = 0; i < count; i++)
  {
    verify(a[i], b[i]);
    if (!memmgr->free(a[i]))
    {
      printf("Failed to free %p\n", b[i]);
      problems++;
    }
    free(b[i]);
  }
  printf("done\n");
  stats("dump5.dot");

  printf("\n");
  if (problems)
    printf("Status: Failure: %d problems found\n", problems);
  else
    printf("Status: Success\n");

  free(a);
  free(b);

  return 0;
}
