#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "memsys.h"

#define PAGE_SIZE 4096

//---- Cache Latencies  ------

#define DCACHE_HIT_LATENCY   1
#define ICACHE_HIT_LATENCY   1
#define L2CACHE_HIT_LATENCY  10

extern MODE   SIM_MODE;
extern uns64  CACHE_LINESIZE;
extern uns64  REPL_POLICY;

extern uns64  DCACHE_SIZE; 
extern uns64  DCACHE_ASSOC; 
extern uns64  ICACHE_SIZE; 
extern uns64  ICACHE_ASSOC; 
extern uns64  L2CACHE_SIZE; 
extern uns64  L2CACHE_ASSOC;
extern uns64  L2CACHE_REPL;
extern uns64  NUM_CORES;
extern uns64 	cycle;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


Memsys *memsys_new(void) 
{
  Memsys *sys = (Memsys *) calloc (1, sizeof (Memsys));

  if(SIM_MODE==SIM_MODE_A){
    sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
  }

  if(SIM_MODE==SIM_MODE_B){
    sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
    sys->icache = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
    sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
    sys->dram    = dram_new();
  }

  if(SIM_MODE==SIM_MODE_C){
    sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
    sys->icache = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
    sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
    sys->dram    = dram_new();
  }

  if( (SIM_MODE==SIM_MODE_D) || (SIM_MODE==SIM_MODE_E)) {
    sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, L2CACHE_REPL);
    sys->dram    = dram_new();
    uns ii;
    for(ii=0; ii<NUM_CORES; ii++){
      sys->dcache_coreid[ii] = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
      sys->icache_coreid[ii] = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
    }
  }

  return sys;
}


////////////////////////////////////////////////////////////////////
// This function takes an ifetch/ldst access and returns the delay
////////////////////////////////////////////////////////////////////

uns64 memsys_access(Memsys *sys, Addr addr, Access_Type type, uns core_id)
{
  uns delay=0;


  // all cache transactions happen at line granularity, so get lineaddr
  Addr lineaddr=addr/CACHE_LINESIZE;

  if(SIM_MODE==SIM_MODE_A){
    delay = memsys_access_modeA(sys,lineaddr,type, core_id);
  }

  if((SIM_MODE==SIM_MODE_B)||(SIM_MODE==SIM_MODE_C)){
    delay = memsys_access_modeBC(sys,lineaddr,type, core_id);
  }

  if((SIM_MODE==SIM_MODE_D)||(SIM_MODE==SIM_MODE_E)){
    delay = memsys_access_modeDE(sys,lineaddr,type, core_id);
  }
  
  //update the stats
  if(type==ACCESS_TYPE_IFETCH){
    sys->stat_ifetch_access++;
    sys->stat_ifetch_delay+=delay;
  }

  if(type==ACCESS_TYPE_LOAD){
    sys->stat_load_access++;
    sys->stat_load_delay+=delay;
  }

  if(type==ACCESS_TYPE_STORE){
    sys->stat_store_access++;
    sys->stat_store_delay+=delay;
  }


  return delay;
}



