#include "amxscript.h"

namespace jit {

AMXScript::AMXScript(AMX *amx)
	: amx_(amx)
{
}

cell AMXScript::getPublicAddr(cell index) const {
	if (index == AMX_EXEC_MAIN) {
		return hdr()->cip;
	}
	if (index < 0 || index >= numPublics()) {
		return 0;
	}
	return publics()[index].address;
}

cell AMXScript::getNativeAddr(int index) const {
	if (index < 0 || index >= numNatives()) {
		return 0;
	}
	return natives()[index].address;
}

int AMXScript::findPublic(cell address) const {
	for (int i = 0; i < numPublics(); i++) {
		if (publics()[i].address == address) {
			return i;
		}
	}
	return -1;
}

int AMXScript::findNative(cell address) const {
	for (int i = 0; i < numNatives(); i++) {
		if (natives()[i].address == address) {
			return i;
		}
	}
	return -1;
}

const char *AMXScript::getPublicName(int index) const {
	if (index < 0 || index >= numPublics()) {
		return 0;
	}
	return reinterpret_cast<char*>(amx_->base + publics()[index].nameofs);
}

const char *AMXScript::getNativeName(int index) const {
	if (index < 0 || index >= numNatives()) {
		return 0;
	}
	return reinterpret_cast<char*>(amx_->base + natives()[index].nameofs);
}

cell *AMXScript::push(cell value) {
	amx_->stk -= sizeof(cell);
	cell *s = stack();
	*s = value;
	return s;
}

cell AMXScript::pop() {
	cell *s = stack();
	amx_->stk += sizeof(cell);
	return *s;
}

void AMXScript::pop(int ncells) {
	amx_->stk += ncells * sizeof(cell);
}

} // namespace jit
