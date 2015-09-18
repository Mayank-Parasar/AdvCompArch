#include "bpred.h"
#include <assert.h>

#define TAKEN   true
#define NOTTAKEN false

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

BPRED::BPRED(uint32_t policy) {

 int i;
// Initialization
 stat_num_branches = 0;
 stat_num_mispred = 0;
 GHR = 0;

this->policy = (BPRED_TYPE_ENUM)policy; // Used 'this->' because 'policy' is a 
                                        // private member
// std::fill(this->PHT, 4096, 2);  
// a PHT consisting of 2-bit counters initialized to the weekly taken state '10'
for (i=0; i<4096; i++)
    PHT[i] = 2; 

}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bool BPRED::GetPrediction(uint32_t PC){
    
    uint16_t index;
    uint8_t val;   
//    bool returntype;
 

    if (this->policy == 2) {    //G-Share Predictor
     index = (PC & 0x00000FFF) ^ (GHR & 0x0FFF);
     val = PHT[index];

     switch (val) {
        case 0: return NOTTAKEN;
                break;
        case 1: return NOTTAKEN;
                break;
        case 2: return TAKEN;
                break;
        case 3: return TAKEN;
                break;

        default: assert(0/*,"ERR: No Legit value identified!!"*/); // Need to print custom text message
        }

    } else {        //AlwaysTaken Predictor
        return TAKEN;  
    }
}


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void  BPRED::UpdatePredictor(uint32_t PC, bool resolveDir, bool predDir) {

bool returntype;
uint16_t index;
uint8_t val;
// Second Update GHR with resolvedDir
// First, Update the PHT of same index with correct value 
 if (this->policy == 2) {
     index = (PC & 0x00000FFF) ^ (GHR & 0x0FFF);
     val = PHT[index];

    // Update the PHT
     if (val == 0) {
            if (resolveDir == TAKEN)
                PHT[index] = 1;
            else
                PHT[index] = 0;
    } else if (val == 1) {
            if (resolveDir == TAKEN)
                PHT[index] = 2;
            else
                PHT[index] = 0;
    } else if (val == 2) {
            if (resolveDir == TAKEN)
                PHT[index] = 3;
            else
                PHT[index] = 1;
    } else if (val == 3) {
            if (resolveDir == TAKEN)
                PHT[index] = 3;
            else
                PHT[index] = 2;
    }

    // Update GHR with resolveDir
    GHR = (((GHR<<1) | resolveDir) & 0x0FFF); 
    }
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

