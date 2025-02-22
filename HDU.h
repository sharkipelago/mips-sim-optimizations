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

		bool flushing = false;
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
		
		void start_flush() {
			flushing = true;

			decode_stall = true;
			execute_stall = true;
			execute_stall = true;
			memory_stall = true;
		}

		void update() {
			if (flushing) {
				if (fetch_stall) {
					fetch_stall = false;
				}
				else if (decode_stall) {
					decode_stall = false;
				}
				else if (execute_stall){
					execute_stall = false;
				}
				else if (memory_stall) {
					memory_stall = false;
				}
				else if (writeback_stall) {
					writeback_stall = false;
				}
			}
			else {
				reset_stalls();
			}
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