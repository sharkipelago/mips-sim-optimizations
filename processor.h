#include "memory.h"
#include "regfile.h"
#include "ALU.h"
#include "control.h"
#include "pipeline.h"

#define BHTSIZE 64


struct BHTLine {
    int prediction;
    uint32_t address;
};

class Processor {
    private:
        int opt_level;
        ALU alu;
        control_t control;
        Memory *memory;
        Registers regfile;
        vector<vector<int>> table;
        
        uint32_t finishedPC;
        bool FDRegWrite = 1, memStall = false, stopFetch2 = false;

        std::vector<BHTLine> BHT;

        // add other structures as needed
        FetchDecodePipeReg FDReg;
        DecodeExPipeReg DXReg;
        ExMemPipeReg XMReg;
        MemWBPipeReg MWBReg;
        // pipelined processor

        // add private functions
        void single_cycle_processor_advance();
        void pipelined_processor_advance();
 
    public:

        Processor(Memory *mem) { 
            regfile.pc = 0; 
            memory = mem;
            BHT.resize(BHTSIZE);
        }

        // Get PC
        uint32_t getPC();

        // Prints the Register File
        void printRegFile() { regfile.print(); }
        
        // Initializes the processor appropriately based on the optimization level
        void initialize(int opt_level);

        // Advances the processor to an appropriate state every cycle
        void advance(); 

        void emptyFDReg();
        void emptyDXReg();
        void stall();
        void fetch_stage();
        void decode_stage();
        void execute_stage(uint32_t, uint32_t, uint32_t, bool);
        void memory_stage();
        void write_back_stage();
        void flush();

};
