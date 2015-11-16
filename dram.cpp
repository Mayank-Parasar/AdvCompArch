#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "dram.h"

#define ROWBUF_SIZE         1024
#define DRAM_BANKS          16

//---- Latency for Part B ------

#define DRAM_LATENCY_FIXED  100

//---- Latencies for Part C,D,E --

#define DRAM_T_ACT         45
#define DRAM_T_CAS         45
#define DRAM_T_PRE         45
#define DRAM_T_BUS         10


extern MODE   SIM_MODE;
extern uns64  CACHE_LINESIZE;
extern uns64 cycle; // You can use this as timestamp for LRU


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

DRAM   *dram_new(){
  DRAM *dram = (DRAM *) calloc (1, sizeof (DRAM));
  assert(DRAM_BANKS <= MAX_DRAM_BANKS);
  return dram;
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

void    dram_print_stats(DRAM *dram){
  double rddelay_avg=0;
  double wrdelay_avg=0;
  char header[256];
  sprintf(header, "DRAM");
  
  if(dram->stat_read_access){
    rddelay_avg=(double)(dram->stat_read_delay)/(double)(dram->stat_read_access);
  }

  if(dram->stat_write_access){
    wrdelay_avg=(double)(dram->stat_write_delay)/(double)(dram->stat_write_access);
  }

  printf("\n%s_READ_ACCESS\t\t : %10llu", header, dram->stat_read_access);
  printf("\n%s_WRITE_ACCESS\t\t : %10llu", header, dram->stat_write_access);
  printf("\n%s_READ_DELAY_AVG\t\t : %10.3f", header, rddelay_avg);
  printf("\n%s_WRITE_DELAY_AVG\t\t : %10.3f", header, wrdelay_avg);


}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

uns64   dram_access(DRAM *dram,Addr lineaddr, Flag is_dram_write) {
  uns64 delay=DRAM_LATENCY_FIXED;

  if(SIM_MODE!=SIM_MODE_B){
    delay = dram_access_mode_CDE(dram, lineaddr, is_dram_write);
  }

  // Update stats
  if(is_dram_write){
    dram->stat_write_access++;
    dram->stat_write_delay+=delay;
  }else{
    dram->stat_read_access++;
    dram->stat_read_delay+=delay;
  }
  
  return delay;
}

///////////////////////////////////////////////////////////////////
// Modify the function below only for Parts C/D/E
///////////////////////////////////////////////////////////////////

uns64   dram_access_mode_CDE(DRAM *dram,Addr lineaddr, Flag is_dram_write){
  uns64 delay=DRAM_LATENCY_FIXED;

    // Assume a mapping with consecutive lines in the same row
    // Assume a mapping with consecutive rowbufs in consecutive rows
    // You need to write this fuction to track open rows 
    // You will need to compute delay based on row hit/miss/empty

  // Open page policy: This means that when we access DRAM data in 
  // row buffer will be remained as it is.. if we access it again
  // then it's a DRAM hit if it present in the same row

  // First check if 'lineaddr' is a hit or a miss
    // line-offset:= size of DRAM row/(size of cacheline)
    // intermediate addr = lineaddr/(line-offset)
    // num_bank = intermediate addr%16
    // num_dram_row = intermediate addr/16
  uns64 line_offset = ROWBUF_SIZE/CACHE_LINESIZE;
  uns64 inter_addr = lineaddr/line_offset; // [row-id + bank_num] => inter_addr
  uns64 num_bank = inter_addr % DRAM_BANKS;
  uns64 num_dram_row = inter_addr/DRAM_BANKS;

  assert (num_bank < DRAM_BANKS);
  // What if row buffer is empty!.. there is a valid bit for that..
  if (dram->perbank_row_buf[num_bank].valid == false) {
    delay = DRAM_T_ACT + DRAM_T_CAS + DRAM_T_BUS;
    // Now put valid as true
    dram->perbank_row_buf[num_bank].valid = true;
    // Initialize the row as well!!
    dram->perbank_row_buf[num_bank].rowid = num_dram_row;
  }
  // If hit; calculate delay
  else if((dram->perbank_row_buf[num_bank].rowid == num_dram_row) && 
      dram->perbank_row_buf[num_bank].valid) {
    delay = DRAM_T_CAS + DRAM_T_BUS;
  }
  // if miss; calculate delay
  else if ((dram->perbank_row_buf[num_bank].rowid != num_dram_row) && 
      dram->perbank_row_buf[num_bank].valid) {  
    // This is a case of miss.. so bring the required row in the row-buffer
    dram->perbank_row_buf[num_bank].rowid = num_dram_row;
    // calculate delay
    delay = DRAM_T_ACT + DRAM_T_PRE + DRAM_T_CAS + DRAM_T_BUS;
  }
  return delay;
}


