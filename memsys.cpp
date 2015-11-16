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
    // printf("At memsys.cpp: %d\n", __LINE__);
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


uns64 memsys_access_modeBC(Memsys *sys, Addr lineaddr, Access_Type type,uns core_id){
  uns64 delay=0;
  Flag needs_dcache_access = FALSE;
  Flag is_dirty = FALSE;
  Flag outcome_L1 = MISS;

     
  if(type == ACCESS_TYPE_IFETCH){
    // YOU NEED TO WRITE THIS PART AND UPDATE DELAY
    outcome_L1 = cache_access(sys->icache, lineaddr, 0, core_id); // Reading line form L1 icache

    delay = ICACHE_HIT_LATENCY; // updated delay for the read

    if (outcome_L1 == MISS) {   // In case there is a miss, we will read from L2
      delay += memsys_L2_access(sys, lineaddr, 0, core_id); // put the delay of reading it from L2
      // Install the line in L1, no writeback
      cache_install(sys->icache, lineaddr, 0, core_id); // Install the line
    }
  }
    
  if(type == ACCESS_TYPE_LOAD){
    // YOU NEED TO WRITE THIS PART AND UPDATE DELAY
    needs_dcache_access = TRUE;
    is_dirty = FALSE;
  }
  

  if(type == ACCESS_TYPE_STORE){
    // YOU NEED TO WRITE THIS PART AND UPDATE DELAY

    needs_dcache_access = TRUE;
    is_dirty = TRUE;
  }


  if(needs_dcache_access) { // We have LOAD/STORE instruction
    // Accessing L1 dcache(for reading/writing based on 'is_dirty') 
    outcome_L1 = cache_access(sys->dcache, lineaddr, is_dirty, core_id); 

    delay = DCACHE_HIT_LATENCY; // initialized the delay for DCACHE access

    if(outcome_L1 == MISS) { // L1 cache miss
      // We are following 'non-inclusive' policy here.
      // read from L2
      // compensation for delay
      delay +=memsys_L2_access(sys, lineaddr, 0/*is_dirty*/, core_id); // because it is write back cache.
                                                                       // we will only write back on dirty
                                                                       // eviction 
      // cache_install() function takes care of eviction_stat
      cache_install(sys->dcache, lineaddr, is_dirty, core_id);
      // if the evicted line is not dirty.. we do not put it in L2
      if(sys->dcache->last_evicted_line.dirty && 
         sys->dcache->last_evicted_line.valid) {
        sys->dcache->last_evicted_line.dirty = FALSE;
        sys->dcache->last_evicted_line.valid = FALSE;
        Addr evit_L1_addr = sys->dcache->last_evicted_line.tag;
        // we are writing back the evicted line to L2 which are 'Dirty': Write-back   
        memsys_L2_access(sys, evit_L1_addr, 1, core_id); 
      }
     
    }
  }

 
  return delay;
}

