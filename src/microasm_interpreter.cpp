#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream> // Add this header for std::stringstream
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>
#include "heap.h"
#include "microasm_compiler.h"
#include "operand_types.h"
std::vector<std::string> mniCallStack;
#define VERSION 2
// Include own header FIRST
#include "microasm_interpreter.h"

// Global registry for MNI functions (define only in ONE .cpp file)
std::map<std::string, MniFunctionType> mniRegistry;

std::unordered_map<int, std::string> lbls; // known labels

void Interpreter::writeToOperand(BytecodeOperand op, int val, int size) {
    switch (op.type)
    {
        case OperandType::LABEL_ADDRESS:
        case OperandType::IMMEDIATE:
        case OperandType::NONE:
            std::cerr << "Attempted to write to a immediate value" << std::endl;
            break;
        
        case OperandType::REGISTER:
            registers[getRegisterIndex(op)] = val;
            break;

        case OperandType::REGISTER_AS_ADDRESS:
        case OperandType::MATH_OPERATOR:
        case OperandType::DATA_ADDRESS:
            writeRamNum(getRamAddr(op), val, size);
            break;
    }
}

int Interpreter::getRamAddr(BytecodeOperand op) {
    switch (op.type)
    {
        case OperandType::LABEL_ADDRESS:
        case OperandType::IMMEDIATE:
        case OperandType::NONE:
        case OperandType::REGISTER:
            throw std::runtime_error("Cannot get ram address for register/immediate");
            break;

        case OperandType::REGISTER_AS_ADDRESS:
            return registers[op.value];

        case OperandType::DATA_ADDRESS:
            return op.value;
        case OperandType::MATH_OPERATOR:
            return getAdvancedAddr(op);
    }
    throw std::runtime_error("Cannot get ram address for unknown");
}

// Define the MNI registration function
void registerMNI(const std::string &module, const std::string &name,
                 MniFunctionType func) {
    std::string fullName = module + "." + name;
    if (mniRegistry.find(fullName) == mniRegistry.end()) {
        mniRegistry[fullName] = func;
        // Optional: Keep debug print here if desired
        // std::cout << "Registered MNI function: " << fullName << std::endl;
    } else {
        std::cerr << "Warning: MNI function " << fullName
                  << " already registered." << std::endl;
    }
}

// --- Interpreter Method Definitions ---

Interpreter::Interpreter(int ramSize, const std::vector<std::string> &args,
                         bool debug, bool trace)
    : registers(24, 0), ram(ramSize, 0), cmdArgs(args), debugMode(debug),
      stackTrace(trace) {
    // Initialize Stack Pointer (RSP, index 7) to top of RAM
    registers[7] = ramSize;
    sp = registers[7];
    // Base Pointer (RBP, index 6) is initalized to zero. Normal this will be
    // set to junk but we can set it to zero.
    registers[6] = 0;
    bp = registers[6];
    // Set data segment base - let's put it in the middle for now
    // Ensure this doesn't collide with stack growing down!

    // Initialize MNI functions
    initializeMNIFunctions();

    // Initialize Heap
    heap_init();

    if (debugMode)
        std::cout << "[Debug][Interpreter] Debug mode enabled. RAM Size: "
                  << ramSize << "\n";
}

int Interpreter::getOperandSize(char type) {
    if (type == '\0') {
        return 1;
    }
    int ret = type >> 4;
    if (ret == 0 && (type & 0xF) == 6)
        ret = 3;
    else if (ret == 0)
        ret = 4;
    return ret;
}

BytecodeOperand Interpreter::nextRawOperand() {
    int size = getOperandSize(bytecode_raw[ip]);
    if (ip + 1 + size >
        bytecode_raw.size()) { // Check size for type byte + value int
        throw std::runtime_error(
            "Unexpected end of bytecode reading typed operand (IP: " +
            std::to_string(ip) +
            ", CodeSize: " + std::to_string(bytecode_raw.size()) + ")");
    }
    BytecodeOperand operand;
    operand.type = static_cast<OperandType>(bytecode_raw[ip++] & 15);
    // Handle NONE type immediately if needed (though it shouldn't be read here
    // usually)
    if (operand.type == OperandType::NONE) {
        operand.value = 0; // No value associated
        // ip adjustment might depend on how NONE is encoded (e.g., if it has a
        // dummy value) Assuming it's just the type byte based on compiler code
        // for MNI marker
    } else {
        if (bytecode_raw[ip-1] == 6) {
            operand.use_reg = true;
        }
        if (ip + size > bytecode_raw.size()) {
            throw std::runtime_error(
                "Unexpected end of bytecode reading operand value (IP: " +
                std::to_string(ip) + ")");
        }
                        operand.value =  (bytecode_raw[ip]);
        if (size >= 2) {operand.value += (bytecode_raw[ip+1] << 8);}
        if (size >= 3) {operand.value += (bytecode_raw[ip+2] << 16);}
        if (size >= 4) {operand.value += (bytecode_raw[ip+3] << 24);}
        if (size >= 5) {operand.value += ((long long)bytecode_raw[ip+4] << 32);}
        if (size == 6) {operand.value += ((long long)bytecode_raw[ip+5] << 40);}

        if (size != 4)
            operand.value &= (1 << (8 * size)) - 1;
        ip += size;
    }
    return operand;
}

int Interpreter::getAdvancedAddr(BytecodeOperand operand) {
    long long data = operand.value;
    int reg = data & 0xFF;
    MathOperatorOperators math_op = (MathOperatorOperators)(data >> 8 & 0xFF);
    int other_val = data >> 16;
    int v1 = registers[reg];
    int v2;
    if (operand.use_reg) v2 = registers[other_val]; else v2 = other_val;
    int ret = 0;
    switch (math_op)
    {
        case op_ADD:
            ret = v1 + v2;
            break;
        case op_SUB:
            ret = v1 - v2;
            break;
        case op_MUL:
            ret = v1 * v2;
            break;
        case op_DIV:
            ret = v1 / v2;
            break;
        case op_BDIV:
            ret = v2 / v1;
            break;
        case op_LSR:
            ret = v1 >> v2;
            break;
        case op_LSL:
            ret = v1 << v2;
            break;
        case op_AND:
            ret = v1 & v2;
            break;
        case op_OR:
            ret = v1 | v2;
            break;
        case op_XOR:
            ret = v1 ^ v2;
            break;
        case op_BSUB:
            ret = v2 - v1;
            break;
        case op_BLSR:
            ret = v2 >> v1;
            break;
        case op_BLSL:
            ret = v2 << v1;
            break;
        case op_NONE:
            throw std::runtime_error("uhhhhhhh");
            break;
    }
    return ret;
} 

int Interpreter::getValue(const BytecodeOperand &operand, int size) {
    switch (operand.type) {
        case OperandType::LABEL_ADDRESS:
        case OperandType::IMMEDIATE:
        case OperandType::NONE:
            return operand.value;
            break;
        
        case OperandType::REGISTER:
            return registers[getRegisterIndex(operand)];
            break;

        case OperandType::REGISTER_AS_ADDRESS:
        case OperandType::MATH_OPERATOR:
        case OperandType::DATA_ADDRESS:
            return readRamNum(getRamAddr(operand), size);
            break;

    default:
        throw std::runtime_error(
            "Cannot get value for unknown or invalid operand type: " +
            std::to_string(static_cast<int>(operand.type)));
    }
}

