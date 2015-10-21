#include <stdio.h>
#include <assert.h>

#include "rob.h"

using namespace std;

extern int32_t NUM_ROB_ENTRIES;

/////////////////////////////////////////////////////////////
// Init function initializes the ROB
/////////////////////////////////////////////////////////////

ROB* ROB_init(void){
  int ii;
  ROB *t = (ROB *) calloc (1, sizeof (ROB));
  for(ii=0; ii<MAX_ROB_ENTRIES; ii++){
    // Entry is not valid
    t->ROB_Entries[ii].valid=false;
    // Entry is not ready to be committed
    t->ROB_Entries[ii].ready=false;
    // Entry has not yet ready for execution
    // meaning, all sources are not available
    t->ROB_Entries[ii].exec=false;
  }
  t->head_ptr=0;
  t->tail_ptr=0;
  return t;
}

/////////////////////////////////////////////////////////////
// Print State
/////////////////////////////////////////////////////////////
void ROB_print_state(ROB *t){
 int ii = 0;
 int limit;
 limit = (t->tail_ptr >= t->head_ptr) ? t->tail_ptr : NUM_ROB_ENTRIES;
  printf("Printing ROB \n");
  printf("Entry  Inst   Valid   ready   exec\n");
  for(ii = 0; ii < limit; ii++) {
    printf("%5d ::  %d\t", ii, (int)t->ROB_Entries[ii].inst.inst_num);
    printf(" %5d\t", t->ROB_Entries[ii].valid);
    printf(" %5d\t", t->ROB_Entries[ii].ready);
    printf(" %5d\n", t->ROB_Entries[ii].exec);
  }
  printf("\n");
}

/////////////////////////////////////////////////////////////
// If there is space in ROB return true, else false
/////////////////////////////////////////////////////////////

// SO we will go into commit state and since it is scalar implementation
// we will only check the 'head' pointer and see if the 'valid' and 'ready'
// bits are 1, then put valid = 0 and move head_ptr++; meaning that since 
// we are committing in-order we only need to check for the head_ptr and 
// and need not to parse the whole ROB

// Also we can change these ROB field in the commit stage as opposed to 
// doing it in write-back stage

// In write back stage we will only set the ready bit to 1.


bool ROB_check_space(ROB *t){

  // Nathan Ans: can also check tail for the purpose of checking if the ROB is filled or not
  if ((t->ROB_Entries[t->tail_ptr].valid == false)) {
    return true;   // condition of NOT completely filled circular buffer
  } else {
    return false;   // If completely filled, this implies there is NO space
  } 

}

/////////////////////////////////////////////////////////////
// insert entry at tail, increment tail (do check_space first)
/////////////////////////////////////////////////////////////

int ROB_insert(ROB *t, Inst_Info inst) { 
  if (ROB_check_space(t)) { // 'true' imply there is space
    // 'inst' is the entry to be inserted
    int local_tail;
    t->ROB_Entries[t->tail_ptr].valid = true;
    t->ROB_Entries[t->tail_ptr].inst = inst;
    // case of roll-over.. check 
    local_tail = t->tail_ptr;
    if (++t->tail_ptr == NUM_ROB_ENTRIES) {
      t->tail_ptr = 0;
    }
    return (local_tail); // we are using this value to know if operation was successful
  } else {
    return -1;  // we are using this value to know if operation was not successful
  }
}

/////////////////////////////////////////////////////////////
// When an inst gets scheduled for execution, mark exec
/////////////////////////////////////////////////////////////
// this has to be done in schedule
void ROB_mark_exec(ROB *t, Inst_Info inst){ 
  // In 'Inst_info' you would be having ROB field as well
  // to put valid/non-valid; also same should be true for
  // RAT fields
  // check for all entries in the ROB <for-loop>(starting from the head continuing till tail)
  // and if you find any of these having both operands ready mark 'exec' to be true
  // the loop variable 'i' should be able to roll over as well so that it can parse the whole ROB



// search and chweck the specific 'inst's sources are ready or not and mark 'exec' to ben true and check for valid as well
  for (int index = 0; index < NUM_ROB_ENTRIES; index++) {
    if (t->ROB_Entries[index].valid == true) {
      if (t->ROB_Entries[index].inst.inst_num == inst.inst_num) { //we have got the instruction
        /*
        cout << "----ROB_mark_exec(inst ready to be put in SCH:?)----\n" << 
        "t->ROB_Entries[index].inst.src1_tag: " << t->ROB_Entries[index].inst.src1_tag <<
        "\nt->ROB_Entries[index].inst.src1_ready: " << t->ROB_Entries[index].inst.src1_ready <<
        "\nt->ROB_Entries[index].inst.src2_tag: " << t->ROB_Entries[index].inst.src2_tag <<
        "\nt->ROB_Entries[index].inst.src2_ready: " << t->ROB_Entries[index].inst.src2_ready << endl;
        */
        if ((t->ROB_Entries[index].inst.src1_ready == true) && 
            (t->ROB_Entries[index].inst.src2_ready == true)) {
          t->ROB_Entries[index].exec = true;     // for the given inst; all src are ready; mark exec true 
          break;
        } else {
          t->ROB_Entries[index].exec = false; // both src are not ready  
          break;        
        }
      }
    }
  }

}


