#include <cstdint>
#include <iostream>
#include "processor.h"
#include <string>
using namespace std;
#define ENABLE_DEBUG

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

    table = {{}, {}, {}, {}, {}};
    table2 = {{}, {}, {}, {}, {}, {}};
    // Optimization level-specific initialization
}

void Processor::advance() {
    switch (opt_level) {
        case 0: single_cycle_processor_advance();
                break;
        case 1: pipelined_processor_advance();
                break;
        case 2: out_of_order_advance();
        // other optimization levels go here
        default: break;
    }
}

uint32_t Processor::getPC(){
    switch (opt_level)
    {
        case 0:
            return regfile.pc;
            break;
        case 1:
            return finishedPC;
            break;
        default:
            return finishedPC;
            break;
    }
    return 0;
}

void Processor::single_cycle_processor_advance() {
    // fetch
    uint32_t instruction;
    memory->access(regfile.pc, instruction, 0, 1, 0);
    DEBUG(cout << "\nPC: "  << regfile.pc << std::dec << " inst: " << instruction << "\n");
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
    DEBUG(cout << "ALU op: " << control.ALU_op << " Funct: " << funct << " opcode: " << opcode << "\n";)
   
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
    DEBUG(cout << "op1 " << operand_1 << " op2 " << operand_2 << " alu_zero " << alu_zero << " alu result " << alu_result << "\n";)
    
    
    uint32_t read_data_mem = 0;
    uint32_t write_data_mem = 0;

    // Memory
    // First read no matter whether it is a load or a store
    memory->access(alu_result, read_data_mem, 0, control.mem_read | control.mem_write, 0);
    DEBUG(cout << "read data mem: " << read_data_mem << " mem read control: " << control.mem_read << " Resulting alu result " << alu_result << "\n";)

    // Stores: sb or sh mask and preserve original leftmost bits
    write_data_mem = control.halfword ? (read_data_mem & 0xffff0000) | (read_data_2 & 0xffff) : 
                    control.byte ? (read_data_mem & 0xffffff00) | (read_data_2 & 0xff): read_data_2;

    DEBUG(cout << "read data 2: " << read_data_2 << "\n";)
    // Write to memory only if mem_write is 1, i.e store
    memory->access(alu_result, read_data_mem, write_data_mem, control.mem_read, control.mem_write);
    DEBUG(cout << "write data mem: " << write_data_mem << " mem write control: " << control.mem_write << " Resulting alu result " << alu_result << "\n";)

    // Loads: lbu or lhu modify read data by masking
    read_data_mem &= control.halfword ? 0xffff : control.byte ? 0xff : 0xffffffff;

    int write_reg = control.link ? 31 : control.reg_dest ? rd : rt;

    uint32_t write_data = control.link ? regfile.pc+8 : control.mem_to_reg ? read_data_mem : alu_result;  

    // Write Back
    regfile.access(0, 0, read_data_2, read_data_2, write_reg, control.reg_write, write_data);
    DEBUG(cout << "Mem to reg: " << control.mem_to_reg << " Read data mem: " << read_data_mem << " Alu result: " << alu_result << "\n";)
    DEBUG(cout << "Are we writing: " << control.reg_write << ", writing " << write_data << " to " << write_reg << "\n";)
    
    // Update PC
    regfile.pc += (control.branch && !control.bne && alu_zero) || (control.bne && !alu_zero) ? imm << 2 : 0; 
    regfile.pc = control.jump_reg ? read_data_1 : control.jump ? (regfile.pc & 0xf0000000) & (addr << 2): regfile.pc;
}


void Processor::emptyDXReg(){
    DXReg.reg_dest_control = 0;
    DXReg.jump_control = 0;
    DXReg.jump_reg_control = 0;
    DXReg.link_control = 0;
    DXReg.shift_control = 0;
    DXReg.branch_control = 0;
    DXReg.bne_control = 0;
    DXReg.mem_read_control = 0;
    DXReg.mem_to_reg_control = 0;
    DXReg.ALU_op_control = 0;
    DXReg.mem_write_control = 0;
    DXReg.halfword_control = 0;
    DXReg.byte_control = 0;
    DXReg.ALU_src_control = 0;
    DXReg.reg_write_control = 0;
    DXReg.zero_extend_control = 0;
    // DXReg.pc = 0;
}

void Processor::emptyODRReg(){
    ODRReg.control.reg_dest_control = 0;
    ODRReg.control.jump_control = 0;
    ODRReg.control.jump_reg_control = 0;
    ODRReg.control.link_control = 0;
    ODRReg.control.shift_control = 0;
    ODRReg.control.branch_control = 0;
    ODRReg.control.bne_control = 0;
    ODRReg.control.mem_read_control = 0;
    ODRReg.control.mem_to_reg_control = 0;
    ODRReg.control.ALU_op_control = 0;
    ODRReg.control.mem_write_control = 0;
    ODRReg.control.halfword_control = 0;
    ODRReg.control.byte_control = 0;
    ODRReg.control.ALU_src_control = 0;
    ODRReg.control.reg_write_control = 0;
    ODRReg.control.zero_extend_control = 0;
    ODRReg.instruction = 0;
    // DXReg.pc = 0;
}

void Processor::emptyFDReg(){
    FDReg.instruction = 0;
    FDReg.pc = 0;
}
void Processor::flush(){
    emptyFDReg();
    emptyDXReg();
}

void Processor::stall(){
    emptyDXReg();
    FDRegWrite = 0;
}

