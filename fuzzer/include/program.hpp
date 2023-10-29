#pragma once

#include "common.hpp"

#include <filesystem>
#include <string>
#include <vector>

enum class InstType {
    IntRegImm,
    IntRegImmShift,
    IntRegImmShiftW,
    IntRegReg,
    Branch,
    Memory,
    PseudoArg0,
    PseudoLi,
    PseudoLoadAddress,
    AtomicArg2,
    AtomicArg3
};

class Instruction {
  public:
    Instruction(RandomNumberGenerator &rnd);
    Instruction(const Instruction &r);
    Instruction(const InstType type, const std::string &opcode,
                const std::vector<std::string> &operands);

    Instruction &operator=(const Instruction &rhs);

    static std::string random_opcode(InstType type, RandomNumberGenerator &rnd);
    static std::vector<std::string> random_operands(InstType type,
                                                    RandomNumberGenerator &rnd);
    static std::string random_operand(InstType type, std::size_t index,
                                      RandomNumberGenerator &rnd);
    static std::string random_register(RandomNumberGenerator &rnd);
    static std::string random_label(RandomNumberGenerator &rnd);
    std::string generate() const;
    InstType Type();
    void Opecode(std::string opcode);
    std::vector<std::string> Operands();
    void Operands(std::vector<std::string> operands);
    void Operand(std::string operand, std::size_t index);

  private:
    InstType type;
    std::string opcode;
    std::vector<std::string> operands;
};

class Block {
  public:
    Block(std::string label, int num, RandomNumberGenerator &rnd);
    Block(const Block &r);
    std::string generate() const;
    void mutate(RandomNumberGenerator &rnd);

  private:
    std::string label;
    std::vector<Instruction> insts;

    void swap_inst(RandomNumberGenerator &rnd);
    void mutate_opcode(RandomNumberGenerator &rnd);
    void mutate_operands(RandomNumberGenerator &rnd);
    void mutate_operand(RandomNumberGenerator &rnd);
    void mutate_inst(RandomNumberGenerator &rnd);
    void insert_inst(RandomNumberGenerator &rnd);
    void remove_inst(RandomNumberGenerator &rnd);
    void insert_memory_access_sequence(RandomNumberGenerator &rnd);
};

class Program {
  public:
    Program();
    Program(int block_num, int inst_num, RandomNumberGenerator &rnd);
    std::string generate() const;
    void mutate(RandomNumberGenerator &rnd);
    void write_to_file(const std::filesystem::path &filepath,
                       const std::string &header = "",
                       const std::string &footer = "") const;

  private:
    std::vector<Block> blocks;
};