////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void memsys_print_stats(Memsys *sys)
{
  char header[256];
  sprintf(header, "MEMSYS");

  double ifetch_delay_avg=0;
  double load_delay_avg=0;
  double store_delay_avg=0;

  if(sys->stat_ifetch_access){
    ifetch_delay_avg = (double)(sys->stat_ifetch_delay)/(double)(sys->stat_ifetch_access);
  }

  if(sys->stat_load_access){
    load_delay_avg = (double)(sys->stat_load_delay)/(double)(sys->stat_load_access);
  }

  if(sys->stat_store_access){
    store_delay_avg = (double)(sys->stat_store_delay)/(double)(sys->stat_store_access);
  }


  printf("\n");
  printf("\n%s_IFETCH_ACCESS  \t\t : %10llu",  header, sys->stat_ifetch_access);
  printf("\n%s_LOAD_ACCESS    \t\t : %10llu",  header, sys->stat_load_access);
  printf("\n%s_STORE_ACCESS   \t\t : %10llu",  header, sys->stat_store_access);
  printf("\n%s_IFETCH_AVGDELAY\t\t : %10.3f",  header, ifetch_delay_avg);
  printf("\n%s_LOAD_AVGDELAY  \t\t : %10.3f",  header, load_delay_avg);
  printf("\n%s_STORE_AVGDELAY \t\t : %10.3f",  header, store_delay_avg);
  printf("\n");

   if(SIM_MODE==SIM_MODE_A){
    sprintf(header, "DCACHE");
    cache_print_stats(sys->dcache, header);
  }
  
  if((SIM_MODE==SIM_MODE_B)||(SIM_MODE==SIM_MODE_C)){
    sprintf(header, "ICACHE");
	cache_print_stats(sys->icache, header);
	sprintf(header, "DCACHE");
    cache_print_stats(sys->dcache, header);
	sprintf(header, "L2CACHE");
    cache_print_stats(sys->l2cache, header);
    dram_print_stats(sys->dram);
  }

  if((SIM_MODE==SIM_MODE_D)||(SIM_MODE==SIM_MODE_E)){
    assert(NUM_CORES==2); //Hardcoded
	sprintf(header, "ICACHE_0");
    cache_print_stats(sys->icache_coreid[0], header);
	sprintf(header, "DCACHE_0");
    cache_print_stats(sys->dcache_coreid[0], header);
	sprintf(header, "ICACHE_1");
    cache_print_stats(sys->icache_coreid[1], header);
	sprintf(header, "DCACHE_1");
    cache_print_stats(sys->dcache_coreid[1], header);
	sprintf(header, "L2CACHE");
    cache_print_stats(sys->l2cache, header);
    dram_print_stats(sys->dram);
    
  }

}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

uns64 memsys_access_modeA(Memsys *sys, Addr lineaddr, Access_Type type, uns core_id){
 Flag needs_dcache_access=FALSE;
  Flag is_write=FALSE;
  
  if(type == ACCESS_TYPE_IFETCH){
    // no icache in this mode
  }
    
  if(type == ACCESS_TYPE_LOAD){
    needs_dcache_access=TRUE;
    is_write=FALSE;
  }
  
  if(type == ACCESS_TYPE_STORE){
    needs_dcache_access=TRUE;
    is_write=TRUE;
  }

  if(needs_dcache_access){
    Flag outcome=cache_access(sys->dcache, lineaddr, is_write, core_id);
    if(outcome==MISS){
      cache_install(sys->dcache, lineaddr, is_write, core_id);
    }
  }

  // timing is not simulated in Part A
  return 0;
}

