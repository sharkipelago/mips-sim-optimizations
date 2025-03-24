#include "memory.h"
#include "regfile.h"
#include "ALU.h"
#include "control.h"
#include "pipeline.h"
#include "OOORegs.h"
#include <map>
#include <queue>

#define BHTSIZE 64


struct BHTLine {
    int prediction;
    uint32_t address;
};

struct ReorderBufferEntry {
    int sequenceNum;
};

struct InstructionQueueEntry {
    int sequenceNum;
    ControlSignals controls;
    vector<int> regDependencies;
};

struct LoadStoreEntry {
    int sequenceNum;
    ControlSignals controls;
    vector<int> regDependencies;
    uint32_t physicalAddress;
};


class Processor {
    private:
        int opt_level;
        ALU alu;
        control_t control;
        Memory *memory;
        Registers regfile;
        Registers physRegFile = Registers(96);
        map<int, int> regMap;
        vector<vector<int>> table;
        queue<ReorderBufferEntry> ReorderBuffer;
        vector<InstructionQueueEntry> InstructionQueue;
        queue<LoadStoreEntry> LoadStoreQueue;

        uint32_t finishedPC;
        bool FDRegWrite = 1, memStall = false, stopFetch2 = false;

        std::vector<BHTLine> BHT;

        int sequence = 0;

        // add other structures as needed
        // pipelined processor
        FetchDecodePipeReg FDReg;
        DecodeExPipeReg DXReg;
        ExMemPipeReg XMReg;
        MemWBPipeReg MWBReg;

        //OOO processor
        FetchDecodeReg OFDReg;
        DecodeRenameReg ODRReg;
        RenameIssueReg ORIReg;
        IssueDispatchReg OIDReg;
        DispatchExecuteReg ODEReg;
        ExecuteWritebackReg OEWReg;
        WritebackCommitReg OWCReg;
        

        // add private functions
        void single_cycle_processor_advance();
        void pipelined_processor_advance();
        void out_of_order_advance();
 
    public:

        Processor(Memory *mem) { 
            regfile.pc = 0; 
            memory = mem;
            BHT.resize(BHTSIZE);
            for (int i = 0; i < regfile.getSize(); i++) {
                regMap[i] = -1;
            }
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

        void OOOfetch();
        void OOOdecode();
        void OOOrename();
        void OOOissue();
        void OOOdispatch();
        void OOOexecute();
        void OOOwriteback();
        void OOOcommit();
        void squash();

};
