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

			fetch_stall = true;
			decode_stall = true;
			execute_stall = true;
			memory_stall = true;
			writeback_stall = true;
		}

		void update() {
			if (flushing) {
				cout << "\nFLUSHING";
				if (fetch_stall) {
					cout << "(fetch is free)\n";
					fetch_stall = false;
				}
				else if (decode_stall) {
					cout << "(decode is free)\n";
					decode_stall = false;
				}
				else if (execute_stall){
					cout << "(execute is free)\n";
					execute_stall = false;
				}
				else if (memory_stall) {
					cout << "(memory is free)\n";
					memory_stall = false;
				}
				else if (writeback_stall) {
					cout << "(writeback is free)\n";
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