void Processor::fetch_stage(){
    uint32_t instruction;

    table[0].push_back(regfile.pc+4);
    
    if (stopFetch2){
        stopFetch2 = false;
        return;
    }
    
    // fetch
    bool successful_access = memory->access(regfile.pc, instruction, 0, 1, 0);
    DEBUG(cout << "Succesful Access?: " << successful_access << "\n";)
    if (!successful_access) {
        FDReg.instruction = 0;
        DEBUG(cout << "Unsucessful Fetch access => stalling\n" ;)
        return;
    }

    if (FDRegWrite == 0){
        return;
    }

    // Branch Prediction
    int ind = hash<uint32_t>{}(regfile.pc) % (BHTSIZE + 1);
    FDReg.pc = regfile.pc + 4;
    if (BHT[ind].prediction > 1){
        regfile.pc = BHT[ind].address;
        FDReg.predict_pc = regfile.pc;
    }
    else {
        // increment pc
        regfile.pc += 4;
        FDReg.predict_pc = regfile.pc;
    }
 
    DEBUG(cout << "inst:" << instruction  << " \n";)
    
    // pass variables
    FDReg.instruction = instruction;
}

void Processor::decode_stage(){
    uint32_t instruction;
    instruction = FDReg.instruction;
    table[1].push_back(FDReg.pc);
    uint32_t temp_pc = FDReg.pc;
    FDRegWrite = 1;
    // decode into contol signals
    if (FDReg.pc != 0 && instruction != 0) { // TODO: Check this - is iffy
        control.decode(instruction);
    }
    else {
        DEBUG(cout << "NOP or failed access. \n";)
        emptyDXReg();
        DXReg.pc = temp_pc;
        return;
    }
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

    int ops[] = {0, 4, 5, 40, 41, 43, 56, 57, 61};
    bool check = false;
    for (int op : ops){
        if (op == opcode){
            check = true;
            break;
        }
    }
    //Stalling - for I type, only check rt for certain cases
    if ((rs == XMReg.write_reg || (rt == XMReg.write_reg && check)) && DXReg.mem_read_control){
        DEBUG(cout << "Stalling - rs: " << rs << " , rt: " << rt << ", XM write reg: " << XMReg.write_reg << " XM Mem Read Control: " << DXReg.mem_read_control << " opcode: " << opcode << "\n";)
        stall();
        DXReg.pc = temp_pc;
        return;
    }
    DEBUG(cout << "Did not stall - rs: " << rs << " , rt: " << rt << ", XM write reg: " << XMReg.write_reg << " XM Mem Read Control: " << " opcode: " << opcode << DXReg.mem_read_control<< "\n";)
    

    // Variables to read data into
    uint32_t read_data_1 = 0;
    uint32_t read_data_2 = 0;

    // Read from reg file
    regfile.access(rs, rt, read_data_1, read_data_2, 0, 0, 0);

    DEBUG(cout << "read_data_1: " << read_data_1 << " read_data_2: " << read_data_2 << "\n";)

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
    DXReg.predict_pc = FDReg.predict_pc;
    DXReg.rd = rd;
    DXReg.rs = rs;
    DXReg.rt = rt;
    DXReg.addr = addr;
    DXReg.shamt = shamt;
    DXReg.opcode = opcode;
    DXReg.funct = funct;

    DEBUG(cout << "[R-Type]" << " opcode: " << opcode <<  ", rs: " << rs << ", rt: " << rt << ", rd: "<< rd << ", shamt: " << shamt << ", funct: " << funct << "\n";)
    DEBUG(cout << "[I-Type]" << " opcode: " << opcode <<  ", rs: " << rs << ", rt: " << rt << ", imm: " << imm << "\n";)
}

