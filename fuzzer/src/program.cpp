#include "program.hpp"
#include "config.hpp"

#include <array>
#include <fstream>
#include <iostream>

Instruction::Instruction(RandomNumberGenerator &rnd) {
    std::vector<InstType> inst_type_list;

    if (Config::instance().enable_rv64a && Config::instance().enable_rv64im) {
        inst_type_list = {InstType::IntRegImm,
                          InstType::IntRegImmShift,
                          InstType::IntRegImmShiftW,
                          InstType::IntRegReg,
                          InstType::Branch,
                          InstType::Memory,
                          InstType::PseudoArg0,
                          InstType::PseudoLi,
                          InstType::PseudoLoadAddress,
                          InstType::AtomicArg2,
                          InstType::AtomicArg3};
    } else if (Config::instance().enable_rv64a) {
        inst_type_list = {InstType::IntRegImm,  InstType::IntRegImmShift,
                          InstType::IntRegReg,  InstType::Branch,
                          InstType::Memory,     InstType::PseudoArg0,
                          InstType::PseudoLi,   InstType::PseudoLoadAddress,
                          InstType::AtomicArg2, InstType::AtomicArg3};
    } else if (Config::instance().enable_rv64im) {
        inst_type_list = {InstType::IntRegImm,        InstType::IntRegImmShift,
                          InstType::IntRegImmShiftW,  InstType::IntRegReg,
                          InstType::Branch,           InstType::Memory,
                          InstType::PseudoArg0,       InstType::PseudoLi,
                          InstType::PseudoLoadAddress};
    } else {
        inst_type_list = {InstType::IntRegImm, InstType::IntRegImmShift,
                          InstType::IntRegReg, InstType::Branch,
                          InstType::Memory,    InstType::PseudoArg0,
                          InstType::PseudoLi,  InstType::PseudoLoadAddress};
    }

    type = inst_type_list[rnd(inst_type_list.size())];
    opcode = random_opcode(type, rnd);
    operands = random_operands(type, rnd);
}

Instruction::Instruction(const Instruction &r)
    : type(r.type), opcode(r.opcode), operands(r.operands){};

Instruction::Instruction(const InstType type, const std::string &opcode,
                         const std::vector<std::string> &operands)
    : type(type), opcode(opcode), operands(operands){};

Instruction &Instruction::operator=(const Instruction &rhs) {
    if (this != &rhs) {
        type = rhs.type;
        opcode = rhs.opcode;
        operands = rhs.operands;
    }
    return *this;
}

std::string Instruction::random_opcode(InstType type,
                                       RandomNumberGenerator &rnd) {
    switch (type) {
    case InstType::IntRegImm: {
#if defined(ENABLE_RV64IM)
        static const std::array<std::string, 7> opcode_list = {
            "addi", "slti", "sltiu", "xori", "ori", "andi",
            "addiw" // RV64I
        };
#else
        static const std::array<std::string, 6> opcode_list = {
            "addi", "slti", "sltiu", "xori", "ori", "andi"};
#endif
        return opcode_list[rnd(0, opcode_list.size())];
    }
    case InstType::IntRegImmShift: {
        static const std::array<std::string, 3> opcode_list = {"slli", "srli",
                                                               "srai"};
        return opcode_list[rnd(0, opcode_list.size())];
    }
    case InstType::IntRegImmShiftW: {
        static const std::array<std::string, 3> opcode_list = {
            "slliw", "srliw", "sraiw" // RV64I
        };
        return opcode_list[rnd(0, opcode_list.size())];
    }
    case InstType::IntRegReg: {
#if defined(ENABLE_RV64IM)
        static const std::array<std::string, 28> opcode_list = {
            "add",  "sub",  "sll",   "slt",  "sltu",   "xor",   "srl",  "sra",
            "or",   "and",  "mul",   "mulh", "mulhsu", "mulhu", "div",  "divu",
            "rem",  "remu", "addw",  "subw", "sllw",   "srlw",  "sraw", // RV64I
            "mulw", "divw", "divuw", "remw", "remuw"                    // RV64M
        };
#else
        static const std::array<std::string, 18> opcode_list = {
            "add",    "sub",   "sll", "slt",  "sltu", "xor",
            "srl",    "sra",   "or",  "and",  "mul",  "mulh",
            "mulhsu", "mulhu", "div", "divu", "rem",  "remu"};
#endif
        return opcode_list[rnd(0, opcode_list.size())];
    }
    case InstType::Branch: {
        static const std::array<std::string, 6> opcode_list = {
            "beq", "bne", "blt", "bge", "bltu", "bgeu"};
        return opcode_list[rnd(0, opcode_list.size())];
    }
    case InstType::Memory: {
#if defined(ENABLE_RV64IM)
        static const std::array<std::string, 11> opcode_list = {
            "lb", "lh", "lw",  "lbu", "lhu", "sb",
            "sh", "sw", "lwu", "ld",  "sd" // RV64I
        };
#else
        static const std::array<std::string, 8> opcode_list = {
            "lb", "lh", "lw", "lbu", "lhu", "sb", "sh", "sw"};
#endif
        return opcode_list[rnd(0, opcode_list.size())];
    }
    case InstType::PseudoArg0: {
        static const std::array<std::string, 2> opcode_list = {"nop", "fence"};
        return opcode_list[rnd(0, opcode_list.size())];
    }
    case InstType::PseudoLi:
        return "li";
    case InstType::PseudoLoadAddress:
        return "la";
    case InstType::AtomicArg2:
        static const std::array<std::string, 2> opcode_list = {
            "lr.w", // RV32A
            "lr.d"  // RV64A
        };
        return opcode_list[rnd(0, opcode_list.size())];
    case InstType::AtomicArg3: {
        static const std::array<std::string, 20> opcode_list = {
            // RV32A
            "sc.w", "amoswap.w", "amoadd.w", "amoxor.w", "amoand.w", "amoor.w",
            "amomin.w", "amomax.w", "amominu.w", "amomaxu.w",
            // RV64A
            "sc.d", "amoswap.d", "amoadd.d", "amoxor.d", "amoand.d", "amoor.d",
            "amomin.d", "amomax.d", "amominu.d", "amomaxu.d"};
        return opcode_list[rnd(0, opcode_list.size())];
    }
    default:
        __builtin_unreachable();
    }
}