// Returns the delay required to service a given request
uns64 memsys_access_modeBC(Memsys *sys, Addr lineaddr, Access_Type type,uns core_id){
  uns64 delay=0;
  Flag needs_icache_access=FALSE;
  Flag needs_dcache_access=FALSE;
  Flag needs_l2_access=FALSE;
  Flag is_write=FALSE;
  Flag outcome_L1; // local variable to know if it's a hit or not (miss)
  Flag outcome_L2;
     
  if(type == ACCESS_TYPE_IFETCH){
    // YOU NEED TO WRITE THIS PART AND UPDATE DELAY
    // first see if it's a L1 cache hit, if yes then add ICACHE_HIT_LATENCY
    // If it's a miss then call memsys_L2_access().. [This function will take care
    // if there's a further L2 miss/hit]

    needs_icache_access = TRUE;
    needs_dcache_access = FALSE;
    is_write = FALSE;
    Flag is_writeback = FALSE;


    outcome_L1 = cache_access(sys->icache, lineaddr, is_write, core_id); // L1-ICACHE Access

    if (outcome_L1 == HIT) {
      delay = ICACHE_HIT_LATENCY;
    } else { // there's L1 miss
      // If there is a L1 MISS then we need to find that line in L2
      outcome_L2 = cache_access(sys->l2cache, lineaddr, is_write, core_id);
      delay += L2CACHE_HIT_LATENCY;
      // If L2 is a hit.. then install the line in L1.. and if something get's
      // evicted due to this and is dirty.. install it in L2 mark is_writeback true,
      // if something get's evicted which is not dirty still install it in L2
      // if nothing get's evicted.. don't install it in L2
      if (outcome_L2 == HIT) {
        cache_install(sys->icache, lineaddr, is_write, core_id);
        // Would this result in eviction.. if yes install the evicted line in L2
        // and if the evicted line is dirty mark 'is_writeback' true..
        if (sys->icache->last_evicted_line.valid) {
          // First see if evicted line from L1 is dirty.. if yes then install it in L2 blindly
          // if not dirty then check if line is present in L2(L2-hit), if yes, then don't install
          // it, otherwise install it.
          if (sys->icache->last_evicted_line.dirty) {
            is_writeback = TRUE;
            cache_install(sys->l2cache, sys->icache->last_evicted_line.tag, is_write, core_id);
            // This installation may further result in L2 eviction.. if those eviction are dirty
            // install them in dram, otherwise don't install them!
            if ((sys->l2cache->last_evicted_line.valid) && (sys->l2cache->last_evicted_line.dirty)) {
              // Check: Do I need to but 'is_writeback' true here as well?
              dram_access(sys->dram, sys->l2cache->last_evicted_line.tag, true);
              // Check: this evicted line has been put in DRAM.. now put valid as false
              sys->l2cache->last_evicted_line.valid = FALSE;
            }
          } else { // If evicted line is not dirty
            // If evicted line is not dirty then check if it's L2 hit, if yes don't do anything
            // if not then install it in L2, again there will be eviction due to this installation
            // in L2, if the evicted line is dirty write-back to memory
            Flag outcome_L1_evic_L2 = cache_access(sys->l2cache, sys->icache->last_evicted_line.tag,
                                                   is_write, core_id); 
            if (outcome_L1_evic_L2 == HIT) {
              // Do nothing
            } else { // 
              cache_install(sys->l2cache, sys->icache->last_evicted_line.tag, is_write, core_id);
              // This installation may further result in L2 eviction.. if those eviction are dirty
              // install them in dram, otherwise don't install them!
              if ((sys->l2cache->last_evicted_line.valid) && (sys->l2cache->last_evicted_line.dirty)) {
                // Check: Do I need to but 'is_writeback' true here as well?
                dram_access(sys->dram, sys->l2cache->last_evicted_line.tag, true);
                sys->l2cache->last_evicted_line.valid = FALSE;
              }              
            }
          }
          // L1 evited line is taken care-of, now put evicted-line as false
          sys->icache->last_evicted_line.valid = FALSE;
        }
      } else { // If there's an L2 Miss
      // you first install the line both at L2 and L1 from DRAM (update delay)
      // check if evicted line from L2 is valid and dirty, if yes write it to dram
      // check if evicted line from L1 is dirty, if yes write it blindly to L2, again
      // if evicted line from L2 is valid and dirty, if yes write it to dram
      // if evicted line from L1 is not dirty, check if L2 already have it.. if it has, do nothing
      // if not put the line in L2 and if evicted line from L2 is valid and dirty, if yes write it to dram
      
      // got the line from DRAM
      delay += dram_access(sys->dram, lineaddr, is_write);
      // L2 install:
      cache_install(sys->l2cache, lineaddr, is_write, core_id);
      // L1 install:
      cache_install(sys->icache, lineaddr, is_write, core_id);
      // if evicted line from L2 is valid and dirty, if yes write it to dram
      if ((sys->l2cache->last_evicted_line.valid) && (sys->l2cache->last_evicted_line.dirty)) {
        // Check: Do I need to but 'is_writeback' true here as well?
        dram_access(sys->dram, sys->l2cache->last_evicted_line.tag, true);
        sys->l2cache->last_evicted_line.valid = FALSE;      
      }
      // if evicted line from L1 is valid and dirty 
      if (sys->icache->last_evicted_line.valid) {
        if (sys->icache->last_evicted_line.dirty) {
            cache_install(sys->l2cache, sys->icache->last_evicted_line.tag, is_write, core_id);
            if ((sys->l2cache->last_evicted_line.valid) && (sys->l2cache->last_evicted_line.dirty)) {
              dram_access(sys->dram, sys->l2cache->last_evicted_line.tag, true);
              sys->l2cache->last_evicted_line.valid = FALSE;
            }
          } else { 
            Flag outcome_L1_evic_L2 = cache_access(sys->l2cache, sys->icache->last_evicted_line.tag,
                                                   is_write, core_id); 
            if (outcome_L1_evic_L2 == HIT) {
              // Do nothing
            } else { // 
              cache_install(sys->l2cache, sys->icache->last_evicted_line.tag, is_write, core_id);
              if ((sys->l2cache->last_evicted_line.valid) && (sys->l2cache->last_evicted_line.dirty)) {
                // Check: Do I need to but 'is_writeback' true here as well?
                dram_access(sys->dram, sys->l2cache->last_evicted_line.tag, true);
                sys->l2cache->last_evicted_line.valid = FALSE;
              }              
            }
          }
          // L1 evited line is taken care-of, now put evicted-line as false
          sys->icache->last_evicted_line.valid = FALSE;
        }


      }
    }


    // Do not forget to update the stat
    sys->stat_ifetch_access++;
    // Check: In 'stat_ifetch_delay' do we keep adding the whole delay parameter.. or just increment it
    // going with the former one
    sys->stat_ifetch_delay = delay + sys->stat_ifetch_delay;
  }
    
  if(type == ACCESS_TYPE_LOAD){
    // YOU NEED TO WRITE THIS PART AND UPDATE DELAY
    Flag hit; 
    hit = cache_access(sys->dcache, lineaddr, false /*is_write*/, core_id); // L1-DCACHE Access

    if (hit == HIT) {
      delay = DCACHE_HIT_LATENCY;
    } else {
      delay = DCACHE_HIT_LATENCY + memsys_L2_access(sys, lineaddr, false /*is_writeback*/, core_id);
    }

    // Do not forget to update the stat.
    sys->stat_load_access++;
    // Check: In 'stat_load_delay' do we keep adding the whole delay parameter.. or just increment it
    // going with the former one
    sys->stat_load_delay = delay + sys->stat_load_delay;    
  }
  

  if(type == ACCESS_TYPE_STORE){
    // YOU NEED TO WRITE THIS PART AND UPDATE DELAY
    Flag hit; 
    hit = cache_access(sys->dcache, lineaddr, true /*is_write*/, core_id); // L1-DCACHE Access

    if (hit == HIT) {
      delay = DCACHE_HIT_LATENCY;
    } else {
      delay = DCACHE_HIT_LATENCY + memsys_L2_access(sys, lineaddr, true /*is_writeback*/, core_id);
    }

    // Do not forget to update the stat.
    sys->stat_store_access++;
    // Check: In 'stat_store_delay' do we keep adding the whole delay parameter.. or just increment it
    // going with the former one
    sys->stat_store_delay = delay + sys->stat_store_delay;       
  }
 
  return delay;
}