void Processor::execute_stage(uint32_t forward_a, uint32_t forward_b, uint32_t prevMWData, bool prevMWWRite){
    alu.generate_control_inputs(DXReg.ALU_op_control, DXReg.funct, DXReg.opcode);
    DEBUG(cout << "ALU op: " << DXReg.ALU_op_control << " Funct: " << DXReg.funct << " opcode: " << DXReg.opcode << "\n";)

    table[2].push_back(DXReg.pc);

    // Find operands for the ALU Execution
    // Operand 1 is always R[rs] -> read_data_1, except sll and srl
    // Operand 2 is immediate if ALU_src = 1, for I-type
    uint32_t operand_1 = DXReg.shift_control ? DXReg.shamt : DXReg.read_data_1;
    uint32_t operand_2 = DXReg.ALU_src_control ? DXReg.imm : DXReg.read_data_2;
    uint32_t alu_zero = 0;



    // if (forward_a != 0) {
    //     operand_1 = forward_a == 2 ? XMReg.alu_result : prevMWData; 
    //     DEBUG(cout << "Forwarding: " << operand_1 << " for operand 1 (forward a: " << forward_a << ")\n";
    // }
    // // Operand 2 is immediate if ALU_src = 1, for I-type, in this case do not forward to rt
    // if (forward_b != 0 && DXReg.ALU_src_control != 1) {
    //     operand_2 = forward_b == 2 ? XMReg.alu_result : prevMWData; 
    //     DEBUG(cout << "Forwarding: " << operand_2 << " for operand 2 (forward b: " << forward_b << ")\n";
    // }

    //FORWARDING
    //TODO: Check that all the cases are correct

    if (DXReg.rs == XMReg.write_reg && XMReg.reg_write_control){
        operand_1 = XMReg.alu_result;
        // If I-type, need to forward to rt if mem read
        if (DXReg.opcode != 0 && XMReg.mem_read_control){
            DXReg.read_data_2 = operand_1;
        }
        DEBUG(cout << "Forwarding: " << operand_1 << " for operand 1 (forward a: " << forward_a << ")\n";)
    }
    else if (forward_a == 1 && prevMWWRite){
        operand_1 = prevMWData; 
        // If I-type, need to forward to rt
        if (DXReg.opcode != 0){
            DXReg.read_data_2 = operand_1;
        }
        DEBUG(cout << "Forwarding: " << operand_1 << " for operand 1 (forward a: " << forward_a << ")\n";)
    }
    else {
        DEBUG(cout << "Did not forward: " << prevMWData << " with control: " << prevMWWRite << " for operand 1 (forward a: " << forward_a << ")\n";)

    }
    
    // Operand 2 is immediate if ALU_src = 1, for I-type, in this case do not forward to rt
    if (DXReg.rt == XMReg.write_reg && XMReg.reg_write_control){
        if (DXReg.ALU_src_control != 0){
            DXReg.read_data_2 = XMReg.alu_result;
        }
        else {
            operand_2 = XMReg.alu_result;
        }
        DEBUG(cout << "Forwarding: " << operand_2 << " for operand 2 (forward b: " << forward_b << ")\n";)
    }
    else if (forward_b == 1 && prevMWWRite){
        if (DXReg.ALU_src_control != 0){
            DXReg.read_data_2 = prevMWData;
        }
        else {
            operand_2 = prevMWData; 
        }
        
        DEBUG(cout << "Forwarding: " << operand_2 << " for operand 2 (forward b: " << forward_b << ")\n";)
    }      
    else {
        DEBUG(cout << "Did not forward: " << prevMWData << " with control: " << prevMWWRite << " for operand 2 (forward b: " << forward_b << ")\n";)
    }  

    

    uint32_t alu_result = alu.execute(operand_1, operand_2, alu_zero);
    DEBUG(cout << "pc: " << DXReg.pc << " op1 " << operand_1 << " op2 "  << operand_2 << " alu_zero " << alu_zero << " alu result " << alu_result << "\n";)

    XMReg.alu_zero = alu_zero;
    XMReg.alu_result = alu_result;
    
    int write_reg = DXReg.link_control ? 31 : DXReg.reg_dest_control ? DXReg.rd : DXReg.rt;  
    XMReg.write_reg = write_reg;

    XMReg.pc_add_result = DXReg.pc + (DXReg.imm << 2);
    XMReg.pc = DXReg.jump_reg_control ? DXReg.read_data_1 : DXReg.jump_control ? (DXReg.pc & 0xf0000000) & (DXReg.addr << 2): DXReg.pc;
    // DEBUG(cout << DXReg.jump_control << " " << DXReg.jump_reg_control << "\n";
    XMReg.orig_pc = DXReg.pc;
    XMReg.predict_pc = DXReg.predict_pc;

    DEBUG(cout << "orig_pc: " << XMReg.orig_pc << " predict_pc " << DXReg.predict_pc << " pc_add_result "  << XMReg.pc_add_result << "\n";)



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

    if ((XMReg.branch_control && !XMReg.bne_control && XMReg.alu_zero) || (XMReg.bne_control && !XMReg.alu_zero)){
        if (XMReg.predict_pc != XMReg.pc_add_result){
            stopFetch2 = true;
        }
    }
    else if (XMReg.branch_control || XMReg.bne_control){
        if (XMReg.predict_pc != XMReg.orig_pc){
            stopFetch2 = true;
        }
    }

}    

void Processor::memory_stage(){
    uint32_t read_data_mem = 0;
    uint32_t write_data_mem = 0;
    table[3].push_back(XMReg.orig_pc);

      // First read no matter whether it is a load or a store
    bool successful_access = memory->access(XMReg.alu_result, read_data_mem, 0, XMReg.mem_read_control | XMReg.mem_write_control, 0);
    DEBUG(cout << "Succesful Access?: " << successful_access << "\n";)
    if (!successful_access) {
        memStall = true;
        DEBUG(cout << "Unsucessful Mem access => stalling\n" ;)
        return;
    }

    DEBUG(cout << "read data mem: " << read_data_mem << " mem read control: " << XMReg.mem_read_control << " Resulting alu result " << XMReg.alu_result << "\n";)
    // Stores: sb or sh mask and preserve original leftmost bits
    write_data_mem = XMReg.halfword_control ? (read_data_mem & 0xffff0000) | (XMReg.read_data_2 & 0xffff) : 
                    XMReg.byte_control ? (read_data_mem & 0xffffff00) | (XMReg.read_data_2 & 0xff): XMReg.read_data_2;

    DEBUG(cout << "read data 2: " << XMReg.read_data_2 << "\n";)
    // Write to memory only if mem_write is 1, i.e store
    successful_access = memory->access(XMReg.alu_result, read_data_mem, write_data_mem, XMReg.mem_read_control, XMReg.mem_write_control);
    DEBUG(cout << "write data mem: " << write_data_mem << " mem write control: " << XMReg.mem_write_control << " Resulting alu result " << XMReg.alu_result << "\n";)
    // if (!successful_access) {
    //     memStall = true;
    //     DEBUG(cout << "Unsucessful Mem access => stalling\n" ;)
    //     return;
    // }
    // Loads: lbu or lhu modify read data by masking
    MWBReg.read_data_mem &= XMReg.halfword_control ? 0xffff : XMReg.byte_control ? 0xff : 0xffffffff;

    //Branch
    if ((XMReg.branch_control && !XMReg.bne_control && XMReg.alu_zero) || (XMReg.bne_control && !XMReg.alu_zero)){
        // DEBUG(cout << "Branch taken => Flushing \n";)
        DEBUG(cout << "Branch taken \n";)
        int ind = hash<uint32_t>{}(XMReg.orig_pc-4) % (BHTSIZE + 1);
        BHT[ind].prediction += 1;
        if (BHT[ind].prediction > 3) { BHT[ind].prediction = 3; }
        // Predicted taken
        if (XMReg.predict_pc != XMReg.orig_pc){
            if (XMReg.predict_pc != XMReg.pc_add_result){
                regfile.pc = XMReg.pc_add_result;   
                DEBUG(cout << "Prediction was: " << XMReg.predict_pc << " Actual was: XMReg.pc_add_result " << "Address prediction wrong => Flushing \n";)
                flush();
            }
            else {
                DEBUG(cout << "Prediction correct - branch taken \n";)
            }
        }
        // Predicted not taken
        else {
            regfile.pc = XMReg.pc_add_result;
            DEBUG(cout << "Prediction wrong => Flushing \n";)
            flush();
        }
        BHT[ind].address = XMReg.pc_add_result;
    }
    //Branch not taken
    else if (XMReg.branch_control || XMReg.bne_control){
        int ind = hash<uint32_t>{}(XMReg.orig_pc-4) % (BHTSIZE + 1);
        BHT[ind].prediction -= 1;
        if (BHT[ind].prediction < 0) { BHT[ind].prediction = 0; }
        // Predicted taken
        if (XMReg.predict_pc != XMReg.orig_pc){
            regfile.pc = XMReg.orig_pc; 
            DEBUG(cout << "Branch not taken - prediction wrong => Flushing \n";)  
            flush();
        }
        // Predicted not taken
        else {
            DEBUG(cout << "Prediction correct \n";)
        }
    }
    //Jump
    else{
        if (XMReg.pc != XMReg.orig_pc){
            regfile.pc = XMReg.pc;
            flush();
        }
    }
    DEBUG(cout << "Orig pc:" << XMReg.orig_pc << " jump pc: " << XMReg.pc << " add pc: " << XMReg.pc_add_result << "\n";)

    // Passing Values
    MWBReg.pc = XMReg.orig_pc;
    MWBReg.write_reg = XMReg.write_reg;
    MWBReg.alu_result = XMReg.alu_result;

    MWBReg.link_control = XMReg.link_control;
    MWBReg.mem_to_reg_control = XMReg.mem_to_reg_control;
    MWBReg.reg_write_control = XMReg.reg_write_control;
    MWBReg.read_data_mem = read_data_mem;

}