int Interpreter::getRegisterIndex(const BytecodeOperand &operand) {
    if (operand.type != OperandType::REGISTER) {
        throw std::runtime_error(
            "Expected register operand, got type " +
            std::to_string(static_cast<int>(operand.type)));
    }
    if (operand.value < 0 || operand.value >= registers.size()) {
        throw std::runtime_error("Invalid register index encountered: " +
                                 std::to_string(operand.value));
    }
    return operand.value;
}

int Interpreter::readRamInt(int address) {
    if (address < 0 || address + sizeof(int) > ram.size()) {
        throw std::runtime_error("Memory read out of bounds at address: " +
                                 std::to_string(address));
    }
    return *reinterpret_cast<int *>(&ram[address]);
}

void Interpreter::writeRamInt(int address, int value) {
    if (address < 0 || address + sizeof(int) > ram.size()) {
        throw std::runtime_error("Memory write out of bounds at address: " +
                                 std::to_string(address));
    }
    *reinterpret_cast<int *>(&ram[address]) = value;
}


int Interpreter::readRamNum(int address, int size) {
    if (address < 0 || address + sizeof(int) > ram.size()) {
        throw std::runtime_error("Memory read out of bounds at address: " +
                                 std::to_string(address));
    }
    unsigned int    ret =  (unsigned char)(ram[address]);
    if (size >= 2) {ret += (ram[address+1] << 8);}
    if (size >= 3) {ret += (ram[address+2] << 16);}
    if (size >= 4) {ret += (ram[address+3] << 24);}
    return (int)ret;
}

void Interpreter::writeRamNum(int address, int value, int size) {
    if (address < 0 || address + sizeof(int) > ram.size()) {
        throw std::runtime_error("Memory write out of bounds at address: " +
                                 std::to_string(address));
    }
                    ram[address] = value & 0xFF;
    if (size >= 2) {ram[address+1] = (value << 8) & 0xFF;}
    if (size >= 3) {ram[address+2] = (value << 16) & 0xFF;}
    if (size >= 4) {ram[address+3] = (value << 24) & 0xFF;}
}

char Interpreter::readRamChar(int address) {
    if (address < 0 || address >= ram.size()) {
        throw std::runtime_error("Memory read out of bounds at address: " +
                                 std::to_string(address));
    }
    return ram[address];
}

void Interpreter::writeRamChar(int address, char value) {
    if (address < 0 || address >= ram.size()) {
        throw std::runtime_error("Memory write out of bounds at address: " +
                                 std::to_string(address));
    }
    ram[address] = value;
}

std::string Interpreter::readRamString(int address) {
    std::string str = "";
    int currentAddr = address;
    while (true) {
        char c = readRamChar(currentAddr++);
        if (c == '\0')
            break;
        str += c;
    }
    return str;
}

void Interpreter::pushStack(int value) {
    registers[7] -= sizeof(int); // Decrement RSP (stack grows down)
    sp = registers[7];
    writeRamInt(sp, value);
}

int Interpreter::popStack() {
    int value = readRamInt(sp);
    registers[7] += sizeof(int); // Increment RSP
    sp = registers[7];
    return value;
}

std::string Interpreter::readBytecodeString() {
    std::string str = "";
    while (ip < bytecode_raw.size()) {
        char c = static_cast<char>(bytecode_raw[ip++]);
        if (c == '\0') {
            break;
        }
        str += c;
    }
    // Consider adding a check if ip reached end without null terminator
    return str;
}

void Interpreter::initializeMNIFunctions() {
    // Example: Math.sin R1 R2 (R1=input reg, R2=output reg)
    registerMNI(
        "Math", "sin",
        [](Interpreter &machine, const std::vector<BytecodeOperand> &args) {
            if (args.size() != 2)
                throw std::runtime_error(
                    "Math.sin requires 2 arguments (srcReg, destReg)");
            int srcReg = machine.getRegisterIndex(args[0]);
            int destReg = machine.getRegisterIndex(args[1]);
            // Note: Using double for calculation, storing as int
            machine.registers[destReg] = static_cast<int>(
                std::sin(static_cast<double>(machine.registers[srcReg])));
        });

    // Example: IO.write 1 R1 (Port=1/2, R1=address of null-terminated string in
    // RAM)
    registerMNI(
        "IO", "write",
        [](Interpreter &machine, const std::vector<BytecodeOperand> &args) {
            if (args.size() != 2)
                throw std::runtime_error(
                    "IO.write requires 2 arguments (port, addressReg/Imm)");
            int port =
                machine.getValue(args[0], 4); // Port can be immediate or register
            int address = machine.getValue(args[1], 4); // Address MUST resolve to a RAM location
            if (args[1].type != OperandType::REGISTER &&
                args[1].type != OperandType::DATA_ADDRESS &&
                args[1].type != OperandType::IMMEDIATE) {
                throw std::runtime_error("IO.write address argument must be "
                                         "register or data address");
            }

            std::ostream &out_stream = (port == 2) ? std::cerr : std::cout;
            if (port != 1 && port != 2)
                throw std::runtime_error("Invalid port for IO.write: " +
                                         std::to_string(port));

            out_stream << machine.readRamString(
                address); // Read null-terminated string from RAM
        });

    // this is a test of the recursive call's mni functions can do
    // call without arguments from MNI then call it'self and then die on the
    // 20th call.
    registerMNI(
        "Test", "recursiveCall",
        [](Interpreter &machine, const std::vector<BytecodeOperand> &args) {
            machine.pushStack(42);
            int value = machine.popStack();
            machine.registers[0] = value;
        });

    registerMNI(
        "Test", "recursiveCallbreaker",
        [](Interpreter &machine, const std::vector<BytecodeOperand> &args) {
            if (args.size() != 1)
                throw std::runtime_error(
                    "Test.recursiveCallbreaker requires 1 argument (count)");
            int count = machine.getValue(args[0], 4);
            if (count <= 0) {
                std::cout << "Recursive call limit reached. Exiting."
                          << std::endl;
                throw std::runtime_error(
                    "Test.recursiveCallbreaker reached max recursion depth: " +
                    std::to_string(count));
                return;
            }
            // Call the recursive function with decremented count
            for (int i = 0; i < count; ++i) {
                machine.callMNI("Test.recursiveCall", {});
            }
            // Call the breaker function again with decremented count
            machine.callMNI(
                "Test.recursiveCallbreaker",
                {BytecodeOperand{OperandType::IMMEDIATE, count - 1}});
            // Note: This will eventually lead to a stack overflow if not
            // careful which is what we want
        });

    // Memory.allocate R1 R2 (R1=size, R2=destReg for address) - Needs heap
    // management! StringOperations.concat R1 R2 R3 (R1=addr1, R2=addr2,
    // R3=destAddr) - Needs memory allocation!
}

