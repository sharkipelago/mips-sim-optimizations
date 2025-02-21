#ifndef HDU_FILE
#define HDU_FILE
#include <vector>
#include <cstdint>
#include <iostream>
#include "pipeline.h"

class HDU {

  public:
    bool fetch_stall = false;
    bool decode_stall = false;
    bool execute_stall = false;
    bool memory_stall = false;
    bool writeback_stall = false;

    HDU() {
    }
    // Hazard Detection Unit stuff

    //Stall Memory and everything before it e.g. cache miss
    void stall_memory() {
			fetch_stall = true;
			decode_stall = true;
			execute_stall = true;
			memory_stall = true;
    }

    void reset_stalls(){
    	fetch_stall = false;
    	decode_stall = false;
    	execute_stall = false;
    	memory_stall = false;
    	writeback_stall = false;
    }
};
#endif