// Note: All caches are Write-back and Allocate-On-Write-Miss
// memsys_L2_access is called by memsys_access_modeBC and returns the delay for servicing a request that
// access the L2 cache
uns64   memsys_L2_access(Memsys *sys, Addr lineaddr, Flag is_writeback, uns core_id) {

  uns64 delay = L2CACHE_HIT_LATENCY;

  //To get the delay of L2 MISS, you must use the dram_access() function
  //To perform writebacks to memory, you must use the dram_access() function
  //This will help us track your memory reads and memory writes
  ///////////////////////////////////////////////////////////////
/*
  // If there is a L1 MISS then we need to find that line in L2
  outcome_L2 = cache_access(sys->l2cache, lineaddr, is_write, core_id);
  // If L2 is a hit.. then install the line in L1.. and if something get's
  // evicted due to this and is dirty.. install it in L2 mark is_writeback true,
  // if something get's evicted which is not dirty still install it in L2
  // if nothing get's evicted.. don't install it in L2
  if (outcome_L2 == HIT) {
    cache_install(sys->icache, lineaddr, is_write, core_id);
    // Would this result in eviction.. if yes install the evicted line in L2
    // and if the evicted line is dirty mark 'is_writeback' true..
    if (sys->icache->last_evicted_line.valid) {
      // First see if evicted line from L1 is dirty.. if yes then install it in L2 blindly
      // if not dirty then check if line is present in L2(L2-hit), if yes, then don't install
      // it, otherwise install it.
      if (sys->icache->last_evicted_line.dirty) {
        Flag is_writeback = TRUE;
        cache_install(sys->l2cache, sys->icache->last_evicted_line.tag, is_write, core_id);
        // This installation may further result in L2 eviction.. if those eviction are dirty
        // install them in dram, otherwise don't install them!
        if ((sys->l2cache->last_evicted_line.valid) && (sys->l2cache->last_evicted_line.dirty)) {
          // Check: Do I need to but 'is_writeback' true here as well?
          dram_access(sys->dram, sys->l2cache->last_evicted_line.tag, true);
          // Check: this evicted line has been put in DRAM.. now put valid as false
          sys->l2cache->last_evicted_line.valid = FALSE;
        }
      } else { // If evicted line is not dirty
        // If evicted line is not dirty then check if it's L2 hit, if yes don't do anything
        // if not then install it in L2, again there will be eviction due to this installation
        // in L2, if the evicted line is dirty write-back to memory
        Flag outcome_L1_evic_L2 = cache_access(sys->l2cache, sys->icache->last_evicted_line.tag,
                                               is_write, core_id); 
        if (outcome_L1_evic_L2 == HIT) {
        // Do nothing
        } else { // 
          cache_install(sys->l2cache, sys->icache->last_evicted_line.tag, is_write, core_id);
          // This installation may further result in L2 eviction.. if those eviction are dirty
          // install them in dram, otherwise don't install them!
          if ((sys->l2cache->last_evicted_line.valid) && (sys->l2cache->last_evicted_line.dirty)) {
            // Check: Do I need to but 'is_writeback' true here as well?
            dram_access(sys->dram, sys->l2cache->last_evicted_line.tag, true);
            sys->l2cache->last_evicted_line.valid = FALSE;
          }              
        }
      }
      // L1 evited line is taken care-of, now put evicted-line as false
      sys->icache->last_evicted_line.valid = FALSE;
    }
  }

  delay = ICACHE_HIT_LATENCY + memsys_L2_access(sys, lineaddr, false , core_id);
*/

  return delay;
}