std::string Interpreter::formatOperandDebug(const BytecodeOperand &op) {
    std::stringstream ss;
    ss << "T:0x" << std::hex << static_cast<int>(op.type) << std::dec;
    ss << " V:" << op.value;
    switch (op.type) {
    case OperandType::REGISTER:
        ss << "(R" << op.value << ")";
        break;
    case OperandType::REGISTER_AS_ADDRESS:
        ss << "($R" << op.value << ")";
        break;
    case OperandType::IMMEDIATE:
        ss << "(Imm)";
        break;
    case OperandType::LABEL_ADDRESS:
        ss << "(LblAddr)";
        break;
    case OperandType::MATH_OPERATOR:
        ss << "(MathOperator)";
        break;
    default:
        ss << "(?)";
        break;
    }
    return ss.str();
}

void Interpreter::load(const std::string &bytecodeFile) {
    std::ifstream in(bytecodeFile, std::ios::binary);
    if (!in)
        throw std::runtime_error("Failed to open bytecode file: " +
                                 bytecodeFile);

    // 1. Read the header
    BinaryHeader header;
    if (!in.read(reinterpret_cast<char *>(&header), sizeof(header))) {
        throw std::runtime_error("Failed to read header from bytecode file: " +
                                 bytecodeFile);
    }

    // 2. Validate the header (basic magic number check)
    if (header.magic != 0x4D53414D) { // "MASM"
        throw std::runtime_error(
            "Invalid magic number in bytecode file. Not a MASM binary.");
    }
    // Could add version checks here too
    if (header.version > VERSION) {
        throw std::runtime_error(
            "Unsupported bytecode version: " + std::to_string(header.version) +
            " (Supported version: 2)");
    }

    // 3. Load Code Segment into bytecode_raw
    bytecode_raw.resize(header.codeSize);
    if (header.codeSize > 0) {
        if (!in.read(reinterpret_cast<char *>(bytecode_raw.data()),
                     header.codeSize)) {
            throw std::runtime_error("Failed to read code segment (expected " +
                                     std::to_string(header.codeSize) +
                                     " bytes)");
        }
    }

    // 4. Load Data Segment into RAM at dataSegmentBase
    if (header.dataSize > 0) {
        if (header.dataSize > ram.size()) {
            throw std::runtime_error("RAM size (" + std::to_string(ram.size()) +
                                     ") too small for data segment (size " +
                                     std::to_string(header.dataSize) + ")");
        }
        char *data = (char *)malloc(header.dataSize * sizeof(char));
        char *tmp_data = data;
        in.read(data, header.dataSize);
        while (data < tmp_data + header.dataSize) {
            int16_t addr = *(int16_t *)&data[0];
            int16_t size = *(int16_t *)&data[2];
            data += 4;
            for (int i = 0; i < size; i++) {
                ram[addr + i] = data[i];
            }
            data += size;
        }
        free((char *)tmp_data);
    }

    if (header.dbgSize > 0) {
        char *dbg = (char *)malloc(header.dbgSize);
        char *dbg_og = dbg;

        in.read((char *)dbg, header.dbgSize);

        while (dbg < dbg_og + header.dbgSize) {
            std::string str = dbg;
            dbg += (strlen(dbg) + 1);
            int addr = *(int *)dbg;
            dbg += sizeof(int);
            lbls[addr] = str;
        }
        free(dbg_og);
    }

    // Check if we read exactly the expected amount
    in.peek(); // Try to read one more byte
    if (!in.eof()) {
        std::cerr
            << "Warning: Extra data found in bytecode file after code and "
               "data segments."
            << std::endl;
    }

    // 5. Set Instruction Pointer to Entry Point
    if (header.entryPoint >= header.codeSize &&
        header.codeSize > 0) { // Allow entryPoint 0 for empty code
        throw std::runtime_error("Entry point (" +
                                 std::to_string(header.entryPoint) +
                                 ") is outside the code segment (size " +
                                 std::to_string(header.codeSize) + ")");
    }
    ip = header.entryPoint;

    if (debugMode) {
        std::cout << "[Debug][Interpreter] Loading bytecode from: "
                  << bytecodeFile << "\n";
        std::cout << "[Debug][Interpreter]   Header - Magic: 0x" << std::hex
                  << header.magic << ", Version: " << std::dec << header.version
                  << ", CodeSize: " << header.codeSize
                  << ", DataSize: " << header.dataSize << ", EntryPoint: 0x"
                  << std::hex << header.entryPoint << std::dec << "\n";
        std::cout << "[Debug][Interpreter]   Data Segment loaded" << "\n";
        std::cout << "[Debug][Interpreter]   IP set to entry point: 0x"
                  << std::hex << ip << std::dec << "\n";
    }
}

void Interpreter::setArguments(const std::vector<std::string> &args) {
    cmdArgs = args;
    if (debugMode) {
        std::cout << "[Debug][Interpreter] Arguments set/updated. Count: "
                  << cmdArgs.size() << "\n";
    }
}

void Interpreter::setDebugMode(bool enabled) {
    debugMode = enabled;
    if (debugMode) {
        std::cout << "[Debug][Interpreter] Debug mode explicitly set to: "
                  << (enabled ? "ON" : "OFF") << "\n";
    }
}

std::string getAddr(int ip, std::unordered_map<int, std::string> dbgData) {
    std::vector<std::pair<int, std::string>> dbgPair;
    for (auto it = dbgData.begin(); it != dbgData.end(); ++it) {
        dbgPair.push_back(std::make_pair(it->first, it->second));
    }
    std::pair<int, std::string> *closest = nullptr;
    int distance = 100000000;
    for (int idx = 0; idx < dbgPair.size(); idx++) {
        int d = ip - dbgPair[idx].first;
        if (d < distance && d >= 0) {
            closest = &dbgPair[idx];
            distance = d;
        }
    }
    std::string ret = closest->second;
    ret += "+" + std::to_string(ip - closest->first) + "";
    return ret;
}

std::string PS1 = (char*)"> ";
void Interpreter::debugger_init() {
    char* value = std::getenv("MasmDebuggerPS1");
    if (value != NULL) {PS1 = value;}
    std::cout << "\nWelcome to the MASM debugger. Run help for a list of all commands\n";
}

int steps = 0;
bool continue_ran = false;
std::string prev_cmd;
std::vector<int> breakpoints;

std::string print_ip(int ip) {
    std::stringstream ss;
    ss << "0x" << std::hex << ip;
    if (lbls.size() > 0)
        ss << " (" << getAddr(ip, lbls) << ")";
    return ss.str();
}

std::stringstream dbg_output;