uns64   memsys_L2_access(Memsys *sys, Addr lineaddr, Flag is_writeback, uns core_id){


  //To get the delay of L2 MISS, you must use the dram_access() function
  //To perform writebacks to memory, you must use the dram_access() function
  //This will help us track your memory reads and memory writes
  
  uns64 delay = L2CACHE_HIT_LATENCY; // initialized the delay with Hit latency
  Flag outcome_L2;


  if(is_writeback == 0) { //Reading <both for LD/ST> (requesting line in case of store as following WB strategy)
    // Read request from L1
    outcome_L2 = cache_access(sys->l2cache, lineaddr, 0, core_id); // Reading line from L2 in case of L1 miss
    if (outcome_L2 == MISS) { // If there is L2 miss on read

      // Delay for DRAM access
      // Reading the cache-line from DRAM (Get the line from DRAM)
      delay += dram_access(sys->dram, lineaddr, 0); 
      // cache_install() takes care of eviction stat 
      cache_install(sys->l2cache, lineaddr, 0, core_id); // Install the line into L2.. it is not dirty
      // If the evicted line is dirty you need to write it to DRAM, otherwise no action required
      if(sys->l2cache->last_evicted_line.valid &&
         sys->l2cache->last_evicted_line.dirty) {
          // Addr evit_L2_addr = sys->l2cache->last_evicted_line.tag;
          sys->l2cache->last_evicted_line.dirty = FALSE;
          sys->l2cache->last_evicted_line.valid = FALSE;
          Addr evit_L2_addr = sys->l2cache->last_evicted_line.tag;

          dram_access(sys->dram, evit_L2_addr, 1/*is_writeback*/);   
      }
    }
  }

  if (is_writeback == 1) { // dirty evicted line from dcache has come to L2
    outcome_L2 = cache_access(sys->l2cache, lineaddr, 1, core_id); // if line is present in L2 already which
                                                                  // which is stale.. you write-into that line
                                                                  // this is considered as hit and no eviction
                                                                  // from L2 would take place
    if (outcome_L2 == MISS) { // But if there's miss
      // Get the line from DRAM
      delay += dram_access(sys->dram, lineaddr, 0);
      // This is correct. Evicted entry is dirty.. it needs to write into L2 after
      // getting the stale-line from DRAM.
      // This is the case of 'Write-Allocate' & 'Write-Back'.
      cache_install(sys->l2cache, lineaddr, 1, core_id); // Eviction stat will be updated here
      // Due to this new install check if there is any dirty evicted entry that needs to be put
      // in DRAM
      if(sys->l2cache->last_evicted_line.valid &&
         sys->l2cache->last_evicted_line.dirty) {
          // Addr evit_L2_addr = sys->l2cache->last_evicted_line.tag;
          sys->l2cache->last_evicted_line.dirty = FALSE;
          sys->l2cache->last_evicted_line.valid = FALSE;
          Addr evit_L2_addr = sys->l2cache->last_evicted_line.tag;
          //  since line is dirty, put 'is_writeback' true
          is_writeback = TRUE;
          dram_access(sys->dram, evit_L2_addr, 1/*is_writeback*/);      
      }
    }
  }
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
  // printf(" At memsys.cpp::memsys_access_modeDE: %d Instn Type: %d core_id: %d\n", __LINE__, type, core_id);
  uns64 delay = 0;
  uns64 delay_c0=0;
  uns64 delay_c1=0;
  Addr p_lineaddr=0;
  Flag outcome_L1 = FALSE;
  Addr c0p_lineaddr, c1p_lineaddr;
  Flag needs_dcache_access_c0 = FALSE;  
  Flag needs_dcache_access_c1 = FALSE;  
  Flag is_write_c0 = FALSE;
  Flag is_write_c1 = FALSE;
	
  p_lineaddr = v_lineaddr;
  
  // TODO: First convert lineaddr from virtual (v) to physical (p) using the
  // function memsys_convert_vpn_to_pfn. Page size is defined to be 4KB.
  // NOTE: VPN_to_PFN operates at page granularity and returns page addr

  if (core_id == 0) { 
    Addr c0_vpn = v_lineaddr >> 12;
    Addr c0_pfn = memsys_convert_vpn_to_pfn(sys, c0_vpn, core_id);
    c0p_lineaddr = (c0_pfn << 12) | (0x00000fff & v_lineaddr);
    // printf("@cycle: %ld \t v_lineaddr: 0x%lx \t c0_vpn: 0x%lx \t c0_pfn: 0x%lx \t c0p_lineaddr: 0x%lx \n", 
      // cycle, v_lineaddr, c0_vpn, c0_pfn, c0p_lineaddr); 
  }

  if (core_id == 1) { 
    Addr c1_vpn = v_lineaddr >> 12;
    Addr c1_pfn = memsys_convert_vpn_to_pfn(sys, c1_vpn, core_id);
    c1p_lineaddr = c1_pfn << 12 | (0x00000fff & v_lineaddr);
    // printf("@cycle: %ld \t v_lineaddr: 0x%lx \t c1_vpn: 0x%lx \t c1_pfn: 0x%lx \t c1p_lineaddr: 0x%lx \n", 
      // cycle, v_lineaddr, c1_vpn, c1_pfn, c1p_lineaddr); 
  }

  assert(c0p_lineaddr != c1p_lineaddr);
  if(type == ACCESS_TYPE_IFETCH){
    // YOU NEED TO WRITE THIS PART AND UPDATE DELAY
    if (core_id == 0) {
      outcome_L1 = cache_access(sys->icache_coreid[0], c0p_lineaddr, 0, core_id); // core0 reading line form L1 icache

      delay_c0 = ICACHE_HIT_LATENCY; // updated delay for the read of core0

      if (outcome_L1 == MISS) {   // In case there is a miss, we will read from L2
        // printf("@Cycle: %d \t IFETCH-L1-MISS: core0\n", cycle);
        delay_c0 += memsys_L2_access_multicore(sys, c0p_lineaddr, 0, core_id); // put the delay of reading it from L2
        // Install the line in L1, no writeback
        cache_install(sys->icache_coreid[0], c0p_lineaddr, 0, core_id); // Install the line
      }
    }
    if (core_id == 1) {
      outcome_L1 = cache_access(sys->icache_coreid[1], c1p_lineaddr, 0, core_id); // Respective core reading line form L1 icache

      delay_c1 = ICACHE_HIT_LATENCY; // updated delay for the read

      if (outcome_L1 == MISS) {   // In case there is a miss, we will read from L2
        // printf("@Cycle: %d \t IFETCH-L1-MISS: core1\n", cycle);
        delay_c1 += memsys_L2_access_multicore(sys, c1p_lineaddr, 0, core_id); // put the delay of reading it from L2
        // Install the line in L1, no writeback
        cache_install(sys->icache_coreid[1], c1p_lineaddr, 0, core_id); // Install the line
      }
    }
  }

  if(type == ACCESS_TYPE_LOAD){
    // YOU NEED TO WRITE THIS PART AND UPDATE DELAY
    if (core_id == 0) {
      needs_dcache_access_c0 = TRUE;
      is_write_c0 = FALSE;
    }
    if (core_id == 1) {
      needs_dcache_access_c1 = TRUE;
      is_write_c1 = FALSE;
    }
  }
  
  if(type == ACCESS_TYPE_STORE){
    // YOU NEED TO WRITE THIS PART AND UPDATE DELAY
    if (core_id == 0) {
      needs_dcache_access_c0 = TRUE;
      is_write_c0 = TRUE;
    }
    if (core_id == 1) {
      needs_dcache_access_c1 = TRUE;
      is_write_c1 = TRUE;
    }
  }
  // Both CPUs can access this code but need to work on 
  // their own variable and structure
  if (needs_dcache_access_c0 || needs_dcache_access_c1) { // Both LD/ST would come here
    if(core_id == 0) {
      // Accessing L1 dcache(for reading/writing based on 'is_dirty') 
      outcome_L1 = cache_access(sys->dcache_coreid[0], c0p_lineaddr, is_write_c0, core_id); 

      delay_c0 = DCACHE_HIT_LATENCY; // initialized the delay for DCACHE access

      if(outcome_L1 == MISS) { // L1 cache miss
        // We are following 'non-inclusive' policy here.
        // read from L2
        // compensation for delay
        // printf("@Cycle: %d \t DCACHE-L1-MISS: core0 type: %d\n", cycle, type);
        delay_c0 +=memsys_L2_access_multicore(sys, c0p_lineaddr, 0/*is_dirty*/, core_id); // because it is write back cache.
                                                                         // we will only write back on dirty
                                                                         // eviction 
        // cache_install() function takes care of eviction_stat
        cache_install(sys->dcache_coreid[0], c0p_lineaddr, is_write_c0, core_id);
        // if the evicted line is not dirty.. we do not put it in L2
        if(sys->dcache_coreid[0]->last_evicted_line.dirty && 
           sys->dcache_coreid[0]->last_evicted_line.valid) {
          sys->dcache_coreid[0]->last_evicted_line.dirty = FALSE;
          sys->dcache_coreid[0]->last_evicted_line.valid = FALSE;
          Addr evit_L1_addr = sys->dcache_coreid[0]->last_evicted_line.tag;
          // we are writing back the evicted line to L2 which are 'Dirty': Write-back   
          memsys_L2_access_multicore(sys, evit_L1_addr, 1, core_id); 
        }
       
      }      
    }
    if(core_id == 1) {
      // Accessing L1 dcache(for reading/writing based on 'is_dirty') 
      outcome_L1 = cache_access(sys->dcache_coreid[1], c1p_lineaddr, is_write_c1, core_id); 

      delay_c1 = DCACHE_HIT_LATENCY; // initialized the delay for DCACHE access

      if(outcome_L1 == MISS) { // L1 cache miss
        // We are following 'non-inclusive' policy here.
        // read from L2
        // compensation for delay
        // printf("@Cycle: %d \t DCACHE-L1-MISS: core1 type: %d\n", cycle, type);

        delay_c1 +=memsys_L2_access_multicore(sys, c1p_lineaddr, 0/*is_dirty*/, core_id); // because it is write back cache.
                                                                         // we will only write back on dirty
                                                                         // eviction 
        // cache_install() function takes care of eviction_stat
        cache_install(sys->dcache_coreid[1], c1p_lineaddr, is_write_c1, core_id);
        // if the evicted line is not dirty.. we do not put it in L2
        if(sys->dcache_coreid[1]->last_evicted_line.dirty && 
           sys->dcache_coreid[1]->last_evicted_line.valid) {
          sys->dcache_coreid[1]->last_evicted_line.dirty = FALSE;
          sys->dcache_coreid[1]->last_evicted_line.valid = FALSE;
          Addr evit_L1_addr = sys->dcache_coreid[1]->last_evicted_line.tag;
          // we are writing back the evicted line to L2 which are 'Dirty': Write-back   
          memsys_L2_access_multicore(sys, evit_L1_addr, 1, core_id); 
        }
      }            
    }
  }


  if (core_id == 0) {
    delay = delay_c0;
  } else{
    delay = delay_c1;
  }
  return delay;
}


