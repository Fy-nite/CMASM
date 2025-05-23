#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <unordered_map>
#include <set>
#include <sstream>
#include <cstdint>
#include <cctype>
#include <cstring>
#include "common_defs.h"
//#include "microasm_compiler.h"

// --- Shared enums/types (should match interpreter/compiler) ---
enum OperandType {
    NONE = 0x00,
    REGISTER = 0x01,
    IMMEDIATE = 0x02,
    LABEL_ADDRESS = 0x03,
    DATA_ADDRESS = 0x04,
    REGISTER_AS_ADDRESS = 0x05,
    MATH_OPERATOR = 0x06,
};

const std::unordered_map<Opcode, std::string> opcodeToString = {
    {MOV, "MOV"}, {ADD, "ADD"}, {SUB, "SUB"}, {MUL, "MUL"}, {DIV, "DIV"}, {INC, "INC"},
    {JMP, "JMP"}, {CMP, "CMP"}, {JE, "JE"}, {JL, "JL"}, {CALL, "CALL"}, {RET, "RET"},
    {PUSH, "PUSH"}, {POP, "POP"},
    {OUT, "OUT"}, {COUT, "COUT"}, {OUTSTR, "OUTSTR"}, {OUTCHAR, "OUTCHAR"},
    {HLT, "HLT"}, {ARGC, "ARGC"}, {GETARG, "GETARG"},
    {AND, "AND"}, {OR, "OR"}, {XOR, "XOR"}, {NOT, "NOT"}, {SHL, "SHL"}, {SHR, "SHR"},
    {MOVADDR, "MOVADDR"}, {MOVTO, "MOVTO"},
    {JNE, "JNE"}, {JG, "JG"}, {JLE, "JLE"}, {JGE, "JGE"},
    {ENTER, "ENTER"}, {LEAVE, "LEAVE"},
    {COPY, "COPY"}, {FILL, "FILL"}, {CMP_MEM, "CMP_MEM"},
    {MNI, "MNI"}, {IN, "IN"}, 
    {MALLOC, "MALLOC"}, {FREE, "FREE"}
};

const std::unordered_map<int, std::string> registerIndexToString = {
    {0, "RAX"}, {1, "RBX"}, {2, "RCX"}, {3, "RDX"},
    {4, "RSI"}, {5, "RDI"}, {6, "RBP"}, {7, "RSP"},
    {8, "R0"}, {9, "R1"}, {10, "R2"}, {11, "R3"},
    {12, "R4"}, {13, "R5"}, {14, "R6"}, {15, "R7"},
    {16, "R8"}, {17, "R9"}, {18, "R10"}, {19, "R11"},
    {20, "R12"}, {21, "R13"}, {22, "R14"}, {23, "R15"}
};

std::string formatOperand(OperandType type, long long value) {
    switch (type & 0xf) {
        case REGISTER:
            if (registerIndexToString.count(value)) return registerIndexToString.at(value);
            return "R?" + std::to_string(value);
        case REGISTER_AS_ADDRESS:
            if (registerIndexToString.count(value)) return "$" + registerIndexToString.at(value);
            return "$R?" + std::to_string(value);
        case IMMEDIATE:
            return std::to_string(value);
        case LABEL_ADDRESS:
            return "#" + std::to_string(value);
        case DATA_ADDRESS:
            return "$" + std::to_string(value);
        case NONE:
            return "[NONE]";
        case MATH_OPERATOR: {
            MathOperator data;
            data.operand = (MathOperatorOperators)(value >> 8 & 0xFF);
            data.reg = value & 0xFF;
            data.other = {Immediate, (int)(value >> 16)};
            if (type >> 4 == 1) data.other.type = Register;
            // std::cout << std::to_string(data.operand) << std::endl;
            // std::cout << std::to_string(type >> 4) << std::endl;
            // std::cout << std::to_string(data.other.val) << std::endl;
            std::string first = registerIndexToString.at(data.reg);
            std::string second;
            if (data.other.type == Register) second = registerIndexToString.at(data.other.val);
            if (data.other.type == Immediate) second = std::to_string(data.other.val);
            std::string operand;
            std::string tmp = first;
            switch (data.operand)
            {
                case op_ADD:
                    operand = "+";
                    break;
                case op_SUB:
                    operand = "-";
                    break;
                case op_MUL:
                    operand = "*";
                    break;
                case op_DIV:
                    operand = "/";
                    break;
                case op_BDIV:
                    first = second;
                    second = tmp;
                    operand = "/";
                    break; // backward div
                case op_LSR:
                    operand = ">>";
                    break;
                case op_LSL:
                    operand = "<<";
                    break;
                case op_AND:
                    operand = "&";
                    break;
                case op_OR:
                    operand = "|";
                    break;
                case op_XOR:
                    operand = "^";
                    break;
                case op_BSUB:
                    first = second;
                    second = tmp;
                    operand = "-";
                    break;
                case op_BLSR:
                    first = second;
                    second = tmp;
                    operand = ">>";
                    break;
                case op_BLSL:
                    first = second;
                    second = tmp;
                    operand = "<<";
                    break;
                case op_NONE:
                    operand = "ERR";
                    break;
                
                default:
                    break;
            }
            return "$[" + first + operand + second + "]";
        }
        default:
            return "\t[UNKNOWN]";
    }
}