void Interpreter::debugger(bool end) {
    if (std::find(breakpoints.begin(), breakpoints.end(), ip) != breakpoints.end()) {
        std::cout << "Breakpoint hit at " << print_ip(ip) << std::endl;
        continue_ran = false;
        steps = 0;
    } else if (!end) {
        if (steps > 0) {
            steps--;
            return;
        }
        if (continue_ran) {
            return;
        }
    } else {
        std::cout << "\nProgram finished" << std::endl;
    }
    while (true) {
        std::cout << PS1 << std::flush;
        std::string input;
        std::getline(std::cin, input);

        std::vector<std::string> tokens;

        if (input.length() == 0)
            input = prev_cmd;
        else
            prev_cmd = input;
        std::stringstream ss(input);
        std::string token;
        while (std::getline(ss, token, ' ')) {
            tokens.push_back(token);
        }

        std::string cmd = tokens[0];
        if (cmd == "step" | cmd == "s") {
            if (tokens.size() > 1)
                steps = std::stoi(tokens[1])-1;
            return;
        } else if (cmd == "breakpoint" | cmd == "b") {
            std::string lbl;
            if (tokens.size() > 1)
                lbl = tokens[1];
            else
                std::cout << "Missing addr" << std::endl;
            if (lbl[0] == '#' && lbls.size() == 0)
                std::cout << "Cannot use a label as a address without debug labels in file (run compiler with -g to include debug info)" << std::endl;
            int addr;
            if (lbl.size() > 2 && lbl[2] == 'x') {
                addr = std::stoi(lbl, nullptr, 16);
            } else if (lbl[0] == '#') {
                for (auto l : lbls) {
                    if (l.second == lbl) {
                        addr = l.first;
                        break;
                    }
                }
            } else {
                addr = std::stoi(lbl);
            }
            if (std::find(breakpoints.begin(), breakpoints.end(), addr) != breakpoints.end()) {
                breakpoints.erase(std::remove(breakpoints.begin(), breakpoints.end(), addr), breakpoints.end());
                std::cout << "Removed breakpoint at " << print_ip(addr) << std::endl;
            } else {
                breakpoints.push_back(addr);
                std::cout << "Put breakpoint at " << print_ip(addr) << std::endl;
            }

        } else if (cmd == "continue" | cmd == "c") {
            continue_ran = true;
            return;
        } else if (cmd == "exit") {
            std::cout << "Goodbye!" << std::endl;
            exit(0);
        } else if (cmd == "status") {
            std::cout << "Debug Labels: ";
            if (lbls.size() > 0) {
                std::cout << "Y" << std::endl;
            } else {
                std::cout << "N" << std::endl;
            }
        } else if (cmd == "stdout") {
            std::cout << dbg_output.str();
        } else if (cmd == "addr") {
            std::cout << "Current IP: " << print_ip(ip) << std::endl;
        } else if (cmd == "help") {
            std::cout << "Debugger commands:" << std::endl;
            std::cout << "    help - Show this message" << std::endl;
            std::cout << "    step <amount> - step one instruction or <amount> instructions" << std::endl;
            std::cout << "    s <amount> - aliases of step <amount>" << std::endl;
            std::cout << "    breakpoint (addr) - pause execution at addr" << std::endl;
            std::cout << "    b (addr) - aliases of breakpoint (addr)" << std::endl;
            std::cout << "    continue - run the program until the program exits" << std::endl;
            std::cout << "    c - aliases of continue" << std::endl;
            std::cout << "    stdout - all text outputted so far from out and its variations" << std::endl;
            std::cout << "    status - status of the program" << std::endl;
            std::cout << "    addr - current address" << std::endl;
            std::cout << "    exit - quit the program" << std::endl;
        } else {
            std::cout << "Unknown command: " << cmd << std::endl;
        }
    }
}