void Processor::write_back_stage() {
    table[4].push_back(MWBReg.pc);
    uint32_t read_data_dummy;
    uint32_t write_data = MWBReg.link_control ? regfile.pc+8 : MWBReg.mem_to_reg_control ? MWBReg.read_data_mem : MWBReg.alu_result; 
    DEBUG(cout << "Mem to reg: " << MWBReg.mem_to_reg_control << " Read data mem: " << MWBReg.read_data_mem << " Alu result: " << MWBReg.alu_result << "\n";)
    DEBUG(cout << "Are we writing: " << MWBReg.reg_write_control << ", writing " << write_data << " to " << MWBReg.write_reg << "\n";)
    regfile.access(0, 0, read_data_dummy, read_data_dummy, MWBReg.write_reg, MWBReg.reg_write_control, write_data);
    if (MWBReg.pc > 3){
        finishedPC = MWBReg.pc - 4;
    }
    
}

void Processor::pipelined_processor_advance() { 
    // pipelined processor logic goes here
    // does nothing currently -- if you call it from the cmd line, you'll run into an infinite loop
    // might be helpful to implement stages in a separate module

    //FORWARDING LOGIC
    uint32_t forward_a;
    uint32_t forward_b;
    uint32_t tempMWData;
    bool MWWrite;
    forward_a = DXReg.rs == MWBReg.write_reg ? 1 : 0;
    forward_b = DXReg.rt == MWBReg.write_reg ? 1 : 0;
    tempMWData = MWBReg.link_control ? regfile.pc+8 : MWBReg.mem_to_reg_control ? MWBReg.read_data_mem : MWBReg.alu_result; 
    MWWrite = MWBReg.reg_write_control;

        
    DEBUG(cout << "\n\n";)
    DEBUG(cout << "==WRITEBACK==" << "\n";)        
    write_back_stage();
    DEBUG(cout << "==MEMORY==" << "\n";)
    memory_stage();
    if (memStall){
        memStall = false;
        table[2].push_back(DXReg.pc);
        table[1].push_back(DXReg.pc);
        table[0].push_back(regfile.pc);
        return;
    }
    DEBUG(cout << "==EXECUTE=="<< "\n";)
    execute_stage(forward_a, forward_b, tempMWData, MWWrite);
    DEBUG(cout << "==DECODE==" << "\n";)
    decode_stage();
    DEBUG(cout << "==FETCH=="<< "\n";)
    fetch_stage();


    string stage_strings[5] = {"F", "D", "X", "M", "W"};
    vector<int> lens = {};
    for (unsigned int i = 0; i < table.size(); i++)
    {
        DEBUG(cout << stage_strings[i] << ": ";)
        for (unsigned int j = 0; j < table[i].size(); j++)
        {
            int len = to_string(abs(table[i][j])).length();
            if (i == 0){
                lens.push_back(len);
            }
            string space(lens[j]-len + 1, ' ');
            DEBUG(cout << table[i][j] << space;)
        }
        DEBUG(cout << "\n";)
    }
    lens.clear();
}

