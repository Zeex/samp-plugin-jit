// Copyright (c) 2014 Zeex
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

#ifndef AMXJIT_CSTDINT_H
#define AMXJIT_CSTDINT_H

#if defined __GNUC__ || (defined _MSC_VER && _MSC_VER >= 1600)
  #if !defined HAVE_STDINT_H
    #define HAVE_STDINT_H 1
  #endif
#endif

#if defined HAVE_STDINT_H
  #include <stdint.h>
#endif

namespace amxjit {

#if defined HAVE_STDINT_H
  using ::int8_t;
  using ::uint8_t;
  using ::int16_t;
  using ::uint16_t;
  using ::int32_t;
  using ::uint32_t;
  using ::int64_t;
  using ::uint64_t;
#else
  #if defined _WIN32
    typedef signed   __int8  int8_t;
    typedef unsigned __int8  uint8_t;
    typedef signed   __int16 int16_t;
    typedef unsigned __int16 uint16_t;
    typedef signed   __int32 int32_t;
    typedef unsigned __int32 uint32_t;
    typedef signed   __int64 int64_t;
    typedef unsigned __int64 uint64_t;
  #endif
#endif

typedef int32_t  intptr_t;
typedef uint32_t uintptr_t;

} // namespace amxjit

#endif // !AMXJIT_CSTDINT_H
