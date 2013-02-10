// Copyright (c) 2013 Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef JIT_MACROS_H
#define JIT_MACROS_H

#if defined __GNUC__
  #include <features.h> // __GNUC_PREREQ()
#endif

#if defined __GNUC__
  #if __GNUC_PREREQ(4,4)
    #define JIT_DELETE = delete
  #endif
  #if __GNUC_PREREQ(4,7)
    #define JIT_OVERRIDE override
    #define JIT_FINAL final
  #endif
#endif

#if defined _MSC_VER
  #if _MSC_VER >= 1700
    #define JIT_OVERRIDE override
    #define JIT_FINAL final
  #endif
#endif

#if !defined JIT_OVERRIDE
  #define JIT_OVERRIDE
#endif
#if !defined JIT_FINAL
  #define JIT_FINAL
#endif
#if !defined JIT_DELETE
  #define JIT_DELETE
#endif

#define JIT_FINAL_OVERRIDE JIT_FINAL JIT_OVERRIDE

#define JIT_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName &) JIT_DELETE; \
  void operator=(const TypeName &) JIT_DELETE

#endif // !JIT_MACROS_H