/////////////////////////////////////////////////////////////////////
// This function converts virtual page number (VPN) to physical frame
// number (PFN).  Note, you will need additional operations to obtain
// VPN from lineaddr and to get physical lineaddr using PFN. 
/////////////////////////////////////////////////////////////////////

uns64 memsys_convert_vpn_to_pfn(Memsys *sys, uns64 vpn, uns core_id){
  uns64 tail = vpn & 0x000fffff;
  uns64 head = vpn >> 20;
  uns64 pfn  = tail + (core_id << 21) + (head << 21);
  assert(NUM_CORES==2);
  return pfn;
}

////////////////////////////////////////////////////////////////////
// --------------- DO NOT CHANGE THE CODE ABOVE THIS LINE ----------
////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////
// For Mode D/E you will use per-core ICACHE and DCACHE
// ----- YOU NEED TO WRITE THIS FUNCTION AND UPDATE DELAY ----------
/////////////////////////////////////////////////////////////////////



uns64 memsys_access_modeDE(Memsys *sys, Addr v_lineaddr, Access_Type type,uns core_id){
  uns64 delay=0;
  Addr p_lineaddr=0;
	
  p_lineaddr = v_lineaddr;
  
  // TODO: First convert lineaddr from virtual (v) to physical (p) using the
  // function memsys_convert_vpn_to_pfn. Page size is defined to be 4KB.
  // NOTE: VPN_to_PFN operates at page granularity and returns page addr

  if(type == ACCESS_TYPE_IFETCH){
    // YOU NEED TO WRITE THIS PART AND UPDATE DELAY
  }

  if(type == ACCESS_TYPE_LOAD){
    // YOU NEED TO WRITE THIS PART AND UPDATE DELAY
  }
  
  if(type == ACCESS_TYPE_STORE){
    // YOU NEED TO WRITE THIS PART AND UPDATE DELAY
  }
 
  return delay;
}


/////////////////////////////////////////////////////////////////////
// This function is called on ICACHE miss, DCACHE miss, DCACHE writeback
// ----- YOU NEED TO WRITE THIS FUNCTION AND UPDATE DELAY ----------
/////////////////////////////////////////////////////////////////////

uns64   memsys_L2_access_multicore(Memsys *sys, Addr lineaddr, Flag is_writeback, uns core_id){
  uns64 delay = L2CACHE_HIT_LATENCY;
	
  //To get the delay of L2 MISS, you must use the dram_access() function
  //To perform writebacks to memory, you must use the dram_access() function
  //This will help us track your memory reads and memory writes
	
  return delay;
}