/////////////////////////////////////////////////////////////////////
// This function is called on ICACHE miss, DCACHE miss, DCACHE writeback
// ----- YOU NEED TO WRITE THIS FUNCTION AND UPDATE DELAY ----------
/////////////////////////////////////////////////////////////////////

uns64   memsys_L2_access_multicore(Memsys *sys, Addr lineaddr, Flag is_writeback, uns core_id){
	
  //To get the delay of L2 MISS, you must use the dram_access() function
  //To perform writebacks to memory, you must use the dram_access() function
  //This will help us track your memory reads and memory writes

  uns64 delay = L2CACHE_HIT_LATENCY; // initialized the delay with Hit latency
  Flag outcome_L2 = FALSE;

  // printf("@Cycle: %d \t memsys_L2_access_multicore() \t is_writeback: %d \t core_id: %d\n", cycle, is_writeback, core_id);
  if(is_writeback == 0) { //Reading <both for LD/ST> (requesting line in case of store as following WB strategy)
    // Read request from L1
    outcome_L2 = cache_access(sys->l2cache, lineaddr, 0, core_id); // Reading line from L2 in case of L1 miss
    if (outcome_L2 == MISS) { // If there is L2 miss on read

      // Delay for DRAM access
      // Reading the cache-line from DRAM (Get the line from DRAM)
      // printf("L2-MISS, get the line from DRAM\n");
      delay += dram_access(sys->dram, lineaddr, 0); 
      // cache_install() takes care of eviction stat 
      // printf("Installing the cache in L2\n");
      cache_install(sys->l2cache, lineaddr, 0, core_id); // Install the line into L2.. it is not dirty
      // If the evicted line is dirty you need to write it to DRAM, otherwise no action required
      if(sys->l2cache->last_evicted_line.valid &&
         sys->l2cache->last_evicted_line.dirty) {
          // Addr evit_L2_addr = sys->l2cache->last_evicted_line.tag;
          sys->l2cache->last_evicted_line.dirty = FALSE;
          sys->l2cache->last_evicted_line.valid = FALSE;
          Addr evit_L2_addr = sys->l2cache->last_evicted_line.tag;

          dram_access(sys->dram, evit_L2_addr, 1/*is_writeback*/);   
      }
    }
  }

  if (is_writeback == 1) { // dirty evicted line from dcache has come to L2
    outcome_L2 = cache_access(sys->l2cache, lineaddr, 1, core_id); // if line is present in L2 already which
                                                                  // which is stale.. you write-into that line
                                                                  // this is considered as hit and no eviction
                                                                  // from L2 would take place
    if (outcome_L2 == MISS) { // But if there's miss
      // printf("L2-MISS, get the line from DRAM\n");
      // Get the line from DRAM
      delay += dram_access(sys->dram, lineaddr, 0);
      // This is correct. Evicted entry is dirty.. it needs to write into L2 after
      // getting the stale-line from DRAM.
      // This is the case of 'Write-Allocate' & 'Write-Back'.
      // printf("Installing the cache in L2\n");
      cache_install(sys->l2cache, lineaddr, 1, core_id); // Eviction stat will be updated here
      // Due to this new install check if there is any dirty evicted entry that needs to be put
      // in DRAM
      if(sys->l2cache->last_evicted_line.valid &&
         sys->l2cache->last_evicted_line.dirty) {
          // Addr evit_L2_addr = sys->l2cache->last_evicted_line.tag;
          sys->l2cache->last_evicted_line.dirty = FALSE;
          sys->l2cache->last_evicted_line.valid = FALSE;
          Addr evit_L2_addr = sys->l2cache->last_evicted_line.tag;
          //  since line is dirty, put 'is_writeback' true
          is_writeback = TRUE;
          dram_access(sys->dram, evit_L2_addr, 1/*is_writeback*/);      
      }
    }
  }

  return delay;
}