void Processor::OOOfetch() {

    uint32_t instruction;
    table2[0].push_back(regfile.pc+4);
    if (stopFetch2){
        stopFetch2 = false;
        OFDReg.fetched = false;
        return;
    }
    
    // fetch
    bool successful_access = memory->access(regfile.pc, instruction, 0, 1, 0);
    DEBUG(cout << "Succesful Access?: " << successful_access << "\n";)
    if (!successful_access) {
        OFDReg.instruction = 0;
        DEBUG(cout << "Unsucessful Fetch access => stalling\n" ;)
        OFDReg.fetched = false;
        return;
    }

    if (FDRegWrite == 0){
        OFDReg.fetched = false;
        return;
    }

    // Branch Prediction
    int ind = hash<uint32_t>{}(regfile.pc) % (BHTSIZE + 1);
    OFDReg.pc = regfile.pc + 4;
    if (BHT[ind].prediction > 1){
        regfile.pc = BHT[ind].address;
        OFDReg.predict_pc = regfile.pc;
    }
    else {
        // increment pc
        regfile.pc += 4;
        OFDReg.predict_pc = regfile.pc;
    }
 
    DEBUG(cout << "inst:" << instruction  << " \n";)
    
    // pass variables
    OFDReg.instruction = instruction;
    OFDReg.fetched = true;

}
void Processor::OOOdecode() {
    uint32_t instruction;
    instruction = OFDReg.instruction;
    DEBUG(cout << "Decode inst:" << OFDReg.instruction  << " \n";)

    uint32_t temp_pc = OFDReg.pc;
    table2[1].push_back(OFDReg.pc);
    FDRegWrite = 1;
    // decode into contol signals
    if (OFDReg.fetched) { // TODO: Check this - is iffy
        control.decode(instruction);
    }
    else {
        DEBUG(cout << "NOP or failed access. \n";)
        emptyODRReg();
        // DXReg.pc = temp_pc;
        ODRReg.pc = 0;
        return;
    }
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
    // regfile.access(rs, rt, read_data_1, read_data_2, 0, 0, 0);

    // DEBUG(cout << "read_data_1: " << read_data_1 << " read_data_2: " << read_data_2 << "\n";)

    // Sign Extend Or Zero Extend the immediate
    // Using Arithmetic right shift in order to replicate 1 
    imm = control.zero_extend ? imm : (imm >> 15) ? 0xffff0000 | imm : imm;

    // Pass Variables
    // Control
    ControlSignals new_control;
    new_control.reg_dest_control = control.reg_dest;
    new_control.jump_control = control.jump;
    new_control.jump_reg_control = control.jump_reg;
    new_control.link_control = control.link;
    new_control.shift_control = control.shift;
    new_control.branch_control = control.branch;
    new_control.bne_control = control.bne;
    new_control.mem_read_control = control.mem_read;
    new_control.mem_to_reg_control = control.mem_to_reg;
    new_control.ALU_op_control = control.ALU_op;
    new_control.mem_write_control = control.mem_write;
    new_control.halfword_control = control.halfword;
    new_control.byte_control = control.byte;
    new_control.ALU_src_control = control.ALU_src;
    new_control.reg_write_control = control.reg_write;
    new_control.zero_extend_control = control.zero_extend;
    ODRReg.control = new_control;

    // Instructions
    ODRReg.imm = imm;
    ODRReg.read_data_1 = read_data_1;
    ODRReg.read_data_2 = read_data_2;
    ODRReg.pc = OFDReg.pc;
    ODRReg.predict_pc = OFDReg.predict_pc;
    ODRReg.rd = rd;
    ODRReg.rs = rs;
    ODRReg.rt = rt;
    ODRReg.addr = addr;
    ODRReg.shamt = shamt;
    ODRReg.opcode = opcode;
    ODRReg.funct = funct;
    ODRReg.instruction = instruction;


    DEBUG(cout << "[R-Type]" << " opcode: " << opcode <<  ", rs: " << rs << ", rt: " << rt << ", rd: "<< rd << ", shamt: " << shamt << ", funct: " << funct << "\n";)
    DEBUG(cout << "[I-Type]" << " opcode: " << opcode <<  ", rs: " << rs << ", rt: " << rt << ", imm: " << imm << "\n";)

}
int Processor::mapReg(int reg){
    int newReg = physRegFile.firstReady();
    physRegFile.setReady(newReg, false);
    regMap[reg] = newReg;
    cout << "Mapped " << reg << " to " << newReg << "\n";
    return newReg;
}
void Processor::OOOrename() {
    table2[2].push_back(ODRReg.pc);
    DEBUG(cout << "Rename inst:" << ODRReg.instruction  << " \n";)
    if (ODRReg.pc == 0){
        return;
    }
    // if (ODRReg.instruction == 0) { // Maybe not needed
    //     ReorderBufferEntry rob = ReorderBufferEntry(sequence);
    //     QueueEntry iqe = QueueEntry(sequence, ODRReg, vector<int>());
    //     ReorderBuffer.push_back(rob);
    //     InstructionQueue.push_back(iqe);
    //     sequence += 1;
    // }
    // else {
        int oldDest = ODRReg.rd;
        int temp;
        vector<int> regs;
        if (ODRReg.opcode == 0){ //R type
            ODRReg.rd = mapReg(ODRReg.rd); // TO DO: Need to set old register to ready if not -1 in map
            temp = regMap[ODRReg.rt];
            if (temp != -1){
                regs.push_back(temp);
            }
            else {
                ODRReg.rt = mapReg(ODRReg.rt);
            }
        }
        else { // Rest of I type
            oldDest = ODRReg.rt;
            if (ODRReg.opcode != 40 && ODRReg.opcode != 56 && ODRReg.opcode != 41
                 && ODRReg.opcode != 43 && ODRReg.opcode != 57 && ODRReg.opcode != 61){ // Add branches
                ODRReg.rt = mapReg(ODRReg.rt);
            }
            else { 
                temp = regMap[ODRReg.rt]; 
                if (temp != -1){
                    regs.push_back(temp);
                }
                else {
                    ODRReg.rt = mapReg(ODRReg.rt);
                }
            }
            
        }
        // ODRReg.rs = regMap[ODRReg.rs];
        temp = regMap[ODRReg.rs];
        if (temp != -1){
            regs.push_back(temp);
            cout << "Pushed back " << temp << "\n";
        }
        else {
            ODRReg.rs = mapReg(ODRReg.rs);
        }
        ODRReg.oldDest = oldDest;
        ReorderBufferEntry rob = ReorderBufferEntry(sequence);
        if (ODRReg.opcode != 35 && ODRReg.opcode != 43){ // Accept everything except load and store
            QueueEntry iqe = QueueEntry(sequence, ODRReg, regs);
            InstructionQueue.push_back(iqe);
            cout << "Instruction pushed to InstQueue with dependencies: ";
        }
        else {
            QueueEntry lse = QueueEntry(sequence, ODRReg, regs);
            LoadStoreQueue.push_back(lse);
            cout << "Instruction pushed to LoadStoreQueue with dependencies: ";
        }
        ReorderBuffer.push_back(rob);
        for (int i = 0; i < regs.size(); i++){
            cout << regs[i] << " ";
        }
        cout << "\n";
    // }
    cout << "Removing dependency: " << OEWReg.write_reg << "\n"; //Fix issue with write_reg being 0 by default
    sequence += 1;

    for (QueueEntry& entry : InstructionQueue){
        cout << "DEPEND SIZE: " << entry.regDependencies.size() << "\n";
        for (int i = 0; i < entry.regDependencies.size(); i++){
            if (entry.regDependencies[i] == OEWReg.write_reg){
                entry.regDependencies.erase(entry.regDependencies.begin()+i);
                cout << "Removed dependency: " << OEWReg.write_reg << " from " << entry.controls.pc << "\n";
            }
            i--;
        }
    }
    for (QueueEntry& entry : LoadStoreQueue){
        for (int i = 0; i < entry.regDependencies.size(); i++){
            if (entry.regDependencies[i] == OEWReg.write_reg){
                entry.regDependencies.erase(entry.regDependencies.begin()+i);
                cout << "Removed dependency: " << OEWReg.write_reg << " from " << entry.controls.pc << "\n";
            }
            i--;
        }
    }

}
void Processor::OOOexecute() {
    bool exe = false;
    bool memExe = false;
    QueueEntry intr;
    if (LoadStoreQueue.size() > 0) {
        cout << "Front of lsq has dependencies: ";
        for (int i = 0; i < LoadStoreQueue.front().regDependencies.size(); i++){
            cout << LoadStoreQueue.front().regDependencies[i] << " ";
        }
        cout << "\n";
        if (LoadStoreQueue.front().regDependencies.empty()) {
        
            intr = LoadStoreQueue.front();
            memExe = true;
            exe = true;
        }
    }
    else {
        cout << "Instruction queue has dependencies: ";
        for (int i = 0; i < InstructionQueue.size(); i++){
            cout << "Instruction with pc: " << InstructionQueue[i].controls.pc << " ";
            for (int j = 0; j < InstructionQueue[i].regDependencies.size(); j++){
                cout << InstructionQueue[i].regDependencies[j] << " ";
            }
            cout << "\n";
            if (InstructionQueue[i].regDependencies.empty()){
                intr = InstructionQueue[i];
                InstructionQueue.erase(InstructionQueue.begin() + i);
                exe = true;
                break;
            }
        }
        cout << "\n";
    }
    cout << "found instruction: " << exe << "\n";
    if (!exe){
        table2[3].push_back(0);
        OEWReg.changed = false;
        return;
    }
    table2[3].push_back(intr.controls.pc);

    physRegFile.access(intr.controls.rs, intr.controls.rt, intr.controls.read_data_1, intr.controls.read_data_2, 0, 0, 0);
    DEBUG(cout << "read_data_1: " << intr.controls.read_data_1 << " read_data_2: " << intr.controls.read_data_2 << "\n";)


    alu.generate_control_inputs(intr.controls.control.ALU_op_control, intr.controls.funct, intr.controls.opcode);
    // DEBUG(cout << "ALU op: " << DXReg.ALU_op_control << " Funct: " << DXReg.funct << " opcode: " << DXReg.opcode << "\n";)


    // Find operands for the ALU Execution
    // Operand 1 is always R[rs] -> read_data_1, except sll and srl
    // Operand 2 is immediate if ALU_src = 1, for I-type
    uint32_t operand_1 = intr.controls.control.shift_control ? intr.controls.shamt : intr.controls.read_data_1;
    uint32_t operand_2 = intr.controls.control.ALU_src_control ? intr.controls.imm : intr.controls.read_data_2;
    uint32_t alu_zero = 0;
    

    uint32_t alu_result = alu.execute(operand_1, operand_2, alu_zero);
    // DEBUG(cout << "pc: " << DXReg.pc << " op1 " << operand_1 << " op2 "  << operand_2 << " alu_zero " << alu_zero << " alu result " << alu_result << "\n";)
    DEBUG(cout << " op1 " << operand_1 << " op2 "  << operand_2 << " alu_zero " << alu_zero << " alu result " << alu_result << "\n";)

    
    int write_reg = intr.controls.control.link_control ? 31 : intr.controls.control.reg_dest_control ? intr.controls.rd : intr.controls.rt;  

    uint32_t pc_add_result = intr.controls.pc + (intr.controls.imm << 2);
    uint32_t pc = intr.controls.control.jump_reg_control ? intr.controls.read_data_1 : intr.controls.control.jump_control ? (intr.controls.pc & 0xf0000000) & (intr.controls.addr << 2): intr.controls.pc;
    // DEBUG(cout << DXReg.jump_control << " " << DXReg.jump_reg_control << "\n";
    uint32_t orig_pc = intr.controls.pc;
    uint32_t predict_pc = intr.controls.predict_pc;

    // DEBUG(cout << "orig_pc: " << XMReg.orig_pc << " predict_pc " << DXReg.predict_pc << " pc_add_result "  << XMReg.pc_add_result << "\n";)
    uint32_t read_data_mem = 0;
    uint32_t write_data_mem = 0;
    // table[3].push_back(XMReg.orig_pc);

    

    if (intr.controls.instruction == 0){ // for nops - could be not necessary as well
        OEWReg.pc = orig_pc;
        OEWReg.sequence = intr.sequenceNum;
        OEWReg.changed = true;
        return;
    }
      // First read no matter whether it is a load or a store
    bool successful_access = memory->access(alu_result, read_data_mem, 0, intr.controls.control.mem_read_control | intr.controls.control.mem_write_control, 0);
    DEBUG(cout << "Succesful Access?: " << successful_access << "\n";)
    if (!successful_access) {
        OOOmemStall = true;
        DEBUG(cout << "Unsucessful Mem access => stalling\n" ;)
        OEWReg.changed = false;
        return;
    }
    if (memExe) { // When non blocking cache is implemented - make sure to not pull from lsq while waiting for cache
        LoadStoreQueue.erase(LoadStoreQueue.begin());
        OOOmemStall = false;
    }

    DEBUG(cout << "read data mem: " << read_data_mem << " mem read control: " << intr.controls.control.mem_read_control << " Resulting alu result " << alu_result << "\n";)
    // Stores: sb or sh mask and preserve original leftmost bits
    write_data_mem = intr.controls.control.halfword_control ? (read_data_mem & 0xffff0000) | (intr.controls.read_data_2 & 0xffff) : 
                    intr.controls.control.byte_control ? (read_data_mem & 0xffffff00) | (intr.controls.read_data_2 & 0xff): intr.controls.read_data_2;

    // DEBUG(cout << "read data 2: " << XMReg.read_data_2 << "\n";)
    // Write to memory only if mem_write is 1, i.e store
    successful_access = memory->access(alu_result, read_data_mem, write_data_mem, intr.controls.control.mem_read_control, intr.controls.control.mem_write_control);
    DEBUG(cout << "write data mem: " << write_data_mem << " mem write control: " << intr.controls.control.mem_write_control << " Resulting alu result " << alu_result << "\n";)
    // if (!successful_access) {
    //     memStall = true;
    //     DEBUG(cout << "Unsucessful Mem access => stalling\n" ;)
    //     return;
    // }
    // Loads: lbu or lhu modify read data by masking
    MWBReg.read_data_mem &= intr.controls.control.halfword_control ? 0xffff : intr.controls.control.byte_control ? 0xff : 0xffffffff;

    //Branch
    if ((intr.controls.control.branch_control && !intr.controls.control.bne_control && alu_zero) || (intr.controls.control.bne_control && !alu_zero)){
        // DEBUG(cout << "Branch taken => Flushing \n";)
        DEBUG(cout << "Branch taken \n";)
        int ind = hash<uint32_t>{}(XMReg.orig_pc-4) % (BHTSIZE + 1);
        BHT[ind].prediction += 1;
        if (BHT[ind].prediction > 3) { BHT[ind].prediction = 3; }
        // Predicted taken
        if (predict_pc != orig_pc){
            if (predict_pc != pc_add_result){
                regfile.pc = pc_add_result;   
                DEBUG(cout << "Prediction was: " << XMReg.predict_pc << " Actual was: " << XMReg.pc_add_result  << " Address prediction wrong => Flushing \n";)
                squash(intr.sequenceNum);
            }
            else {
                DEBUG(cout << "Prediction correct - branch taken \n";)
            }
        }
        // Predicted not taken
        else {
            regfile.pc = pc_add_result;
            DEBUG(cout << "Prediction wrong => Flushing \n";)
            squash(intr.sequenceNum);
        }
        BHT[ind].address = pc_add_result;
    }
    //Branch not taken
    else if (intr.controls.control.branch_control || intr.controls.control.bne_control){
        int ind = hash<uint32_t>{}(orig_pc-4) % (BHTSIZE + 1);
        BHT[ind].prediction -= 1;
        if (BHT[ind].prediction < 0) { BHT[ind].prediction = 0; }
        // Predicted taken
        if (predict_pc != orig_pc){
            regfile.pc = orig_pc; 
            DEBUG(cout << "Branch not taken - prediction wrong => Flushing \n";)  
            squash(intr.sequenceNum);
        }
        // Predicted not taken
        else {
            DEBUG(cout << "Prediction correct \n";)
        }
    }
    //Jump
    else{
        if (pc != orig_pc){
            regfile.pc = pc;
            squash(intr.sequenceNum);
        }
    }
    DEBUG(cout << "Orig pc:" << orig_pc << " jump pc: " << pc << " add pc: " << pc_add_result << "\n";)

    

    OEWReg.pc = orig_pc;
    OEWReg.write_reg = write_reg;
    OEWReg.alu_result = alu_result;

    OEWReg.link_control = intr.controls.control.link_control;
    OEWReg.mem_to_reg_control = intr.controls.control.mem_to_reg_control;
    OEWReg.reg_write_control = intr.controls.control.reg_write_control;
    OEWReg.read_data_mem = read_data_mem;
    OEWReg.arch_write_reg = intr.controls.oldDest;
    OEWReg.sequence = intr.sequenceNum;
    OEWReg.changed = true;

}
void Processor::OOOwriteback() {
    table2[4].push_back(OEWReg.pc);
    uint32_t read_data_dummy;
    uint32_t write_data = MWBReg.link_control ? regfile.pc+8 : OEWReg.mem_to_reg_control ? OEWReg.read_data_mem : OEWReg.alu_result; 
    DEBUG(cout << "Mem to reg: " << OEWReg.mem_to_reg_control << " Read data mem: " << OEWReg.read_data_mem << " Alu result: " << OEWReg.alu_result << "\n";)
    DEBUG(cout << "Are we writing: " << OEWReg.reg_write_control << ", writing " << write_data << " to " << OEWReg.write_reg << "\n";)
    physRegFile.access(0, 0, read_data_dummy, read_data_dummy, OEWReg.write_reg, OEWReg.reg_write_control, write_data);
    OWCReg.arch_write_reg = OEWReg.arch_write_reg;
    OWCReg.reg_write_control = OEWReg.reg_write_control;
    OWCReg.write_data = write_data;
    OWCReg.write_reg = OEWReg.write_reg;
    OWCReg.sequence = OEWReg.sequence;
    OWCReg.pc = OEWReg.pc;
    if (OEWReg.changed){
        commitReady.push_back(OWCReg);

    }
}

