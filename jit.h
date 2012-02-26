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

#ifndef JIT_H
#define JIT_H

#include <iosfwd>
#include <map>
#include <string>

#include "amx/amx.h"
#include "jitasm/jitasm.h"

class JIT;

// JITFunction represents a JIT-compiled AMX function.
class JITFunction : public jitasm::function<void, JITFunction> {
public:
	JITFunction(JIT *jitter, ucell address);

	void main();

	void SetLabel(cell address, const std::string &tag = std::string());
	std::string GetLabelName(cell address, const std::string &tag = std::string()) const;

private:
	// Disable copying.
	JITFunction(const JITFunction &);
	JITFunction &operator=(const JITFunction &);

	JIT *jit_;
	ucell address_;
};

class JIT {
	friend class JITFunction;
public:
	typedef cell (CDECL *PublicFunction)();

	JIT(AMX *amx);
	~JIT();

	inline AMX        *GetAmx()       { return amx_; }
	inline AMX_HEADER *GetAmxHeader() { return amxhdr_; }

	// Get pointer to start of AMX data section.
	inline unsigned char *GetAmxData() { return data_; }

	// Get pointer to start of AMX code section.
	inline unsigned char *GetAmxCode() { return code_; }

	// Get assembled function (and assemble if needed).
	JITFunction *GetFunction(ucell address);

	// Assemble AMX function at the specified address.
	JITFunction *AssembleFunction(ucell address);

	// Call a public function (and assemble if needed).
	int CallPublicFunction(int index, cell *retval);

	// Output generated code to a stream.
	void DumpCode(std::ostream &stream) const;

private:
	// Disable copying.
	JIT(const JIT &);
	JIT &operator=(const JIT &);

private:
	AMX        *amx_;
	AMX_HEADER *amxhdr_;

	unsigned char *data_;
	unsigned char *code_;

	// proc_map_ maps AMX funtions to their JIT equivalents.
	typedef std::map<ucell, JITFunction*> ProcMap;
	ProcMap proc_map_;
};

#endif // !JIT_H
