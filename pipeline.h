#ifndef PIPELINE_FILE
#define PIPELINE_FILE
#include <vector>
#include <cstdint>
#include <iostream>

struct FetchDecodePipeReg {
    uint32_t pc = 0;
    uint32_t instruction = 0;
};

struct DecodeExPipeReg {
    //Control
    bool reg_dest_control = 0; 
    bool jump_control = 0;
    bool jump_reg_control = 0;
    bool link_control = 0;
    bool shift_control = 0;
    bool branch_control = 0;
    bool bne_control = 0;
    bool mem_read_control = 0;
    bool mem_to_reg_control = 0;
    unsigned ALU_op_control : 2; 
    bool mem_write_control = 0;
    bool halfword_control = 0;
    bool byte_control = 0;
    bool ALU_src_control = 0;
    bool reg_write_control = 0;
    bool zero_extend_control = 0;

    //Main Reg
    uint32_t read_data_1 = 0;
    uint32_t read_data_2 = 0;


    int opcode = 0;
    int addr = 0;
    int rt = 0;
    int rs = 0;
    int rd = 0;
    int shamt = 0;
    int funct = 0;
    uint32_t imm = 0;
    uint32_t pc = 0;
};

struct ExMemPipeReg {
    // bool reg_dest_control; 
    // bool jump_control;
    // bool jump_reg_control;
    bool link_control = 0;
    // bool shift_control;
    bool branch_control = 0;
    bool bne_control = 0;   
    bool mem_read_control = 0;
    bool mem_to_reg_control = 0;
    // bool ALU_op_control;
    bool mem_write_control = 0;
    bool halfword_control = 0;
    bool byte_control = 0;
    // bool ALU_src_control;
    bool reg_write_control = 0;
    // bool zero_extend_control;
    
    uint32_t pc_add_result = 0;
    uint32_t pc = 0;
    uint32_t orig_pc = 0;

    uint32_t alu_zero = 0;
    uint32_t alu_result = 0;
    uint32_t read_data_2 = 0;
    uint32_t write_reg = 0;
};

struct MemWBPipeReg {
    // bool reg_dest_control; 
    // bool jump_control;
    // bool jump_reg_control;
    bool link_control = 0;
    // bool shift_control;
    // bool branch_control;
    // bool bne_control;
    // bool mem_read_control;
    bool mem_to_reg_control = 0;
    // bool ALU_op_control;
    // bool mem_write_control;
    // bool halfword_control;
    // bool byte_control;
    // bool ALU_src_control;
    bool reg_write_control = 0;
    // bool zero_extend_control;

    uint32_t read_data_mem = 0;
    uint32_t alu_result = 0; 
    uint32_t pc = 0;
    int write_reg = 0;
};  



#endif
