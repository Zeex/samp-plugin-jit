#ifndef JIT_AMXSCRIPT_H
#define JIT_AMXSCRIPT_H

#include <cstddef>
#include <amx/amx.h>

namespace jit {

class AMXScript {
public:
	AMXScript(AMX *amx);

public:
	operator AMX*() { return amx(); }
	AMX *operator->() { return amx(); }

public:
	AMX *amx() const {
		return amx_;
	}
	AMX_HEADER *hdr() const {
		return reinterpret_cast<AMX_HEADER*>(amx()->base);
	}

	unsigned char *code() const {
		return amx()->base + hdr()->cod;
	}
	std::size_t codeSize() const {
		return hdr()->dat - hdr()->cod;
	}

	unsigned char *data() const {
		return amx()->data != 0 ? amx()->data : amx()->base + hdr()->dat;
	}
	std::size_t dataSize() const {
		return hdr()->hea - hdr()->dat;
	}

	int numPublics() const {
		return (hdr()->natives - hdr()->publics) / hdr()->defsize;
	}
	int numNatives() const {
		return (hdr()->libraries - hdr()->natives) / hdr()->defsize;
	}

	AMX_FUNCSTUBNT *publics() const {
		return reinterpret_cast<AMX_FUNCSTUBNT*>(hdr()->publics + amx()->base);
	}
	AMX_FUNCSTUBNT *natives() const {
		return reinterpret_cast<AMX_FUNCSTUBNT*>(hdr()->natives + amx()->base);
	}

	cell getPublicAddr(int index) const;
	cell getNativeAddr(int index) const;

	const char *getPublicName(int index) const;
	const char *getNativeName(int index) const;

	int findPublic(cell address) const;
	int findNative(cell address) const;

public:
	cell *stack() const {
		return reinterpret_cast<cell*>(data() + amx()->stk);
	}
	cell stackSize() const {
		return hdr()->stp - hdr()->hea;
	}

	cell *push(cell value);
	cell pop();
	void pop(int ncells);

private:
	AMX *amx_;
};

} // namespace jit

#endif // !JIT_AMXSCRIPT_H