std::string escapeString(const std::string& s) {
    std::ostringstream ss;
    for (char c : s) {
        if (c == '\n') ss << "\\n";
        else if (c == '\t') ss << "\\t";
        else if (c == '\\') ss << "\\\\";
        else if (c == '"') ss << "\\\"";
        else if (std::isprint(static_cast<unsigned char>(c))) ss << c;
        else ss << "\\x" << std::setw(2) << std::setfill('0') << std::hex << (int)(unsigned char)c;
    }
    return ss.str();
}

std::string readString(const std::vector<uint8_t>& vec, size_t& offset) {
    std::string str;
    while (offset < vec.size()) {
        char c = static_cast<char>(vec[offset++]);
        if (c == '\0') break;
        str += c;
    }
    return str;
}

void printHexBytes(const std::vector<uint8_t>& vec, size_t start, size_t end) {
    for (size_t i = start; i < end && i < vec.size(); ++i)
        std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)vec[i] << " ";
    std::cout << std::dec;
}

int getOperandCount(Opcode opcode) {
    switch (opcode) {
        case MOV: case ADD: case SUB: case MUL: case DIV: case CMP: case AND: case OR: case XOR: case SHL: case SHR:
        case GETARG: case COPY: case FILL: case CMP_MEM: case OUT: case COUT: case OUTCHAR:
            return 2;
        case OUTSTR: case MOVTO: case MOVADDR:
            return 3;
        case INC: case JMP: case JE: case JL: case CALL: case PUSH: case POP: case JNE: case JG: case JLE: case JGE: case ENTER: case ARGC: case IN:
            return 1;
        case RET: case LEAVE: case HLT:
            return 0;
        case MALLOC: case FREE:
            return 2;
        case MNI:
            return -1; // special
        default:
            return -2; // unknown
    }
}

int getOperandSize(char type) {
    if (type == '\0') {
        return 0;
    }
    int ret =  type >> 4;
    if (ret == 0) ret = 4;
    return ret;
}

// ANSI color codes
#define CLR_RESET   "\033[0m"
#define CLR_OPCODE  "\033[1;36m"
#define CLR_OFFSET  "\033[1;33m"
#define CLR_OPERAND "\033[1;32m"
#define CLR_HEX     "\033[1;35m"
#define CLR_COMMENT "\033[1;90m"
#define CLR_ERROR   "\033[1;31m"

char* repr(char* str) {
    int len = 0;
    char i = str[0];
    while (i!='\0') {
        len++;
        i = str[len];
    }

    char* ret = (char*)malloc(len*2+1);
    char* tmp = ret;
    for (int i=0;i<len;i++) {
        switch (str[i])
        {
            case '\n':
                tmp[0] = '\\';
                tmp[1] = 'n';
                tmp += 2;
                break;
            case '\\':
                tmp[0] = '\\';
                tmp[1] = '\\';
                tmp += 2;
                break;
            case '\t':
                tmp[0] = '\\';
                tmp[1] = 't';
                tmp += 2;
                break;
            case '"':
                tmp[0] = '\\';
                tmp[1] = '"';
                tmp += 2;
                break;
            
            default:
                tmp[0] = str[i];
                tmp++;
        }
    }
    return ret;
}

