// Copyright (c) 2012-2014 Zeex
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

#include <cassert>
#include "amxptr.h"

namespace amxjit {

AMX *AMXPtr::AccessAmx() {
  assert(amx_ != 0);
  return amx_;
}

const AMX *AMXPtr::AccessAmx() const {
  assert(amx_ != 0);
  return amx_;
}

AMX_HEADER *AMXPtr::header() const {
  return reinterpret_cast<AMX_HEADER*>(AccessAmx()->base);
}

unsigned char *AMXPtr::code() const {
  return AccessAmx()->base + header()->cod;
}

std::size_t AMXPtr::code_size() const {
  return header()->dat - header()->cod;
}

unsigned char *AMXPtr::data() const {
  return AccessAmx()->data != 0 ? AccessAmx()->data
                                : AccessAmx()->base + header()->dat;
}
std::size_t AMXPtr::data_size() const {
  return header()->hea - header()->dat;
}

int AMXPtr::num_publics() const {
  return (header()->natives - header()->publics)
          / header()->defsize;
}

int AMXPtr::num_natives() const {
  return (header()->libraries - header()->natives)
          / header()->defsize;
}

AMX_FUNCSTUBNT *AMXPtr::publics() const {
  return reinterpret_cast<AMX_FUNCSTUBNT*>(header()->publics
                                            + AccessAmx()->base);
}

AMX_FUNCSTUBNT *AMXPtr::natives() const {
  return reinterpret_cast<AMX_FUNCSTUBNT*>(header()->natives
                                            + AccessAmx()->base);
}

cell AMXPtr::GetPublicAddress(cell index) const {
  if (index == AMX_EXEC_MAIN) {
    AMX_HEADER *hdr = header();
    if (hdr->cip > 0) {
      return hdr->cip;
    }
  } else if (index >= 0 || index < num_publics()) {
    return publics()[index].address;
  }
  return 0;
}

cell AMXPtr::GetNativeAddress(cell index) const {
  if (index >= 0 && index < num_natives()) {
    return natives()[index].address;
  }
  return 0;
}

cell AMXPtr::FindPublic(cell address) const {
  int n = num_publics();
  AMX_FUNCSTUBNT *publics = this->publics();
  for (int i = 0; i < n; i++) {
    if (publics[i].address == static_cast<ucell>(address)) {
      return i;
    }
  }
  return -1;
}

cell AMXPtr::FindNative(cell address) const {
  int n = num_natives();
  AMX_FUNCSTUBNT *natives = this->natives();
  for (int i = 0; i < n; i++) {
    if (natives[i].address == static_cast<ucell>(address)) {
      return i;
    }
  }
  return -1;
}

const char *AMXPtr::GetPublicName(cell index) const {
  if (index >= 0 && index < num_publics()) {
    return reinterpret_cast<char*>(AccessAmx()->base
                                   + publics()[index].nameofs);
  }
  return 0;
}

const char *AMXPtr::GetNativeName(cell index) const {
  if (index >= 0 && index < num_natives()) {
    return reinterpret_cast<char*>(AccessAmx()->base
                                   + natives()[index].nameofs);
  }
  return 0;
}

} // namespace amxjit