void Processor::OOOcommit() {
    table2[5].push_back(OWCReg.pc);
    cout << "List of ready instruction pcs: ";
    for (int i = 0; i < commitReady.size(); i++){
        cout << "(" << commitReady[i].pc << ", " << commitReady[i].sequence << ")";
    }
    cout << "\n";
    uint32_t read_data_dummy;
    if (ReorderBuffer.size() == 0){
        return;
    }
    int ind = 0;
    for (WritebackCommitReg reg : commitReady){
        if (reg.sequence == ReorderBuffer.front().sequenceNum){
            cout << "Committing sequence number: " << reg.sequence << " with pc: " << reg.pc << "\n";
            ReorderBuffer.erase(ReorderBuffer.begin());
            commitReady.erase(commitReady.begin() + ind);
            if (reg.pc > 3){
                finishedPC = reg.pc - 4;
            }
            DEBUG(cout << "Are we writing: " << reg.reg_write_control << ", writing " << reg.write_data << " to " << reg.arch_write_reg << "\n";)
            regfile.access(0, 0, read_data_dummy, read_data_dummy, reg.arch_write_reg, reg.reg_write_control, reg.write_data);
            DEBUG(cout << "Released register: " << reg.write_reg << "\n";)
            physRegFile.setReady(reg.write_reg, true);
            break;
        }
        ind += 1;
    }
    
    
    
    
}   

