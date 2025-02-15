#include <cstdint>
#include <iostream>
#include "processor.h"
using namespace std;

#ifdef ENABLE_DEBUG
#define DEBUG(x) x
#else
#define DEBUG(x) 
#endif

void Processor::initialize(int level) {
    // Initialize Control
    control = {.reg_dest = 0, 
               .jump = 0,
               .jump_reg = 0,
               .link = 0,
               .shift = 0,
               .branch = 0,
               .bne = 0,
               .mem_read = 0,
               .mem_to_reg = 0,
               .ALU_op = 0,
               .mem_write = 0,
               .halfword = 0,
               .byte = 0,
               .ALU_src = 0,
               .reg_write = 0,
               .zero_extend = 0};
   
    opt_level = level;
    // Optimization level-specific initialization
}

void Processor::advance() {
    switch (opt_level) {
        case 0: single_cycle_processor_advance();
                break;
        case 1: pipelined_processor_advance();
                break;
        // other optimization levels go here
        default: break;
    }
}

void Processor::single_cycle_processor_advance() {
    // fetch
    uint32_t instruction;
    memory->access(regfile.pc, instruction, 0, 1, 0);
    DEBUG(cout << "\nPC: 0x" << std::hex << regfile.pc << std::dec << "\n");
    // increment pc
    regfile.pc += 4;
    
    // decode into contol signals
    control.decode(instruction);
    DEBUG(control.print());

    // extract rs, rt, rd, imm, funct 
    int opcode = (instruction >> 26) & 0x3f;
    int rs = (instruction >> 21) & 0x1f;
    int rt = (instruction >> 16) & 0x1f;
    int rd = (instruction >> 11) & 0x1f;
    int shamt = (instruction >> 6) & 0x1f;
    int funct = instruction & 0x3f;
    uint32_t imm = (instruction & 0xffff);
    int addr = instruction & 0x3ffffff;
    // Variables to read data into
    uint32_t read_data_1 = 0;
    uint32_t read_data_2 = 0;
    
    // Read from reg file
    regfile.access(rs, rt, read_data_1, read_data_2, 0, 0, 0);
    
    // Execution 
    alu.generate_control_inputs(control.ALU_op, funct, opcode);
   
    // Sign Extend Or Zero Extend the immediate
    // Using Arithmetic right shift in order to replicate 1 
    imm = control.zero_extend ? imm : (imm >> 15) ? 0xffff0000 | imm : imm;
    
    // Find operands for the ALU Execution
    // Operand 1 is always R[rs] -> read_data_1, except sll and srl
    // Operand 2 is immediate if ALU_src = 1, for I-type
    uint32_t operand_1 = control.shift ? shamt : read_data_1;
    uint32_t operand_2 = control.ALU_src ? imm : read_data_2;
    uint32_t alu_zero = 0;

    uint32_t alu_result = alu.execute(operand_1, operand_2, alu_zero);
    
    
    uint32_t read_data_mem = 0;
    uint32_t write_data_mem = 0;

    // Memory
    // First read no matter whether it is a load or a store
    memory->access(alu_result, read_data_mem, 0, control.mem_read | control.mem_write, 0);
    // Stores: sb or sh mask and preserve original leftmost bits
    write_data_mem = control.halfword ? (read_data_mem & 0xffff0000) | (read_data_2 & 0xffff) : 
                    control.byte ? (read_data_mem & 0xffffff00) | (read_data_2 & 0xff): read_data_2;
    // Write to memory only if mem_write is 1, i.e store
    memory->access(alu_result, read_data_mem, write_data_mem, control.mem_read, control.mem_write);
    // Loads: lbu or lhu modify read data by masking
    read_data_mem &= control.halfword ? 0xffff : control.byte ? 0xff : 0xffffffff;

    int write_reg = control.link ? 31 : control.reg_dest ? rd : rt;

    uint32_t write_data = control.link ? regfile.pc+8 : control.mem_to_reg ? read_data_mem : alu_result;  

    // Write Back
    regfile.access(0, 0, read_data_2, read_data_2, write_reg, control.reg_write, write_data);
    
    // Update PC
    regfile.pc += (control.branch && !control.bne && alu_zero) || (control.bne && !alu_zero) ? imm << 2 : 0; 
    regfile.pc = control.jump_reg ? read_data_1 : control.jump ? (regfile.pc & 0xf0000000) & (addr << 2): regfile.pc;
}

