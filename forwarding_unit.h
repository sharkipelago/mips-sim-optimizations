#ifndef FORWARD_FILE
#define FORWARD_FILE
#include <vector>
#include <cstdint>
#include <iostream>
#include "pipeline.h"

class ForwardingUnit {

  public:
      ForwardingUnit() {
      }
      // ForwardingUnit stuff
      void check(int IDEX_rs, int IDEX_rt, int EXMEM_rd, int MEMWB_rd, uint8_t &forwardA, uint8_t &forwardB) 
      {
            //Check Execute first, because in case where both EXEMEM_rd and MEMWB_rd ==  IDEX_rs then execute should take precedence
            //as it would be the final thing to be written to regfile

            forwardA = IDEX_rs == EXMEM_rd ? 2 : IDEX_rs == MEMWB_rd ? 1 : 0;
            forwardB = IDEX_rt == EXMEM_rd ? 2 : IDEX_rt == MEMWB_rd ? 1 : 0;

            // if (IDEX_rs == EXMEM_rd)
            //     forwardA = 2; // 0b10
            // else if (IDEX_rs == MEMWB_rd)
            //     forwardA = 1; //0b01
            // else
            //     forwardA = 0; // No Forwarding
      }
};
#endif