void Interpreter::execute() {
    // Reset flags before execution? Or assume they persist? Assume reset for
    // now.
    zeroFlag = false;
    signFlag = false;
    // IP should be set by load() or jump instructions

    // Ensure SP and BP reflect register values at start
    sp = registers[7];
    bp = registers[6];

    bool exit = false;
    if (debugMode) debugger_init();
    while (ip < bytecode_raw.size() && !exit) {
        if (debugMode) debugger();
        int currentIp = ip;
        if (debugMode)
            std::cout << "[Debug][Interpreter] IP: " << print_ip(ip);

        Opcode opcode = static_cast<Opcode>(bytecode_raw[ip++]);

        if (debugMode) {
            std::cout << ": Opcode 0x" << std::hex << std::setw(2)
                      << std::setfill('0') << static_cast<int>(opcode)
                      << std::dec;
            // You might want a helper function to convert Opcode enum to string
            // here
            std::cout << " (";
            switch (opcode) { // Basic opcode names for debug
            case MOV:
                std::cout << "MOV";
                break;
            case MOVB:
                std::cout << "MOVB";
                break;
            case ADD:
                std::cout << "ADD";
                break;
            case SUB:
                std::cout << "SUB";
                break;
            case MUL:
                std::cout << "MUL";
                break;
            case DIV:
                std::cout << "DIV";
                break;
            case INC:
                std::cout << "INC";
                break;
            case JMP:
                std::cout << "JMP";
                break;
            case CMP:
                std::cout << "CMP";
                break;
            case JE:
                std::cout << "JE";
                break;
            case JL:
                std::cout << "JL";
                break;
            case CALL:
                std::cout << "CALL";
                break;
            case RET:
                std::cout << "RET";
                break;
            case PUSH:
                std::cout << "PUSH";
                break;
            case POP:
                std::cout << "POP";
                break;
            case OUT:
                std::cout << "OUT";
                break;
            case COUT:
                std::cout << "COUT";
                break;
            case OUTSTR:
                std::cout << "OUTSTR";
                break;
            case OUTCHAR:
                std::cout << "OUTCHAR";
                break;
            case HLT:
                std::cout << "HLT";
                break;
            case ARGC:
                std::cout << "ARGC";
                break;
            case GETARG:
                std::cout << "GETARG";
                break;
            case DB:
                std::cout << "DB";
                break;
            case LBL:
                std::cout << "LBL";
                break;
            case AND:
                std::cout << "AND";
                break;
            case OR:
                std::cout << "OR";
                break;
            case XOR:
                std::cout << "XOR";
                break;
            case NOT:
                std::cout << "NOT";
                break;
            case SHL:
                std::cout << "SHL";
                break;
            case SHR:
                std::cout << "SHR";
                break;
            case MOVADDR:
                std::cout << "MOVADDR";
                break;
            case MOVTO:
                std::cout << "MOVTO";
                break;
            case JNE:
                std::cout << "JNE";
                break;
            case JG:
                std::cout << "JG";
                break;
            case JLE:
                std::cout << "JLE";
                break;
            case JGE:
                std::cout << "JGE";
                break;
            case ENTER:
                std::cout << "ENTER";
                break;
            case LEAVE:
                std::cout << "LEAVE";
                break;
            case COPY:
                std::cout << "COPY";
                break;
            case FILL:
                std::cout << "FILL";
                break;
            case CMP_MEM:
                std::cout << "CMP_MEM";
                break;
            case MNI:
                std::cout << "MNI";
                break;
            case IN:
                std::cout << "IN";
                break;
            case MALLOC:
                std::cout << "MALLOC";
                break;
            case FREE:
                std::cout << "FREE";
                break;
            default:
                std::cout << "???";
                break;
            }
            std::cout << ")\n";
        }

        // Store pre-execution register state for comparison if needed
        std::vector<int> regsBefore;
        if (debugMode)
            regsBefore = registers;

        try {
            switch (opcode) {
            // Basic Arithmetic
            case MOV: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_src = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Src ): "
                              << formatOperandDebug(op_src) << "\n";
                writeToOperand(op_dest, getValue(op_src, 4), 4);
                break;
            }
            case MOVB: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_src = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Src ): "
                              << formatOperandDebug(op_src) << "\n";
                writeToOperand(op_dest, getValue(op_src, 1), 1);
                break;
            }
            case ADD: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_src = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Src ): "
                              << formatOperandDebug(op_src) << "\n";
                writeToOperand(op_dest, getValue(op_src, 4) + getValue(op_dest, 4), 4);
                break;
            }
            case SUB: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_src = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Src ): "
                              << formatOperandDebug(op_src) << "\n";
                writeToOperand(op_dest, getValue(op_dest, 4) - getValue(op_src, 4), 4);
                break;
            }
            case MUL: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_src = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Src ): "
                              << formatOperandDebug(op_src) << "\n";
                writeToOperand(op_dest, getValue(op_src, 4) * getValue(op_dest, 4), 4);
                break;
            }
            case DIV: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_src = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Src ): "
                              << formatOperandDebug(op_src) << "\n";
                int src_val = getValue(op_src, 4);
                if (src_val == 0)
                    throw std::runtime_error("Division by zero");
                writeToOperand(op_dest, getValue(op_dest, 4) / src_val, 4);
                break;
            }
            case INC: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                writeToOperand(op_dest, getValue(op_dest, 4)+1, 4);
                break;
            }

            // Flow Control
            case JMP: {
                BytecodeOperand op_target = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Target): "
                              << formatOperandDebug(op_target) << "\n";
                // Target should be immediate label address from compiler
                if (op_target.type != OperandType::LABEL_ADDRESS &&
                    op_target.type != OperandType::IMMEDIATE) {
                    throw std::runtime_error(
                        "JMP requires immediate/label address operand");
                }
                ip = op_target.value; // Jump to absolute address
                if (debugMode)
                    std::cout << "[Debug][Interpreter]     Jumping to 0x"
                              << std::hex << ip << std::dec << "\n";
                break;
            }
            case CMP: {
                BytecodeOperand op1 = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1: "
                              << formatOperandDebug(op1) << "\n";
                BytecodeOperand op2 = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2: "
                              << formatOperandDebug(op2) << "\n";
                int val1 = getValue(op1, 4);
                int val2 = getValue(op2, 4);
                zeroFlag = (val1 == val2);
                signFlag = (val1 < val2);
                if (debugMode)
                    std::cout << "[Debug][Interpreter]     Compare(" << val1
                              << ", " << val2 << ") -> ZF=" << zeroFlag
                              << ", SF=" << signFlag << "\n";
                break;
            }
            // Conditional Jumps (JE, JNE, JL, JG, JLE, JGE)
            case JE:
            case JNE:
            case JL:
            case JG:
            case JLE:
            case JGE: {
                BytecodeOperand op_target = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Target): "
                              << formatOperandDebug(op_target) << "\n";
                if (op_target.type != OperandType::LABEL_ADDRESS &&
                    op_target.type != OperandType::IMMEDIATE) {
                    throw std::runtime_error("Conditional jump requires "
                                             "immediate/label address operand");
                }
                bool shouldJump = false;
                switch (opcode) {
                case JE:
                    shouldJump = zeroFlag;
                    break;
                case JNE:
                    shouldJump = !zeroFlag;
                    break;
                case JL:
                    shouldJump = signFlag;
                    break;
                case JG:
                    shouldJump = !zeroFlag && !signFlag;
                    break;
                case JLE:
                    shouldJump = zeroFlag || signFlag;
                    break;
                case JGE:
                    shouldJump = zeroFlag || !signFlag;
                    break;
                default:
                    break; // Should not happen
                }
                if (shouldJump) {
                    ip = op_target.value;
                    if (debugMode)
                        std::cout << "[Debug][Interpreter]     Condition met. "
                                     "Jumping to 0x"
                                  << std::hex << ip << std::dec << "\n";
                } else {
                    if (debugMode)
                        std::cout << "[Debug][Interpreter]     Condition not "
                                     "met. Continuing.\n";
                }
                break;
            }

            case CALL: {
                BytecodeOperand op_target = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Target): "
                              << formatOperandDebug(op_target) << "\n";
                if (op_target.type != OperandType::LABEL_ADDRESS &&
                    op_target.type != OperandType::IMMEDIATE) {
                    throw std::runtime_error(
                        "CALL requires immediate/label address operand");
                }
                pushStack(ip); // Push return address (address AFTER call
                               // instruction + operands)
                if (debugMode)
                    std::cout
                        << "[Debug][Interpreter]     Pushing return address 0x"
                        << std::hex << ip << std::dec << ". Calling 0x"
                        << std::hex << op_target.value << std::dec << "\n";
                ip = op_target.value; // Jump to function
                break;
            }
            case RET: {
                if (registers[7] >= ram.size())
                    throw std::runtime_error("Stack underflow on RET");
                int retAddr = popStack();
                if (debugMode)
                    std::cout
                        << "[Debug][Interpreter]     Popped return address 0x"
                        << std::hex << retAddr << std::dec << ". Returning.\n";
                ip = retAddr; // Pop return address and jump
                break;
            }

            // Stack Operations
            case PUSH: {
                BytecodeOperand op_src = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Src): "
                              << formatOperandDebug(op_src) << "\n";
                int val = getValue(op_src, 4);
                pushStack(val); // Push value (reg or imm)
                if (debugMode)
                    std::cout << "[Debug][Interpreter]     Pushed value " << val
                              << ". New SP: 0x" << std::hex << sp << std::dec
                              << "\n";
                break;
            }
            case POP: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                int dest_reg = getRegisterIndex(op_dest);
                int val = popStack();
                registers[dest_reg] = val;
                if (debugMode)
                    std::cout << "[Debug][Interpreter]     Popped value " << val
                              << " into R" << dest_reg << ". New SP: 0x"
                              << std::hex << sp << std::dec << "\n";
                break;
            }

            // I/O Operations
            case OUT: {
                BytecodeOperand op_port = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Port): "
                              << formatOperandDebug(op_port) << "\n";
                BytecodeOperand op_val = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Val ): "
                              << formatOperandDebug(op_val) << "\n";
                int port = getValue(op_port, 4);
                std::ostream &out_stream = (port == 2) ? std::cerr : std::cout;
                if (port != 1 && port != 2)
                    throw std::runtime_error("Invalid port for OUT: " +
                                             std::to_string(port));
                switch (op_val.type) {
                case OperandType::DATA_ADDRESS: {
                    // Get the absolute RAM address (Base + Offset)
                    int address = op_val.value;
                    if (address < 0 || address >= ram.size()) {
                        throw std::runtime_error(
                            "OUT: Data address out of RAM bounds: " +
                            std::to_string(op_val.value));
                    }
                    out_stream << readRamString(address); // Read and print the string from RAM
                    if (debugMode) dbg_output << readRamString(address); // Read and print the string from RAM
                    break;
                }
                case OperandType::REGISTER_AS_ADDRESS: {
                    int reg_index = op_val.value;
                    if (reg_index < 0 || reg_index >= registers.size()) {
                        throw std::runtime_error("OUT: Invalid register index "
                                                 "for REGISTER_AS_ADDRESS: " +
                                                 std::to_string(reg_index));
                    }
                    int address =
                        registers[reg_index]; // Get address from register
                    if (address < 0 || address >= ram.size()) {
                        throw std::runtime_error(
                            "OUT: Address in register R" +
                            std::to_string(reg_index) + " (" +
                            std::to_string(address) + ") is out of RAM bounds");
                    }
                    out_stream << readRamString(address); // Read and print the string from RAM
                    if (debugMode) dbg_output << readRamString(address); // Read and print the string from RAM
                    break;
                }
                case OperandType::REGISTER: {
                    int reg_val = registers[getRegisterIndex(op_val)];
                    out_stream << reg_val; // Print the integer value in the register
                    if (debugMode) dbg_output << reg_val;
                    break;
                }
                case OperandType::IMMEDIATE: {
                    out_stream << op_val.value; // Print the immediate integer value
                    if (debugMode) dbg_output << op_val.value; 
                    break;
                }
                default:
                    throw std::runtime_error(
                        "Unsupported operand type for OUT value: " +
                        std::to_string(static_cast<int>(op_val.type)));
                }
                break;
            }
            case COUT: {
                BytecodeOperand op_port = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Port): "
                              << formatOperandDebug(op_port) << "\n";
                BytecodeOperand op_val = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Val ): "
                              << formatOperandDebug(op_val) << "\n";
                int port = getValue(op_port, 4);
                std::ostream &out_stream = (port == 2) ? std::cerr : std::cout;
                if (port != 1 && port != 2)
                    throw std::runtime_error("Invalid port for COUT: " +
                                             std::to_string(port));
                out_stream << static_cast<char>(getValue(op_val, 4)); // Output char value
                if (debugMode) dbg_output << static_cast<char>(getValue(op_val, 4));
                break;
            }
            case OUTSTR: { // Prints string from RAM address IN REGISTER
                BytecodeOperand op_port = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Port): "
                              << formatOperandDebug(op_port) << "\n";
                BytecodeOperand op_addr = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Addr): "
                              << formatOperandDebug(op_addr) << "\n";
                BytecodeOperand op_len = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op3(Len ): "
                              << formatOperandDebug(op_len) << "\n";
                int port = getValue(op_port, 4);
                std::ostream &out_stream = (port == 2) ? std::cerr : std::cout;
                if (port != 1 && port != 2)
                    throw std::runtime_error("Invalid port for OUTSTR: " +
                                             std::to_string(port));

                int addr = getValue(op_addr, 4);
                int len = getValue(op_len, 4);
                for (int i = 0; i < len; ++i) {
                    out_stream << readRamChar(addr + i); // Read char by char
                    if (debugMode) dbg_output << readRamChar(addr + i);
                }
                // No automatic newline for OUTSTR
                break;
            }
            case OUTCHAR: { // Prints single char from RAM address IN REGISTER
                BytecodeOperand op_port = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Port): "
                              << formatOperandDebug(op_port) << "\n";
                BytecodeOperand op_addr = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Addr): "
                              << formatOperandDebug(op_addr) << "\n";
                int port = getValue(op_port, 4);
                std::ostream &out_stream = (port == 2) ? std::cerr : std::cout;
                if (port != 1 && port != 2)
                    throw std::runtime_error("Invalid port for OUTCHAR: " +
                                             std::to_string(port));

                int addr = getValue(op_addr, 4);
                out_stream << readRamChar(addr); // Read single char
                if (debugMode) dbg_output << readRamChar(addr);
                // No automatic newline for OUTCHAR
                break;
            }
            case IN: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";

                std::string input;
                std::getline(std::cin, input);

                // Write input and null terminator to memory
                std::copy(input.begin(), input.end(), ram.begin() + getRamAddr(op_dest));
                ram[getRamAddr(op_dest) + input.size()] = '\0';
                break;
            }

            // Program Control
            case HLT: {
                if (debugMode)
                    std::cout << "[Debug][Interpreter] HLT encountered.\n";
                exit = true; // exit loop
                break;
            } // Halt execution
            case ARGC: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                writeToOperand(op_dest, cmdArgs.size(), 4); // Use cmdArgs member
                break;
            }
            case GETARG: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_index = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Index): "
                              << formatOperandDebug(op_index) << "\n";
                int index = getValue(op_index, 4);
                if (index < 0 || index >= cmdArgs.size()) {
                    throw std::runtime_error("GETARG index out of bounds: " +
                                             std::to_string(index));
                }
                // How to store string arg in int register? Store address.
                // Need to copy string to RAM and store address.
                // Simplified: Not implemented fully. Requires memory
                // management.
                // Update: WE HAVE MEMORY MANAGMENT NOW YIPPIE. time to implement this --carson
                // code is expected to free memory
                int str_addr = mmalloc(cmdArgs[index].length());

                writeToOperand(op_dest, str_addr, 4);

                std::copy(cmdArgs[index].begin(), cmdArgs[index].end(), ram.begin() + str_addr);
                ram[str_addr + cmdArgs[index].size()] = '\0';
                break;
            }

            // Bitwise Operations
            case AND: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_src = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Src ): "
                              << formatOperandDebug(op_src) << "\n";
                writeToOperand(op_dest, getValue(op_dest, 4)&getValue(op_src, 4), 4);
                break;
            }
            case OR: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_src = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Src ): "
                              << formatOperandDebug(op_src) << "\n";
                writeToOperand(op_dest, getValue(op_dest, 4)|getValue(op_src, 4), 4);
                break;
            }
            case XOR: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_src = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Src ): "
                              << formatOperandDebug(op_src) << "\n";
                writeToOperand(op_dest, getValue(op_dest, 4)^getValue(op_src, 4), 4);
                break;
            }
            case NOT: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                writeToOperand(op_dest, ~getValue(op_dest, 4), 4);
                break;
            }
            case SHL: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_count = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Count): "
                              << formatOperandDebug(op_count) << "\n";
                writeToOperand(op_dest, getValue(op_dest, 4)<<getValue(op_count, 2), 4);
                break;
            }
            case SHR: {
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_count = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Count): "
                              << formatOperandDebug(op_count) << "\n";
                writeToOperand(op_dest, getValue(op_dest, 4)>>getValue(op_count, 4), 4);
                break;
            } // Arithmetic right shift

            // Memory Addressing
            case MOVADDR: { // MOVADDR dest_reg src_addr_reg offset_reg
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_src_addr = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(SrcAddr): "
                              << formatOperandDebug(op_src_addr) << "\n";
                BytecodeOperand op_offset = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op3(Offset): "
                              << formatOperandDebug(op_offset) << "\n";
                int address = getValue(op_src_addr, 4) + getValue(op_offset, 4);
                writeToOperand(op_dest, readRamInt(address), 4);
                break;
            }
            case MOVTO: { // MOVTO dest_addr_reg offset_reg src_reg
                BytecodeOperand op_dest_addr = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(DestAddr): "
                              << formatOperandDebug(op_dest_addr) << "\n";
                BytecodeOperand op_offset = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Offset): "
                              << formatOperandDebug(op_offset) << "\n";
                BytecodeOperand op_src = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op3(Src): "
                              << formatOperandDebug(op_src) << "\n";
                int address = getValue(op_dest_addr, 4) + getValue(op_offset, 4);
                writeRamInt(address, getValue(op_src, 4));
                break;
            }

            // Stack Frame Management
            case ENTER: { // ENTER framesize (immediate)
                BytecodeOperand op_frameSize = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(FrameSize): "
                              << formatOperandDebug(op_frameSize) << "\n";
                int frameSize = getValue(op_frameSize, 4);
                pushStack(registers[6]);     // Push RBP
                registers[6] = registers[7]; // MOV RBP, RSP
                bp = registers[6];
                registers[7] -=
                    frameSize; // SUB RSP, framesize // (PUSH 0) * framesize
                sp = registers[7];
                break;
            }
            case LEAVE: {                    // LEAVE
                registers[7] = registers[6]; // MOV RSP, RBP
                sp = registers[7];
                registers[6] = popStack(); // POP RBP
                bp = registers[6];
                break;
            }

            // String/Memory Operations
            case COPY: { // COPY dest_addr_reg src_addr_reg len_reg
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_src = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Src ): "
                              << formatOperandDebug(op_src) << "\n";
                BytecodeOperand op_len = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op3(Len ): "
                              << formatOperandDebug(op_len) << "\n";
                int dest_addr = getValue(op_dest, 4);
                int src_addr = getValue(op_src, 4);
                int len = getValue(op_len, 4);
                if (len < 0)
                    throw std::runtime_error("COPY length cannot be negative");
                if (dest_addr < 0 || dest_addr + len > ram.size() ||
                    src_addr < 0 || src_addr + len > ram.size()) {
                    throw std::runtime_error(
                        "COPY memory access out of bounds");
                }
                // Use memcpy directly (it's in the global namespace via
                // <cstring>)
                memcpy(&ram[dest_addr], &ram[src_addr], len);
                break;
            }
            case FILL: { // FILL dest_addr_reg value_reg len_reg
                BytecodeOperand op_dest = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Dest): "
                              << formatOperandDebug(op_dest) << "\n";
                BytecodeOperand op_val = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Val ): "
                              << formatOperandDebug(op_val) << "\n";
                BytecodeOperand op_len = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op3(Len ): "
                              << formatOperandDebug(op_len) << "\n";
                int dest_addr = getValue(op_dest, 4);
                int value =
                    getValue(op_val, 4) & 0xFF; // Use lower byte of value register
                int len = getValue(op_len, 4);
                if (len < 0)
                    throw std::runtime_error("FILL length cannot be negative");
                if (dest_addr < 0 || dest_addr + len > ram.size()) {
                    throw std::runtime_error(
                        "FILL memory access out of bounds");
                }
                // Use memset directly
                memset(&ram[dest_addr], value, len);
                break;
            }
            case CMP_MEM: { // CMP_MEM addr1_reg addr2_reg len_reg
                BytecodeOperand op_addr1 = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(Addr1): "
                              << formatOperandDebug(op_addr1) << "\n";
                BytecodeOperand op_addr2 = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(Addr2): "
                              << formatOperandDebug(op_addr2) << "\n";
                BytecodeOperand op_len = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op3(Len ): "
                              << formatOperandDebug(op_len) << "\n";
                int addr1 = getValue(op_addr1, 4);
                int addr2 = getValue(op_addr2, 4);
                int len = getValue(op_len, 4);
                if (len < 0)
                    throw std::runtime_error(
                        "CMP_MEM length cannot be negative");
                if (addr1 < 0 || addr1 + len > ram.size() || addr2 < 0 ||
                    addr2 + len > ram.size()) {
                    throw std::runtime_error(
                        "CMP_MEM memory access out of bounds");
                }
                // Use memcmp directly
                int result = memcmp(&ram[addr1], &ram[addr2], len);
                zeroFlag = (result == 0);
                signFlag = (result < 0);
                break;
            }
            case MALLOC: { // MALLOC ptr_reg size
                BytecodeOperand op_ptr = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(ptr): "
                              << formatOperandDebug(op_ptr) << "\n";
                BytecodeOperand op_size = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(size): "
                              << formatOperandDebug(op_size) << "\n";

                int size = getValue(op_size, 4);

                int result = mmalloc(size);

                writeToOperand(op_ptr, result, 4);
                
                zeroFlag = (result == 0);
                signFlag = (result < 0);
                break;
            }
            case FREE: { // FREE result ptr
                BytecodeOperand op_result = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op1(result): "
                              << formatOperandDebug(op_result) << "\n";
                BytecodeOperand op_ptr = nextRawOperand();
                if (debugMode)
                    std::cout << "[Debug][Interpreter]   Op2(ptr): "
                              << formatOperandDebug(op_ptr) << "\n";

                int ptr = getValue(op_ptr, 4);

                int result = mfree(ptr);

                writeToOperand(op_result, result, 4);
                
                zeroFlag = (result == 0);
                signFlag = (result < 0);
                break;
            }

            case MNI: {
                std::string functionName =
                    readBytecodeString(); // Read Module.Function\0
                if (debugMode)
                    std::cout
                        << "[Debug][Interpreter]   MNI Func: " << functionName
                        << "\n";
                std::vector<BytecodeOperand> mniArgs;
                while (true) {
                    BytecodeOperand arg = nextRawOperand();
                    if (arg.type == OperandType::NONE) {
                        if (debugMode)
                            std::cout
                                << "[Debug][Interpreter]     End MNI Args\n";
                        break; // End of arguments marker
                    }
                    if (debugMode)
                        std::cout << "[Debug][Interpreter]     MNI Arg : "
                                  << formatOperandDebug(arg) << "\n";
                    mniArgs.push_back(arg);
                }
                if (mniRegistry.count(functionName)) {
                    mniRegistry[functionName](
                        *this,
                        mniArgs); // Call the registered function
                } else {
                    throw std::runtime_error(
                        "Unregistered MNI function called: " + functionName);
                }
                break;
            }

            default:
                throw std::runtime_error("Unimplemented or unknown opcode "
                                         "encountered during execution: 0x" +
                                         std::to_string(opcode));
            }

            // Optional: Print changed registers
            if (debugMode) {
                for (size_t i = 0; i < registers.size(); ++i) {
                    if (registers[i] != regsBefore[i]) {
                        std::cout << "[Debug][Interpreter]     Reg Change: R"
                                  << i << " = " << registers[i] << " (was "
                                  << regsBefore[i] << ")\n";
                    }
                }
                // Print flag changes if CMP or relevant instruction executed
                // (Requires tracking if flags were potentially modified by the
                // opcode)
            }

        } catch (const std::exception &e) {
            // Print MNI stack trace if any

            if (!mniCallStack.empty()) {
                std::cerr << "MNI Call Stack (most recent call last):\n";
                for (auto it = mniCallStack.rbegin(); it != mniCallStack.rend();
                     ++it) {
                    std::cerr << "  at " << *it << std::endl;
                }
            }
            std::cerr << "\nRuntime Error at bytecode offset 0x" << std::hex
                      << currentIp << std::dec << " (Opcode: 0x" << std::hex
                      << static_cast<int>(opcode) << std::dec
                      << "): " << e.what() << std::endl;
            // Stack trace if -t or --trace
            if (stackTrace) {
                std::cerr << "\nStack Trace (most recent call first):\n";
                struct stack_frame frame;
                frame.rbp = registers[6]; // ebp
                frame.ip = ip;

                while (frame.rbp != 0) {
                    std::cerr << getAddr(frame.ip, lbls) << std::endl;
                    frame.ip = readRamInt(frame.rbp + 4);
                    frame.rbp = readRamInt(frame.rbp);
                }
                std::cerr << "\n";
            }
            static const char *regNames[24] = {

                "RAX", "RBX", "RCX", "RDX", "RSI",
                "RDI", "RBP", "RSP",

                "R0",  "R1",  "R2",  "R3",  "R4",
                "R5",  "R6",  "R7",

                "R8",  "R9",  "R10", "R11", "R12",
                "R13", "R14", "R15"

            };

            // Dump registers with names and color
            std::cerr << "Register dump:\n";
            // Dump registers as an ASCII box (8 per row)
            const int regsPerRow = 8;
            const int totalRegs = registers.size();
            const int rows = (totalRegs + regsPerRow - 1) / regsPerRow;
            const int colWidth = 12; // Match hex value width
            std::cerr << "+"
                      << std::string(regsPerRow * (colWidth + 1) - 1, '-')
                      << "+\n";
            for (int row = 0; row < rows; ++row) {
                // Header row: register names (centered, colWidth chars)
                std::cerr << "|";
                for (int col = 0; col < regsPerRow; ++col) {
                    int idx = row * regsPerRow + col;
                    if (idx < totalRegs) {
                        std::string color;
                        if (idx == 0)
                            color = "\033[1;33m"; // RAX: yellow
                        else if (idx == 6 || idx == 7)
                            color = "\033[1;36m"; // RBP/RSP: cyan
                        else
                            color = "\033[1m";
                        std::string name = regNames[idx];
                        int pad = colWidth - name.length();
                        int left = pad / 2, right = pad - left;
                        std::cerr << color << std::string(left, ' ') << name
                                  << std::string(right, ' ') << "\033[0m"
                                  << "|";
                    } else {
                        std::cerr << std::string(colWidth, ' ') << "|";
                    }
                }
                std::cerr << "\n|";
                // Value row: decimal values (colWidth chars)
                for (int col = 0; col < regsPerRow; ++col) {
                    int idx = row * regsPerRow + col;
                    if (idx < totalRegs) {
                        std::cerr << std::setw(colWidth - 1) << registers[idx]
                                  << " |";
                    } else {
                        std::cerr << std::string(colWidth, ' ') << "|";
                    }
                }

                std::cerr << "\n|";
                // Value row: hex values (colWidth chars)
                for (int col = 0; col < regsPerRow; ++col) {
                    int idx = row * regsPerRow + col;
                    if (idx < totalRegs) {
                        std::stringstream hexss;
                        hexss << "0x" << std::hex << std::setw(8)
                              << std::setfill('0') << registers[idx]
                              << std::dec;
                        std::string hexval = hexss.str();
                        int pad = colWidth - hexval.length();
                        int left = pad / 2, right = pad - left;
                        std::cerr << std::string(left, ' ') << hexval
                                  << std::string(right, ' ') << "|";
                    } else {
                        std::cerr << std::string(colWidth, ' ') << "|";
                    }
                }

                std::cerr << "\n+"
                          << std::string(regsPerRow * (colWidth + 1) - 1, '-')
                          << "+\n";
            }

            std::cerr << "  ZF=" << zeroFlag << ", SF=" << signFlag << "\n";

            std::cerr << "\n";
            // std::cerr << "\033[1;33mRAX\033[0m, \033[1;36mRBP/RSP\033[0m:
            // special "
            //              "registers\n";

            check_unfreed_memory(true); // cleanup heap
            throw; // Re-throw after logging context
        }
    }
    check_unfreed_memory(); // cleanup memory and print unfreed memory
    if (debugMode) debugger(true); // Allow for some last minute commands
}

