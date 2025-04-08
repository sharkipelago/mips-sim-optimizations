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
    bool dead = false;
    ReorderBufferEntry(int sequence){
        sequenceNum = sequence;
    }
};
struct QueueEntry {
    int sequenceNum;
    DecodeRenameReg controls;
    vector<int> regDependencies;
    QueueEntry(){}
    QueueEntry(int sequence, DecodeRenameReg control, vector<int> regs){
        sequenceNum = sequence;
        controls = control;
        regDependencies = regs;
    }
};

struct InstructionQueueEntry {
    int sequenceNum;
    DecodeRenameReg controls;
    vector<int> regDependencies;
    InstructionQueueEntry(int sequence, DecodeRenameReg control, vector<int> regs){
        sequenceNum = sequence;
        controls = control;
        regDependencies = regs;
    }
};

struct LoadStoreEntry {
    int sequenceNum;
    DecodeRenameReg controls;
    vector<int> regDependencies;
    uint32_t physicalAddress;
    LoadStoreEntry(int sequence, DecodeRenameReg control, vector<int> regs){
        sequenceNum = sequence;
        controls = control;
        regDependencies = regs; 
    }
};


// class ExecutionPort {
//     public: 
//     ALU alu;
//     Memory* memory;
//     vector<BHTLine>* BHT;
//     ExecutionPort(ALU nalu, Memory *mem, vector<BHTLine>* branchHist) { 
//         alu = nalu;
//         memory = mem;
//         BHT = branchHist;
//     }
//     void executeIntr(InstructionQueueEntry intr);
//     void executeMem(LoadStoreEntry lse);
// };

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
        vector<vector<int>> table2;
        vector<ReorderBufferEntry> ReorderBuffer;
        vector<QueueEntry> InstructionQueue;
        vector<QueueEntry> LoadStoreQueue;
        // vector<ExecutionPort> executes;

        uint32_t finishedPC = 0;
        bool FDRegWrite = 1, memStall = false, stopFetch2 = false;

        bool OOOmemStall = false;

        vector<BHTLine> BHT;

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
        vector<WritebackCommitReg> commitReady;
        

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
            // executes.push_back(ExecutionPort(alu, mem, &BHT));
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

        int mapReg(int reg);
        void emptyODRReg();
        void OOOfetch();
        void OOOdecode();
        void OOOrename();
        void OOOissue();
        void OOOdispatch();
        void OOOexecute();
        void OOOwriteback();
        void OOOcommit();
        void squash(int num);

};
