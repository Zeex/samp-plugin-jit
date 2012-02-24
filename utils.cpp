// Copyright (c) 2012, Sergey Zolotarev
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// // LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <cstddef>
#include "amx.h"

namespace sampjit {

int FindPublic(AMX *amx, cell address) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	AMX_FUNCSTUBNT *publics = reinterpret_cast<AMX_FUNCSTUBNT*>(amx->base + hdr->publics);

	int num_publics = (hdr->natives - hdr->publics) / hdr->defsize;
	for (int index = 0; index < num_publics; index++) {
		if (publics[index].address == address) {
			return index;
		}
	}

	return -1;
}

bool IsMain(AMX *amx, cell address) {
	return (reinterpret_cast<AMX_HEADER*>(amx->base))->cip == address;
}

bool IsPublic(AMX *amx, cell address) {
	return IsMain(amx, address) || FindPublic(amx, address) >= 0;
}

ucell GetPublicAddress(AMX *amx, cell index) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

	AMX_FUNCSTUBNT *publics = reinterpret_cast<AMX_FUNCSTUBNT*>(amx->base + hdr->publics);
	int num_publics = (hdr->natives - hdr->publics) / hdr->defsize;

	if (index == -1) {
		return hdr->cip;
	}

	if (index < 0 || index >= num_publics) {
		return 0;
	}

	return publics[index].address;
}

ucell GetNativeAddress(AMX *amx, cell index) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

	AMX_FUNCSTUBNT *natives = reinterpret_cast<AMX_FUNCSTUBNT*>(amx->base + hdr->natives);
	int num_natives = (hdr->libraries - hdr->natives) / hdr->defsize;

	if (index < 0 || index >= num_natives) {
		return 0;
	}

	return natives[index].address;
}

} // namespace sampjit
