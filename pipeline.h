#ifndef PIPELINE_FILE
#define PIPELINE_FILE
#include <vector>
#include <cstdint>
#include <iostream>

struct FetchDecodePipeReg {
    uint32_t pc;
    uint32_t instruction;
};

struct DecodeExPipeReg {
    //Control
    bool reg_dest_control; 
    bool jump_control;
    bool jump_reg_control;
    bool link_control;
    bool shift_control;
    bool branch_control;
    bool bne_control;
    bool mem_read_control;
    bool mem_to_reg_control;
    bool ALU_op_control;
    bool mem_write_control;
    bool halfword_control;
    bool byte_control;
    bool ALU_src_control;
    bool reg_write_control;
    bool zero_extend_control;

    //Main Reg
    uint32_t read_data_1;
    uint32_t read_data_2;


    int opcode;
    int rt;
    int rd;
    int shamt;
    int funct;
    uint32_t imm;
    uint32_t pc;
};

struct ExMemPipeReg {
    // bool reg_dest_control; 
    // bool jump_control;
    // bool jump_reg_control;
    bool link_control;
    // bool shift_control;
    bool branch_control;
    bool bne_control;   
    bool mem_read_control;
    bool mem_to_reg_control;
    // bool ALU_op_control;
    bool mem_write_control;
    bool halfword_control;
    bool byte_control;
    // bool ALU_src_control;
    bool reg_write_control;
    // bool zero_extend_control;
    
    uint32_t pc_add_result;

    uint32_t alu_zero ;
    uint32_t alu_result;
    uint32_t read_data_2;
    uint32_t write_reg;
};

struct MemWBPipeReg {
    // bool reg_dest_control; 
    // bool jump_control;
    // bool jump_reg_control;
    bool link_control;
    // bool shift_control;
    // bool branch_control;
    // bool bne_control;
    // bool mem_read_control;
    bool mem_to_reg_control;
    // bool ALU_op_control;
    // bool mem_write_control;
    // bool halfword_control;
    // bool byte_control;
    // bool ALU_src_control;
    bool reg_write_control;
    // bool zero_extend_control;

    uint32_t read_data_mem;
    uint32_t alu_result; 
    int write_reg;
};  



#endif
