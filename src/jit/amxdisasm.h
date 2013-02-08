#ifndef JIT_AMXDISASM_H
#define JIT_AMXDISASM_H

#include <cassert>
#include <string>
#include <vector>
#include <amx/amx.h>
#include "amxopcode.h"
#include "amxscript.h"

namespace jit {

// REG_NONE is defined in WinNT.h.
#ifdef REG_NONE
	#undef REG_NONE
#endif

enum AMXRegister {
	REG_NONE = 0,
	REG_PRI = (2 << 0),
	REG_ALT = (2 << 1),
	REG_COD = (2 << 2),
	REG_DAT = (2 << 3),
	REG_HEA = (2 << 4),
	REG_STP = (2 << 5),
	REG_STK = (2 << 6),
	REG_FRM = (2 << 7),
	REG_CIP = (2 << 8)
};

class AMXInstruction {
public:
	AMXInstruction();

public:
	int size() const {
		return sizeof(cell) * (1 + operands_.size());
	}

	const cell address() const {
		return address_;
	}
	void setAddress(cell address) {
		address_ = address;
	}

	AMXOpcode opcode() const {
		return opcode_;
	}
	void setOpcode(AMXOpcode opcode) {
		opcode_ = opcode;
	}

	cell operand(unsigned int index = 0u) {
		assert(index < operands_.size());
		return operands_[index];
	}
	std::vector<cell> &operands() {
		return operands_;
	}
	const std::vector<cell> &operands() const {
		return operands_;
	}
	void setOperands(std::vector<cell> operands) {
		operands_ = operands;
	}
	void addOperand(cell value) {
		operands_.push_back(value);
	}
	int numOperands() const {
		return operands_.size();
	}

public:
	const char *name() const;

	int getSrcRegs() const;
	int getDestRegs() const;

	std::string string() const;

private:
	cell address_;
	AMXOpcode opcode_;
	std::vector<cell> operands_;

private:
	struct StaticInfoTableEntry {
		const char *name;
		int srcRegs;
		int destRegs;
	};
	static const StaticInfoTableEntry info[NUM_AMX_OPCODES];
};

class AMXDisassembler {
public:
	AMXDisassembler(const AMXScript &amx);

public:
	// See JIT::setOpcodeMap() for details.
	void setOpcodeMap(cell *opcodeMap) { opcodeMap_ = opcodeMap; }

	// Returns address of currently executing instruction (instruction pointer).
	cell ip() const { return ip_; }

	// Sets instruction pointer.
	void setIp(cell ip) { ip_ = ip; }

public:
	// Decodes current instruction and returns true until the end of code gets
	// reached or an error occurs. The optional error argument is set to true
	// on error.
	bool decode(AMXInstruction &instr, bool *error = 0);

private:
	AMXScript amx_;
	cell *opcodeMap_;
	cell ip_;
};

} // namespace jit

#endif // !JIT_AMXDISASM_H