std::vector<std::string>
Instruction::random_operands(InstType type, RandomNumberGenerator &rnd) {
    std::vector<std::string> operands;
    switch (type) {
    case InstType::IntRegImm:
    case InstType::IntRegImmShift:
    case InstType::IntRegImmShiftW:
    case InstType::IntRegReg:
    case InstType::Branch:
    case InstType::Memory:
    case InstType::AtomicArg3:
        for (std::size_t i = 0; i < 3; ++i) {
            operands.emplace_back(random_operand(type, i, rnd));
        }
        break;
    case InstType::PseudoArg0:
        break;
    case InstType::PseudoLi:
    case InstType::AtomicArg2:
        for (std::size_t i = 0; i < 2; ++i) {
            operands.emplace_back(random_operand(type, i, rnd));
        }
        break;
    case InstType::PseudoLoadAddress:
        for (std::size_t i = 0; i < 2; ++i) {
            operands.emplace_back(random_operand(type, i, rnd));
        }
        break;
    default:
        __builtin_unreachable();
    }
    return operands;
}

std::string Instruction::random_operand(InstType type, std::size_t index,
                                        RandomNumberGenerator &rnd) {
    switch (type) {
    case InstType::IntRegImm:
        switch (index) {
        case 0:
            return Instruction::random_register(rnd);
        case 1:
            return Instruction::random_register(rnd);
        case 2:
            return std::to_string(rnd(-pow(2, 11), pow(2, 11)));
        default:
            __builtin_unreachable();
        }
        break;
    case InstType::IntRegImmShift:
        switch (index) {
        case 0:
            return Instruction::random_register(rnd);
        case 1:
            return Instruction::random_register(rnd);
        case 2:
            if (Config::instance().enable_rv64im) {
                return std::to_string(rnd(64));
            } else {
                return std::to_string(rnd(32));
            }
        default:
            __builtin_unreachable();
        }
        break;
    case InstType::IntRegImmShiftW:
        switch (index) {
        case 0:
            return Instruction::random_register(rnd);
        case 1:
            return Instruction::random_register(rnd);
        case 2:
            return std::to_string(rnd(32));
        default:
            __builtin_unreachable();
        }
        break;
    case InstType::IntRegReg:
    case InstType::AtomicArg3:
        switch (index) {
        case 0:
            return Instruction::random_register(rnd);
        case 1:
            return Instruction::random_register(rnd);
        case 2:
            return Instruction::random_register(rnd);
        default:
            __builtin_unreachable();
        }
        break;
    case InstType::Branch:
        switch (index) {
        case 0:
            return Instruction::random_register(rnd);
        case 1:
            return Instruction::random_register(rnd);
        case 2:
            return Instruction::random_label(rnd);
        default:
            __builtin_unreachable();
        }
        break;
    case InstType::Memory:
        switch (index) {
        case 0:
            return Instruction::random_register(rnd);
        case 1:
            return Instruction::random_register(rnd);
        // TODO: address offset range
        case 2:
            return std::to_string(rnd(pow(2, 11)));
        default:
            __builtin_unreachable();
        }
        break;
    case InstType::PseudoArg0:
        __builtin_unreachable();
    case InstType::PseudoLi:
        switch (index) {
        case 0:
            return Instruction::random_register(rnd);
        case 1:
            return std::to_string(rnd(0, 32));
        default:
            __builtin_unreachable();
        }
        break;
    case InstType::PseudoLoadAddress:
        switch (index) {
        case 0:
            return Instruction::random_register(rnd);
        case 1:
            return "test_memory + " + std::to_string(rnd(pow(2, 11)));
        default:
            __builtin_unreachable();
        }
        break;
    case InstType::AtomicArg2:
        switch (index) {
        case 0:
            return Instruction::random_register(rnd);
        case 1:
            return Instruction::random_register(rnd);
        default:
            __builtin_unreachable();
        }
        break;
    default:
        __builtin_unreachable();
    }
}