void Processor::fetch_stage(){
    uint32_t instruction;
    // fetch
    memory->access(regfile.pc, instruction, 0, 1, 0);
    // increment pc
    regfile.pc += 4;
    // pass variables
    FDReg.pc = regfile.pc;
    FDReg.instruction = instruction;


}
void Processor::decode_stage(){
    uint32_t instruction;
    instruction = FDReg.instruction;
    // decode into contol signals
    control.decode(instruction);

    // extract rs, rt, rd, imm, funct 
    int opcode = (instruction >> 26) & 0x3f;
    int rs = (instruction >> 21) & 0x1f;
    int rt = (instruction >> 16) & 0x1f;
    int rd = (instruction >> 11) & 0x1f;
    int shamt = (instruction >> 6) & 0x1f;
    int funct = instruction & 0x3f;
    uint32_t imm = (instruction & 0xffff);
    int addr = instruction & 0x3ffffff;
    // Variables to read data into
    uint32_t read_data_1 = 0;
    uint32_t read_data_2 = 0;

    // Read from reg file
    regfile.access(rs, rt, read_data_1, read_data_2, 0, 0, 0);

    // Sign Extend Or Zero Extend the immediate
    // Using Arithmetic right shift in order to replicate 1 
    imm = control.zero_extend ? imm : (imm >> 15) ? 0xffff0000 | imm : imm;

    // Pass Variables
    // Control
    DXReg.reg_dest_control = control.reg_dest;
    DXReg.jump_control = control.jump;
    DXReg.jump_reg_control = control.jump_reg;
    DXReg.link_control = control.link;
    DXReg.shift_control = control.shift;
    DXReg.branch_control = control.branch;
    DXReg.bne_control = control.bne;
    DXReg.mem_read_control = control.mem_read;
    DXReg.mem_to_reg_control = control.mem_to_reg;
    DXReg.ALU_op_control = control.ALU_op;
    DXReg.mem_write_control = control.mem_write;
    DXReg.halfword_control = control.halfword;
    DXReg.byte_control = control.byte;
    DXReg.ALU_src_control = control.ALU_src;
    DXReg.reg_write_control = control.reg_write;
    DXReg.zero_extend_control = control.zero_extend;

    // Instructions
    DXReg.imm = imm;
    DXReg.read_data_1 = read_data_1;
    DXReg.read_data_2 = read_data_2;
    DXReg.pc = FDReg.pc;
    DXReg.rd = rd;
    DXReg.rt = rt;
    DXReg.addr = addr;
    DXReg.shamt = shamt;
    DXReg.opcode = opcode;
    DXReg.funct = funct;
}

void Processor::execute_stage(){
    alu.generate_control_inputs(DXReg.ALU_op_control, DXReg.funct, DXReg.opcode);

    // Find operands for the ALU Execution
    // Operand 1 is always R[rs] -> read_data_1, except sll and srl
    // Operand 2 is immediate if ALU_src = 1, for I-type
    uint32_t operand_1 = DXReg.shift_control ? DXReg.shamt : DXReg.read_data_1;
    uint32_t operand_2 = DXReg.ALU_src_control ? DXReg.imm : DXReg.read_data_2;
    uint32_t alu_zero = 0;

    uint32_t alu_result = alu.execute(operand_1, operand_2, alu_zero);
    XMReg.alu_zero = alu_zero;
    XMReg.alu_result = alu_result;
    
    int write_reg = DXReg.link_control ? 31 : DXReg.reg_dest_control ? DXReg.rd : DXReg.rt;  
    XMReg.write_reg = write_reg;

    XMReg.pc_add_result = DXReg.jump_reg_control ? DXReg.read_data_1 : DXReg.jump_control ? (DXReg.pc & 0xf0000000) & (DXReg.addr << 2): DXReg.pc;

    // Pass Variables
    // Control

    XMReg.link_control = DXReg.link_control;
    XMReg.branch_control = DXReg.branch_control;
    XMReg.bne_control = DXReg.bne_control;
    XMReg.mem_read_control = DXReg.mem_read_control;
    XMReg.mem_to_reg_control = DXReg.mem_to_reg_control;
    XMReg.mem_write_control = DXReg.mem_write_control;
    XMReg.halfword_control = DXReg.halfword_control;
    XMReg.byte_control = DXReg.byte_control;
    XMReg.reg_write_control = DXReg.reg_write_control;

    XMReg.read_data_2 = DXReg.read_data_2;
}    

void Processor::memory_stage(){
    uint32_t read_data_mem = 0;
    uint32_t write_data_mem = 0;

      // First read no matter whether it is a load or a store
    memory->access(XMReg.alu_result, read_data_mem, 0, XMReg.mem_read_control | XMReg.mem_write_control, 0);
    // Stores: sb or sh mask and preserve original leftmost bits
    write_data_mem = XMReg.halfword_control ? (read_data_mem & 0xffff0000) | (XMReg.read_data_2 & 0xffff) : 
                    XMReg.byte_control ? (read_data_mem & 0xffffff00) | (XMReg.read_data_2 & 0xff): XMReg.read_data_2;
    // Write to memory only if mem_write is 1, i.e store
    memory->access(XMReg.alu_result, read_data_mem, write_data_mem, XMReg.mem_read_control, XMReg.mem_write_control);
    // Loads: lbu or lhu modify read data by masking
    read_data_mem &= XMReg.halfword_control ? 0xffff : XMReg.byte_control ? 0xff : 0xffffffff;

    //Stopped Here

    // Passing Values
    MWBReg.write_reg = XMReg.write_reg

}

void Processor::write_back_stage() {
    uint32_t read_data_dummy;
    uint32_t write_data = MWBReg.link_control ? regfile.pc+8 : MWBReg.mem_to_reg_control ? MWBReg.read_data_mem : MWBReg.alu_result;  
    regfile.access(0, 0, read_data_dummy, read_data_dummy, MWBReg.write_reg, MWBReg.reg_write_control, write_data);
}

void Processor::pipelined_processor_advance() { 
    // pipelined processor logic goes here
    // does nothing currently -- if you call it from the cmd line, you'll run into an infinite loop
    // might be helpful to implement stages in a separate module


        
    write_back_stage();
    memory_stage();
    execute_stage();
    decode_stage();
    fetch_stage();


}
