#include <assert.h>
#include <stdio.h>

#include "rat.h"


/////////////////////////////////////////////////////////////
// Init function initializes the RAT
/////////////////////////////////////////////////////////////

RAT* RAT_init(void){
  int ii;
  RAT *t = (RAT *) calloc (1, sizeof (RAT));
  for(ii=0; ii<MAX_ARF_REGS; ii++){
    t->RAT_Entries[ii].valid=false;
  }
  return t;
}

/////////////////////////////////////////////////////////////
// Print State
/////////////////////////////////////////////////////////////
void RAT_print_state(RAT *t){
 int ii = 0;
  printf("Printing RAT \n");
  printf("Entry  Valid  prf_id\n");
  for(ii = 0; ii < MAX_ARF_REGS; ii++) {
    printf("%5d ::  %d \t\t", ii, t->RAT_Entries[ii].valid);
    printf("%5d \n", (int)t->RAT_Entries[ii].prf_id);
  }
  printf("\n");
}

/////////////////////////////////////////////////////////////
//------- DO NOT CHANGE THE CODE ABOVE THIS LINE -----------
/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
// For source registers, we need RAT to provide renamed reg
/////////////////////////////////////////////////////////////

int  RAT_get_remap(RAT *t, int arf_id){
  // we have 'ard_id' as the index into RAT
  // only provide the 'prf_id' if the entry
  // in RAT is valid; otherwise return '-1'
  // which can be used as the check for invalid
  // entry
  if (t->RAT_Entries[arf_id].valid) {
    return (t->RAT_Entries[arf_id].prf_id);
  } else {
    return -1;
  }

}

/////////////////////////////////////////////////////////////
// For destination regs, we need to remap ARF to PRF
/////////////////////////////////////////////////////////////

void RAT_set_remap(RAT *t, int arf_id, int prf_id){
  // Assuming that we have given proper 'arf_id'
  // and 'prf_id' and we want to put them into 
  // one entry
  // We also need to set valid as 1, as we are 
  // creating a valid mapping.. the check of
  // wheather mapping is valid or not should be 
  // done earlier in the code, before calling this
  // function

  t->RAT_Entries[arf_id].valid = true;
  t->RAT_Entries[arf_id].prf_id = prf_id;


}

/////////////////////////////////////////////////////////////
// On commit, we may need to reset RAT information 
/////////////////////////////////////////////////////////////

void RAT_reset_entry(RAT *t, int arf_id){
  // We have commited the instruction and now we would
  // like to clean up that RAT entry. 
  // we have the index of the entry; set valid to 0 for
  // this entry can 'reset' it.

  t->RAT_Entries[arf_id].valid = false;

}


/***********************************************************/