std::string Instruction::random_register(RandomNumberGenerator &rnd) {
    int r_num = 0;
    while (1) {
        int r = rnd(32);
        // Avoid using the x1 register as it is used to store the return
        // address. Avoid using the x2 register as it is used to store the stack
        // pointer.
        if (r == 1 || r == 2) {
            continue;
        }
        r_num = r;
        break;
    }
    return "x" + std::to_string(r_num);
}

std::string Instruction::random_label(RandomNumberGenerator &rnd) {
    int r = rnd(Config::instance().initial_seed_block_count);
    return "label_" + std::to_string(r);
}

std::string Instruction::generate() const {
    std::string inst = opcode + " ";
    if (type == InstType::Memory) {
        inst += operands[0] + ", " + operands[2] + "(" + operands[1] + ")";
    } else if (type == InstType::AtomicArg2) {
        inst += operands[0] + ", (" + operands[1] + ")";
    } else if (type == InstType::AtomicArg3) {
        inst += operands[0] + ", " + operands[1] + ", (" + operands[2] + ")";
    } else {
        if (operands.size() > 0) {
            for (std::size_t i = 0; i < operands.size() - 1; ++i) {
                inst += operands[i] + ", ";
            }
            inst += operands.back();
        }
    }
    return inst;
}

InstType Instruction::Type() { return this->type; }
void Instruction::Opecode(std::string opcode) { this->opcode = opcode; }
std::vector<std::string> Instruction::Operands() { return this->operands; }
void Instruction::Operands(std::vector<std::string> operands) {
    this->operands = operands;
}
void Instruction::Operand(std::string operand, std::size_t index) {
    this->operands[index] = operand;
}

Block::Block(std::string label, int num, RandomNumberGenerator &rnd)
    : label(label) {
    for (int i = 0; i < num; ++i) {
        insts.emplace_back(Instruction(rnd));
    }
}

Block::Block(const Block &r) : label(r.label), insts(r.insts){};

std::string Block::generate() const {
    std::string code = label + ":\n";
    for (const Instruction &inst : insts) {
        code += "    ";
        code += inst.generate();
        code += "\n";
    }
    return code;
}

void Block::mutate(RandomNumberGenerator &rnd) {
    const int num_operations =
        Config::instance().enable_insert_memory_access_sequence ? 8 : 7;

    if (rnd(2) == 0) {
        for (int i = 0, cnt = 1 + rnd(Config::instance().max_operation_count);
             i < cnt; i++) {
            switch (rnd(num_operations)) {
            case 0:
                swap_inst(rnd);
                break;
            case 1:
                mutate_opcode(rnd);
                break;
            case 2:
                mutate_operands(rnd);
                break;
            case 3:
                mutate_operand(rnd);
                break;
            case 4:
                mutate_inst(rnd);
                break;
            case 5:
                insert_inst(rnd);
                break;
            case 6:
                remove_inst(rnd);
                break;
            case 7:
                insert_memory_access_sequence(rnd);
                break;
            default:
                __builtin_unreachable();
            }
        }
    } else {
        switch (rnd(num_operations)) {
        case 0:
            for (int i = 0,
                     cnt = 1 + rnd(Config::instance().max_operation_count);
                 i < cnt; i++) {
                swap_inst(rnd);
            }
            break;
        case 1:
            for (int i = 0,
                     cnt = 1 + rnd(Config::instance().max_operation_count);
                 i < cnt; i++) {
                mutate_opcode(rnd);
            }
            break;
        case 2:
            for (int i = 0,
                     cnt = 1 + rnd(Config::instance().max_operation_count);
                 i < cnt; i++) {
                mutate_operands(rnd);
            }
            break;
        case 3:
            for (int i = 0,
                     cnt = 1 + rnd(Config::instance().max_operation_count);
                 i < cnt; i++) {
                mutate_operand(rnd);
            }
            break;
        case 4:
            for (int i = 0,
                     cnt = 1 + rnd(Config::instance().max_operation_count);
                 i < cnt; i++) {
                mutate_inst(rnd);
            }
            break;
        case 5:
            for (int i = 0,
                     cnt = 1 + rnd(Config::instance().max_operation_count);
                 i < cnt; i++) {
                insert_inst(rnd);
            }
            break;
        case 6:
            for (int i = 0,
                     cnt = 1 + rnd(Config::instance().max_operation_count);
                 i < cnt; i++) {
                remove_inst(rnd);
            }
            break;
        case 7:
            for (int i = 0,
                     cnt = 1 + rnd(Config::instance().max_operation_count);
                 i < cnt; i++) {
                insert_memory_access_sequence(rnd);
            }
            break;
        default:
            __builtin_unreachable();
        }
    }
}