static std::vector<std::string> mniCallStackInternal;

void Interpreter::callMNI(const std::string &name,
                          const std::vector<BytecodeOperand> &args) {

    // Use a static thread_local call stack for MNI stack tracing

    static thread_local std::vector<std::string> mniCallStackInternal;
    mniCallStackInternal.push_back(name);
    if (mniRegistry.count(name)) {
        try {
            mniRegistry[name](*this, args);
        } catch (...) {
            // Print stack trace on error
            std::cerr << "MNI Call Stack (most recent call last):\n";
            for (auto it = mniCallStackInternal.rbegin();
                 it != mniCallStackInternal.rend(); ++it) {
                std::cerr << "  at " << *it << std::endl;
            }
            mniCallStackInternal.pop_back();
            throw;
        }
    } else {
        mniCallStackInternal.pop_back();
        throw std::runtime_error("Unregistered MNI function called: " + name);
    }
    mniCallStackInternal.pop_back();
}

// --- Standalone Interpreter Main Function Definition ---

int microasm_interpreter_main(int argc, char *argv[]) {
    // --- Argument Parsing for Standalone Interpreter ---
    std::string bytecodeFile;
    bool enableDebug = false;
    bool stackTrace = false;
    std::vector<std::string> programArgs; // Args for the interpreted program

    // argv[0] here is the *first argument* after "-i", not the program name
    for (int i = 0; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--debug") {
            enableDebug = true;
        } else if (arg == "-t" || arg == "--trace") {
            stackTrace = true;
        } else if (bytecodeFile.empty()) {
            bytecodeFile = arg;
        } else {
            // Remaining args are for the program being interpreted
            programArgs.push_back(arg);
        }
    }

    if (bytecodeFile.empty()) {
        std::cerr << "Interpreter Usage: <bytecode.bin> [args...] [-d|--debug]"
                  << std::endl;
        return 1;
    }

    // --- End Argument Parsing ---

    try {
        Interpreter interpreter(65536, programArgs, enableDebug,
                                stackTrace); // Pass debug flag
        interpreter.load(bytecodeFile);
        interpreter.execute();

        std::cout << "Execution finished successfully!"
                  << std::endl; // HLT used to provide its own message
    } catch (const std::exception &e) {
        // Error already logged in execute() or load()
        std::cerr << "Execution failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