void Processor::squash(int sequenceNum) {
    cout << "Squashing all instructions over: " << sequenceNum << "\n";
    vector<ReorderBufferEntry>::iterator it = ReorderBuffer.begin();

    while(it != ReorderBuffer.end()) {
        if(it->sequenceNum > sequenceNum) {
            it = ReorderBuffer.erase(it);
        }
        else ++it;
    }
    vector<QueueEntry>::iterator it2 = LoadStoreQueue.begin();

    while(it2 != LoadStoreQueue.end()) {
        if(it2->sequenceNum > sequenceNum) {
            it2 = LoadStoreQueue.erase(it2);
        }
        else ++it2;
    }
    vector<QueueEntry>::iterator it3 = InstructionQueue.begin();

    while(it3 != InstructionQueue.end()) {
        if(it3->sequenceNum > sequenceNum) {
            it3 = InstructionQueue.erase(it3);
        }
        else ++it3;
    }

    vector<WritebackCommitReg>::iterator it4 = commitReady.begin();

    while(it4 != commitReady.end()) {
        if(it4->sequence > sequenceNum) {
            it4 = commitReady.erase(it4);
        }
        else ++it4;
    }
}

void Processor::out_of_order_advance() { 
    DEBUG(cout << "==COMMIT==" <<  std::endl;)
    OOOcommit();
    DEBUG(cout << "==WRITEBACK==" <<  std::endl;)
    OOOwriteback();
    DEBUG(cout << "==EXECUTE==" <<  std::endl;)
    OOOexecute();
    DEBUG(cout << "==RENAME==" <<  std::endl;)
    OOOrename();
    DEBUG(cout << "==DECODE==" <<  std::endl;)
    OOOdecode();
    DEBUG(cout << "==FETCH==" <<  std::endl;)
    OOOfetch();

    // cout << "Not ready registers: ";
    // for (int i = 0; i < physRegFile.getSize(); i++){
    //     if (!physRegFile.ready(i)){
    //         cout << i << " ";
    //     }
    // }
    // cout << "\n";
    // for (int i = -1; i < regfile.getSize(); i++){
    //     cout << i << ":" << regMap[i] << "\n";
    // }
    
    // OOOissue();
    // OOOdispatch();
    
    // string stage_strings[6] = {"F", "D", "R", "X", "W", "C"};
    // vector<int> lens = {};
    // for (unsigned int i = 0; i < table2.size(); i++)
    // {
    //     DEBUG(cout << stage_strings[i] << ": ";)
    //     for (unsigned int j = 0; j < table2[i].size(); j++)
    //     {
    //         int len = to_string(abs(table2[i][j])).length();
    //         if (i == 0){
    //             lens.push_back(len);
    //         }
    //         string space(lens[j]-len + 1, ' ');
    //         DEBUG(cout << table2[i][j] << space;)
    //     }
    //     DEBUG(cout << "\n";)
    // }
    // lens.clear();
    
    

}