/////////////////////////////////////////////////////////////
// Once an instruction finishes execution, mark rob entry as done
/////////////////////////////////////////////////////////////
// this will be done in write-back
// for all instruction between head
// and tail <-- Only for the instn
// passed as an argument? TODO

void ROB_mark_ready(ROB *t, Inst_Info inst) { 
  // you are searching for this 'insn' in the rob if it's eexec true then mark it ready and come out
  for (int index = 0; index < NUM_ROB_ENTRIES; index++) {
    if (t->ROB_Entries[index].valid == true) {
      if ( t->ROB_Entries[index].inst.inst_num == inst.inst_num) { //we have got thr instruction
        if (t->ROB_Entries[index].exec == true) {
          t->ROB_Entries[index].ready = true;     //ready to be committed
          break;
        } else {
          t->ROB_Entries[index].ready = true;     //NOT-ready to be committed     
          break;     
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////
// Find whether the prf (rob entry) is ready
/////////////////////////////////////////////////////////////
//  this function should take 'tag (prf_id)' of RoB and should tell
//  if it is ready.. you need to call this fucntion from elsewhere

bool ROB_check_ready(ROB *t, int tag){  
// check ready for s
  if ((t->ROB_Entries[tag].ready == true) && (t->ROB_Entries[tag].valid == true)) {
    return true;   // the entry at given tag is ready to be committed 
  } else {
    return false;   // the entry at given tag is NOT ready to be comitted
  }
}


/////////////////////////////////////////////////////////////
// Check if the oldest ROB entry is ready for commit
/////////////////////////////////////////////////////////////
// only check if head_ptr is ready to commit
// we will do this in commit stage also?
bool ROB_check_head(ROB *t){  
// always checkk for 'valid'
  if ((t->ROB_Entries[t->head_ptr].ready == true) && (t->ROB_Entries[t->head_ptr].valid == true)) { 
    return true;    // entry head is pointing to is ready to be comitted
  } else {
    return false;
  } 

}

/////////////////////////////////////////////////////////////
// For writeback of freshly ready tags, wakeup waiting inst
/////////////////////////////////////////////////////////////
// so every instrn which has this 'tag' as their source_tag;
// put source_tag as '-1' and src_ready as 1 ;
// and check this for all the RoB

// we will do this in write back
// to this for all entries between
// head and tail
void  ROB_wakeup(ROB *t, int tag){  

  // 'tag' is 'dr_tag' update src_tag for all entries in ROB make sure it's valid
  // get the 'prf_id' of the given 'tag' from RAT mapping only if it's a valid mapping
  // dr_tag is 'prf_id' so no need to get it from RAT: CHECK
  
  // Also update the entry 

  for (int index = 0; index < NUM_ROB_ENTRIES; index++) {
      if (t->ROB_Entries[index].valid == true) { // CHECK: need to check 'exec' here as well?
        if (t->ROB_Entries[index].inst.src1_tag == tag) {
            t->ROB_Entries[index].inst.src1_ready = true;
            t->ROB_Entries[index].inst.src1_tag = -1;
          }
        if (t->ROB_Entries[index].inst.src2_tag == tag) {
            t->ROB_Entries[index].inst.src2_ready = true;
            t->ROB_Entries[index].inst.src2_tag = -1;
          }
        }
      }
  }



/////////////////////////////////////////////////////////////
// Remove oldest entry from ROB (after ROB_check_head)
/////////////////////////////////////////////////////////////
// head_pointer can only be moved in commit stage

// We will do this in commit
// put the valid == 0 and then 
// head_ptr++
// what instrn_info should we return??
// of new 'head_ptr' or old 'head_ptr'
// Inst_Info for the one you have removed
Inst_Info ROB_remove_head(ROB *t){ 

  Inst_Info tmp;
  tmp.inst_num = -1;

  if (ROB_check_head(t)) {
    t->ROB_Entries[t->head_ptr].valid = false;
    // t->ROB_Entries[t->head_ptr].exec = false;
    // t->ROB_Entries[t->head_ptr].ready = false;
    tmp = t->ROB_Entries[t->head_ptr].inst;
    if (++t->head_ptr == NUM_ROB_ENTRIES) {
      t->head_ptr = 0;
    }
    return tmp;
  } else {
    return tmp;  // check for the 'inst.inst_num == -1' to know if head is removed successfully or not    
  }
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