template< typename T >
std::string int_to_hex( T i )
{
    std::stringstream stream;
    stream << "0x" 
            << std::setfill ('0') << std::setw(sizeof(T)*2) 
            << std::hex << i;
    return stream.str();
}

bool contains(int line, std::vector<int> labels, std::unordered_map<int, int> addr_to_line, int* label_i) {
    for (int i=0;i<labels.size();i++) {
        int lbl = labels[i];
        for (auto& pair : addr_to_line) {
            if (pair.first == lbl && pair.second == line) {
                *label_i = i;
                return true;
            }
        }
    }
    return false;
}

// Always define as decoder_main when included in the main project
int decoder_main(int argc, char* argv[]) {
    if (argc < 1) {
        std::cerr << "Usage: masm -u <file.bin> [output.masm]" << std::endl;
        return 1;
    }
    char* file = argv[0];
    char* output = argv[1];
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        std::cerr << "Error: Cannot open input file: " << file << std::endl;
        return 1;
    }

    std::vector<std::string> instructions;
    std::unordered_map<int, int> addr_to_line;
    std::vector<int> labels;
    int entry_line;

    try {
        BinaryHeader header;
        if (!in.read(reinterpret_cast<char*>(&header), sizeof(header)))
            throw std::runtime_error("Failed to read header.");

        std::cout << "--- Header ---" << std::endl;
        std::cout << "Magic:      0x" << std::hex << header.magic << std::dec
                  << " ('" << (char)(header.magic & 0xFF)
                  << (char)((header.magic >> 8) & 0xFF)
                  << (char)((header.magic >> 16) & 0xFF)
                  << (char)((header.magic >> 24) & 0xFF) << "')" << std::endl;
        std::cout << "Version:    " << header.version << std::endl;
        std::cout << "Code Size:  " << header.codeSize << " bytes" << std::endl;
        std::cout << "Data Size:  " << header.dataSize << " bytes" << std::endl;
        std::cout << "Dbg  Size:  " << header.dbgSize << " bytes" << std::endl;
        std::cout << "Entry Point:" << CLR_HEX << header.entryPoint << CLR_RESET << " (offset)" << std::endl;
        std::cout << "--------------" << std::endl << std::endl;

        std::unordered_map<int, std::string> lbls;
        char* dbg = (char*)malloc(header.dbgSize);
        char* dbg_og = dbg;

        in.seekg(sizeof(BinaryHeader) + header.codeSize + header.dataSize);
        in.read((char*)dbg, header.dbgSize);
        in.seekg(sizeof(BinaryHeader));

        while (dbg < dbg_og + header.dbgSize) {
            std::string str = dbg;
            dbg += (strlen(dbg) + 1);
            int addr = *(int*)dbg;
            dbg += sizeof(int);
            lbls[addr] = str;
        }
        free(dbg_og);

        std::vector<uint8_t> code(header.codeSize);
        if (header.codeSize > 0 && !in.read(reinterpret_cast<char*>(code.data()), header.codeSize))
            throw std::runtime_error("Failed to read code segment.");

        std::set<uint32_t> referencedDataOffsets;
        std::cout << "--- Code Segment (Size: " << header.codeSize << ") ---" << std::endl;
        std::cout << "Offset  | Bytes                          | Disassembly" << std::endl;
        std::cout << "--------|--------------------------------|--------------------------------" << std::endl;

        size_t ip = 0;
        int line = 0;
        int label_i = 0;
        while (ip < code.size()) {
            if (ip == header.entryPoint) {
                entry_line = line;
            }
            addr_to_line[ip] = line;
            size_t startIp = ip;
            uint8_t opcodeByte = code[ip++];
            Opcode opcode = static_cast<Opcode>(opcodeByte);

            std::ostringstream diss;
            diss << CLR_OPCODE << std::setw(8) << std::left
                 << (opcodeToString.count(opcode) ? opcodeToString.at(opcode) : ("0x" + std::to_string(opcodeByte)))
                 << CLR_RESET;

            std::vector<std::pair<OperandType, long long>> operands;
            std::string mniFunc;
            size_t tempIp = ip;
            bool unknown = false;

            try {
                int opCount = getOperandCount(opcode);
                if (opcode == MNI) {
                    mniFunc = readString(code, tempIp);
                    // while (tempIp + 1 + size <= code.size()) {
                    while (true) {
                        int size = getOperandSize(code[tempIp]);
                        OperandType t = static_cast<OperandType>(code[tempIp] & 15);
                        tempIp++;
                        if (t == NONE) break;
                        int v = *reinterpret_cast<const int*>(&code[tempIp]);
                        if (size != 4) v &= (1 << (8*size))-1;
                        tempIp += size;
                        operands.emplace_back(t, v);
                        if (t == DATA_ADDRESS) referencedDataOffsets.insert(v);
                    }
                } else if (opCount >= 0) {
                    for (int i = 0; i < opCount; ++i) {
                        int size = getOperandSize(code[tempIp]);
                        if (tempIp + 1 + size > code.size())
                            throw std::runtime_error("Unexpected end of code segment while reading operand");
                        OperandType t = static_cast<OperandType>(code[tempIp++] & 0b1111);
                        if (code[tempIp-1] == 6) t = (OperandType)(t + (1 << 4));

                        long long       v =  (code[tempIp]);
                        if (size >= 2) {v += (code[tempIp+1] << 8);}
                        if (size >= 3) {v += (code[tempIp+2] << 16);}
                        if (size >= 4) {v += (code[tempIp+3] << 24);}
                        if (size >= 5) {v += ((long long)code[tempIp+4] << 32);}
                        if (size == 6) {v += ((long long)code[tempIp+5] << 40);}

                        //if (size != 4) v = v & (1<<(8*(size)))-1;
                        tempIp += size;
                        operands.emplace_back(t, v);
                        if (t == LABEL_ADDRESS) labels.push_back(v);
                        if (t == DATA_ADDRESS) referencedDataOffsets.insert(v);
                    }
                } else if (!opcodeToString.count(opcode)) {
                    throw std::runtime_error("Unknown opcode encountered.");
                }
            } catch (...) {
                unknown = true;
            }

            if (lbls.count(startIp)) {
                std::cout << CLR_OFFSET 
                            << std::setw(7) << std::setfill('0') << startIp
                            << CLR_RESET << " |                                | "
                            << std::endl;
                std::cout << CLR_OFFSET 
                            << std::setw(7) << std::setfill('0') << startIp
                            << CLR_RESET << " |                                | "
                            << CLR_OPCODE << "lbl " << CLR_RESET
                            << CLR_OPERAND << lbls[startIp]
                            << std::endl;
            }

            size_t endIp = tempIp;
            std::cout << CLR_OFFSET << std::setw(7) << std::setfill('0') << startIp << CLR_RESET << " | ";
            std::cout << CLR_HEX;
            printHexBytes(code, startIp, endIp);
            std::cout << CLR_RESET << std::setfill(' ') << std::setw(33 - (3 * (endIp - startIp))) << " | ";

            // Colorize disassembly
            if (opcode == MNI) {
                std::cout << CLR_OPCODE << "MNI" << CLR_RESET << " " << CLR_OPERAND << mniFunc << CLR_RESET;
                std::string op_str = "MNI " + mniFunc;
                for (auto& op : operands) {
                    std::string operand;
                    if (op.first == LABEL_ADDRESS) {
                        if (header.dbgSize != 0) {
                            operand += " " + lbls.at(op.second);
                        } else
                            operand += " #" + std::to_string(op.second);
                    } else {
                        operand += " " + formatOperand(op.first, op.second);
                    }
                    std::cout << " " << CLR_OPERAND << operand << CLR_RESET;
                }
                instructions.push_back(op_str);
                std::cout << std::endl;
            } else if (opcodeToString.count(opcode)) {
                std::string op_str = opcodeToString.at(opcode);
                std::cout << CLR_OPCODE << op_str << CLR_RESET;
                for (auto& op : operands) {
                    std::string operand;
                    if (op.first == LABEL_ADDRESS) {
                        if (header.dbgSize != 0) {
                            operand += " " + lbls.at(op.second);
                        } else
                            operand += " #" + std::to_string(op.second);
                    } else {
                        operand += " " + formatOperand(op.first, op.second);;
                    }
                    std::cout << CLR_OPERAND << operand << CLR_RESET;
                    op_str += operand;
                }
                instructions.push_back(op_str);
                std::cout << std::endl;
            } else {
                std::cout << CLR_ERROR << "Unknown Opcode (0x" << std::hex << (int)opcodeByte << std::dec << ")" << CLR_RESET << std::endl;
                instructions.push_back("Unknown Opcode (0x" + int_to_hex(opcodeByte) + ")");
                unknown = true;
            }
            if (unknown) break;
            ip = endIp;
            line++;
        }
        std::cout << "--------|--------------------------------|--------------------------------" << std::endl << std::endl;

        // Data segment (colorize comments)
        std::cout << "--- Data Segment (Size: " << header.dataSize << ") ---" << std::endl;
        std::vector<char> data(header.dataSize);
        std::vector<bool> processed(header.dataSize, false);
        char* mem_data = (char*)malloc(header.dataSize * sizeof(char));
        char* tmp_data = mem_data;
        in.read(mem_data, header.dataSize);
        while (mem_data < tmp_data+header.dataSize) {
            int16_t addr = *(int16_t*)&mem_data[0];
            int16_t size = *(int16_t*)&mem_data[2];
            mem_data += 4;
            std::string data_string;
            std::string ins = "DB $" + std::to_string((int)addr) + " \"" + repr(mem_data) + "\"";
            std::cout << ins << std::endl;
            instructions.push_back(ins);
            mem_data += size;
        }
        free((char*)tmp_data);

        // for (uint32_t i = 0; i < header.dataSize;) {
        //     if (processed[i]) { ++i; continue; }
        //     bool isRef = referencedDataOffsets.count(i);
        //     std::string s;
        //     bool isStr = false;
        //     uint32_t end = i;
        //     if (isRef) {
        //         uint32_t k = i;
        //         bool possible = true;
        //         while (k < header.dataSize) {
        //             char c = data[k];
        //             if (c == '\0') { end = k; isStr = possible; break; }
        //             if (!std::isprint((unsigned char)c) && c != '\n' && c != '\t') possible = false;
        //             s += c; ++k;
        //         }
        //         if (k == header.dataSize && data[k-1] != '\0') isStr = false;
        //     }
        //     if (isStr) {
        //         std::cout << CLR_COMMENT << "; Referenced Data (String)" << CLR_RESET << std::endl;
        //         std::cout << "DB $" << i << " \"" << escapeString(s) << "\"" << std::endl;
        //         for (uint32_t p = i; p <= end; ++p) processed[p] = true;
        //         i = end + 1;
        //     } else {
        //         if (isRef) std::cout << CLR_COMMENT << "; Referenced Data (Byte)" << CLR_RESET << std::endl;
        //         else std::cout << CLR_COMMENT << "; Unreferenced Data (Byte)" << CLR_RESET << std::endl;
        //         std::cout << "DB $" << i << " 0x"
        //                   << std::setw(2) << std::setfill('0') << std::hex
        //                   << (int)(unsigned char)data[i]
        //                   << std::dec << std::endl;
        //         processed[i] = true;
        //         ++i;
        //     }
        // }
        if (header.dataSize == 0) std::cout << "(Empty)" << std::endl;
        std::cout << "--------------" << std::endl;

        if (argc >= 2) {
            std::string decompiled;
            for (int i=0;i<instructions.size();i++) {
                std::string ins = instructions[i];
                if (header.dbgSize != 0) {
                    for (auto& l : lbls) {
                        if (addr_to_line[l.first]==i) {
                            decompiled += "\nlbl " + l.second + "\n";
                        }
                    }
                } else if (i==entry_line) {
                    decompiled += "\nlbl main\n";
                } else if (contains(i, labels, addr_to_line, &label_i)) {
                    decompiled += "\nlbl label_" + std::to_string(label_i) + "\n";
                }
                decompiled += ins + "\n";
            }
            std::ofstream out(output);
            out << decompiled;
            out.close();
        }
    } catch (const std::exception& e) {
        std::cerr << CLR_ERROR << "Decoding Error: " << e.what() << CLR_RESET << std::endl;
        return 1;
    }
    return 0;
}