void Block::swap_inst(RandomNumberGenerator &rnd) {
    int i = rnd(0, this->insts.size());
    int j = rnd(0, this->insts.size());
    std::swap(this->insts[i], this->insts[j]);
}

void Block::mutate_opcode(RandomNumberGenerator &rnd) {
    int i = rnd(0, this->insts.size());
    InstType type = this->insts[i].Type();
    this->insts[i].Opecode(Instruction::random_opcode(type, rnd));
}

void Block::mutate_operands(RandomNumberGenerator &rnd) {
    int i = rnd(0, this->insts.size());
    InstType type = this->insts[i].Type();
    this->insts[i].Operands(Instruction::random_operands(type, rnd));
}

void Block::mutate_operand(RandomNumberGenerator &rnd) {
    int inst_idx = rnd(this->insts.size());
    std::vector<std::string> operands = this->insts[inst_idx].Operands();

    if (operands.size() == 0) {
        return;
    }

    InstType type = this->insts[inst_idx].Type();
    int operand_idx = rnd(operands.size());
    this->insts[inst_idx].Operand(
        Instruction::random_operand(type, operand_idx, rnd), operand_idx);
}

void Block::mutate_inst(RandomNumberGenerator &rnd) {
    int i = rnd(0, this->insts.size());
    Instruction inst = Instruction(rnd);
    this->insts[i] = inst;
}

void Block::insert_inst(RandomNumberGenerator &rnd) {
    int i = rnd(0, this->insts.size());
    Instruction inst = Instruction(rnd);
    this->insts.insert(this->insts.begin() + i, inst);
}

void Block::remove_inst(RandomNumberGenerator &rnd) {
    if (this->insts.size() > 1) {
        int i = rnd(0, this->insts.size());
        this->insts.erase(this->insts.begin() + i);
    }
}

void Block::insert_memory_access_sequence(RandomNumberGenerator &rnd) {
    // ld xReg1, memory + N
    Instruction load_address_inst(
        InstType::PseudoLoadAddress,
        Instruction::random_opcode(InstType::PseudoLoadAddress, rnd),
        Instruction::random_operands(InstType::PseudoLoadAddress, rnd));
    // load/store xReg2, 0(xReg1)
    Instruction memory_access_inst(
        InstType::Memory, Instruction::random_opcode(InstType::Memory, rnd),
        {Instruction::random_operand(InstType::Memory, 0, rnd),
         load_address_inst.Operands().at(0), "0"});

    int i = rnd(0, this->insts.size());
    this->insts.insert(this->insts.begin() + i, load_address_inst);
    this->insts.insert(this->insts.begin() + i + 1, memory_access_inst);
}

Program::Program() : blocks(std::vector<Block>()) {}

Program::Program(int block_num, int inst_num, RandomNumberGenerator &rnd) {
    for (int i = 0; i < block_num; ++i) {
        std::string label = "label_" + std::to_string(i);
        this->blocks.emplace_back(Block(label, inst_num, rnd));
    }
}

std::string Program::generate() const {
    std::string code = "";
    for (const Block &block : this->blocks) {
        code += block.generate();
        code += "\n";
    }
    return code;
}

void Program::mutate(RandomNumberGenerator &rnd) {
    for (int i = 0; i < 1 + rnd(2); ++i) {
        this->blocks[rnd(blocks.size())].mutate(rnd);
    }
}

void Program::write_to_file(const std::filesystem::path &filepath,
                            const std::string &header,
                            const std::string &footer) const {
    std::ofstream ofs(filepath, std::ios::out | std::ios::trunc);
    if (!ofs) {
        std::cerr << "Failed to open " << filepath
                  << " for outputting a input as file." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (not header.empty()) {
        ofs << header;
    }
    ofs << generate();
    if (not footer.empty()) {
        ofs << footer;
    }
    ofs.close();
}
