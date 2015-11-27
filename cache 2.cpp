#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "cache.h"

extern MODE SIM_MODE;
extern uns64  L2CACHE_REPL;
extern uns64 SWP_CORE0_WAYS; // Not so..Static Way Partitioning
extern uns64 cycle; // You can use this as timestamp for LRU
extern uns64 CACHE_LINESIZE; // parameters added to uniquely 
extern uns64 L2CACHE_SIZE;   // identify L2 cache from private 
extern uns64 L2CACHE_ASSOC;  // L1 of both cores 
extern uns64 utl_cnt[16][2];

////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE INIT FUNCTION -----------
////////////////////////////////////////////////////////////////////

Cache  *cache_new(uns64 size, uns64 assoc, uns64 linesize, uns64 repl_policy){

   Cache *c = (Cache *) calloc (1, sizeof (Cache));
   c->num_ways = assoc;
   c->repl_policy = repl_policy;

   if(c->num_ways > MAX_WAYS){
     // printf("Change MAX_WAYS in cache.h to support %llu ways\n", c->num_ways);
     exit(-1);
   }

   // determine num sets, and init the cache
   c->num_sets = size/(linesize*assoc);
   c->sets  = (Cache_Set *) calloc (c->num_sets, sizeof(Cache_Set));

   return c;
}

////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE PRINT STATS FUNCTION -----------
////////////////////////////////////////////////////////////////////

void    cache_print_stats    (Cache *c, char *header){
  double read_mr =0;
  double write_mr =0;

  if(c->stat_read_access){
    read_mr=(double)(c->stat_read_miss)/(double)(c->stat_read_access);
  }

  if(c->stat_write_access){
    write_mr=(double)(c->stat_write_miss)/(double)(c->stat_write_access);
  }

  printf("\n%s_READ_ACCESS    \t\t : %10llu", header, c->stat_read_access);
  printf("\n%s_WRITE_ACCESS   \t\t : %10llu", header, c->stat_write_access);
  printf("\n%s_READ_MISS      \t\t : %10llu", header, c->stat_read_miss);
  printf("\n%s_WRITE_MISS     \t\t : %10llu", header, c->stat_write_miss);
  printf("\n%s_READ_MISS_PERC  \t\t : %10.3f", header, 100*read_mr);
  printf("\n%s_WRITE_MISS_PERC \t\t : %10.3f", header, 100*write_mr);
  printf("\n%s_DIRTY_EVICTS   \t\t : %10llu", header, c->stat_dirty_evicts);

  printf("\n");
}



////////////////////////////////////////////////////////////////////
// Note: the system provides the cache with the line address
// Return HIT if access hits in the cache, MISS otherwise 
// Also if is_write is TRUE, then mark the resident line as dirty
// Update appropriate stats
////////////////////////////////////////////////////////////////////

Flag cache_access(Cache *c, Addr lineaddr, uns is_write, uns core_id){
  // Check: Tushar:'core_id' will be used in both cache_access and cache_install..
  // Your Code Goes Here
  // printf("c->num_sets: %d \t c->num_ways: %d \t SWP_CORE0_WAYS: %d \n", )
  Flag outcome=MISS; // Default value is MISS
  // First calculate the set_index
  unsigned set_index = lineaddr % c->num_sets;
  // update the stats, based on if it's a read_access or write_access
  if (is_write) {
    c->stat_write_access++;
    // printf("Cycle: %lu \t In: %s \t c->stat_write_access: %lu \n", __LINE__, cycle, __func__, c->stat_write_access);
  } else {
    c->stat_read_access++;
    // printf("Cycle: %lu \t In: %s \t c->stat_read_access: %lu \n", __LINE__, cycle, __func__, c->stat_read_access);
  }

  // Loop-over all the Cache-lines/ways in a given 'set_index'
  for (uns k=0; k<c->num_ways; k++) { // complete this for-loop
  // If cache_line is valid and core_id is same as the one mentioned in the parameter
    if ((c->sets[set_index].line[k].valid == true) && 
        (c->sets[set_index].line[k].core_id == core_id)) {
      // update the counters..
      if (c->sets[set_index].line[k].tag == lineaddr) {
        outcome = HIT;
        c->sets[set_index].line[k].last_access_time = cycle;
        if (is_write) {
          c->sets[set_index].line[k].dirty = true;
        }
      } 
    }
  }
  
  // Your access to the cache is complete.. update the stats if it's a hit or miss
  if ((outcome == MISS) && is_write) {
    c->stat_write_miss++;
  }
  if ((outcome == MISS) && !is_write) {
    c->stat_read_miss++;
  }
  return outcome;
}

////////////////////////////////////////////////////////////////////
// Utility Based Cache Partition: check_umon()
////////////////////////////////////////////////////////////////////
Flag check_umon(Cache *c, Addr lineaddr, uns is_write, uns core_id){
  assert(c->num_sets == 64);
  Flag outcome=MISS;
  uns umon_set_index;
  int pos=0;
  uns set_index = lineaddr % (L2CACHE_SIZE/(CACHE_LINESIZE*L2CACHE_ASSOC));
  // printf("[@Cycle: %d]set_index: %d\n", cycle, set_index);
  if (set_index%16==0)
  {
    umon_set_index=set_index/16;
  } else {
    return HIT; // we will not check if set_index is not multiple of 16
  }
  // Now check if it's a hit or miss in the given set and increment
  // the counter associated with it accordingly
  for (uns i = 0; i < c->num_ways; ++i)
  {
    if ((c->sets[umon_set_index].line[i].valid == true) && 
        (c->sets[umon_set_index].line[i].core_id == core_id))
    {
      if (c->sets[umon_set_index].line[i].tag == lineaddr) {
        outcome = HIT;
        // Before updating my 'last_access_time', I just need to find how many 'ways'
        // in my set, has got more cycle time then me, that would be my indexof the
        // counter to be updated
        for (uns k = 0; k < c->num_ways; ++k)
        {
          if (c->sets[umon_set_index].line[i].last_access_time < 
            c->sets[umon_set_index].line[k].last_access_time)
          {
            pos++;
          }
        } 
        utl_cnt[pos][core_id]++; // updating counter on a hit
        c->sets[umon_set_index].line[i].last_access_time = cycle;
      }
    }
  }
  // if (outcome == HIT)
  // {  printf("@Cycle: %d utility counters for core: %d\n", cycle, core_id);
  //   for (int i = 0; i < c->num_ways; ++i)
  //   {
  //     printf("%d \t", utl_cnt[i][core_id]);
  //   }
  // }
  return outcome;
}


////////////////////////////////////////////////////////////////////
// Note: the system provides the cache with the line address
// Install the line: determine victim using repl policy (LRU/RAND)
// copy victim into last_evicted_line for tracking writebacks
////////////////////////////////////////////////////////////////////

void cache_install(Cache *c, Addr lineaddr, uns is_write, uns core_id){

  // Find victim using cache_find_victim
  // Initialize the evicted entry (c->last_evicted_line)
  // Initialize the victime entry

  unsigned set_index = lineaddr % c->num_sets;
  unsigned victim_index = c->num_ways;
  Flag L2_access = false;

  if((c->num_ways == L2CACHE_ASSOC) && ((c->num_ways*c->num_sets*CACHE_LINESIZE) == L2CACHE_SIZE)) {
    // printf("[line: %d]@Cycle: %d \t L2 Cache access\n", __LINE__, cycle);
    L2_access = true;
  }

  // At this point all the Cache-lines in the set are valid and we have to evict a line
  // to replace it with new one.
  victim_index = cache_find_victim (c, set_index, core_id);
  if(c->sets[set_index].line[victim_index].valid) { // if valid is true, it means cache is completely
                                                    // filled. and we have applied LRU policy for replacement
    if (c->sets[set_index].line[victim_index].dirty) { //If line getting evicted is dirty
      // printf("c->sets[set_index].line[victim_index].core_id: %d\n", c->sets[set_index].line[victim_index].core_id);
      c->stat_dirty_evicts++;
      // printf("[line: %d]@Cycle: %lu \t In: %s \t c->stat_dirty_evicts: %lu \n", __LINE__, cycle, __func__, c->stat_dirty_evicts);
    }

    // Initialize the evicted entry
    c->last_evicted_line = c->sets[set_index].line[victim_index];
  } else {
    c->last_evicted_line.valid = false; // there will not be evicted line in this case as
                                        // control comes here when there is invalid entry in cache
  }

  assert (victim_index != c->num_ways); // This should not happen..

  // Now insert the new line..
  c->sets[set_index].line[victim_index].valid = TRUE;
  // Initialize the victim entry
  c->sets[set_index].line[victim_index].tag = lineaddr;
  c->sets[set_index].line[victim_index].dirty = is_write; // check if it's correct.
  // Update the stats
  c->sets[set_index].line[victim_index].last_access_time = cycle;
  // update core-id as well
  c->sets[set_index].line[victim_index].core_id = core_id;

}
//////////////////////////////////////////////////////////////////////
// Utility Based Cache Partitioning: umon_install()
//////////////////////////////////////////////////////////////////////

void umon_install(Cache *c, Addr lineaddr, uns is_write, uns core_id){

  // Find victim using cache_find_victim
  // Initialize the evicted entry (c->last_evicted_line)
  // Initialize the victime entry
  assert(c->num_sets == 64);
  uns umon_set_index;
  uns umon_victim = c->num_ways;
  // Note: we will only check for 0, 16, 32, .. sets
  // umon_install() is not a 'hit-case' so don't increment
  // the hit-counter
  uns set_index = lineaddr % (L2CACHE_SIZE/(CACHE_LINESIZE*L2CACHE_ASSOC));
  // 'set_index' should be in accordance with L2CACHE set index
  if (set_index%16==0)
  {
    umon_set_index=set_index/16;
    // assert(0);
  } else {
    return; // We will install line if set-index is multiple of 16
  }
  // find victim for given umon index
  for (uns i = 0; i < c->num_ways; ++i)
  {
    if (c->sets[umon_set_index].line[i].valid == false)
    {
      umon_victim = i;
      // who set the validbit to false initially!?
      break;
    }
  }
  // If there is no invalid line to be evited, do replacement, LRU style
  if (umon_victim == c->num_ways)
  {
    uns smallest_cycle_count = cycle; 
    // Looping over all the ways to find the minimum time
    for (uns i=0; i<c->num_ways; i++) {
      if (smallest_cycle_count > c->sets[umon_set_index].line[i].last_access_time) {
        smallest_cycle_count = c->sets[umon_set_index].line[i].last_access_time;
        umon_victim = i;
      }
    }    
  }
  assert (umon_victim != c->num_ways); // This should not happen..
  // At this point we have got the victim to be replaced
  // Install the line
  c->sets[umon_set_index].line[umon_victim].valid = TRUE;
  // Initialize the victim entry
  c->sets[umon_set_index].line[umon_victim].tag = lineaddr;
  c->sets[umon_set_index].line[umon_victim].dirty = is_write; // check if it's correct.
  // Update the stats
  c->sets[umon_set_index].line[umon_victim].last_access_time = cycle;
  // update core-id as well
  c->sets[umon_set_index].line[umon_victim].core_id = core_id;

  // // sanity
  // uns count=0;
  // for (uns i = 0; i < c->num_ways; ++i)
  // {
  //   if (c->sets[umon_set_index].line[i].valid == true)
  //   {
  //     count++;
  //   }
  // }
  // if (count)
  // {
  //   printf("[Line: %d; @Cycle: %d]In umon_install(): set-index: %d \n", __LINE__, cycle, umon_set_index);
  //   printf("[Line: %d; @Cycle: %d]In umon_install(): Valid lines in UMON for this set is: %d; core_id: %d\n", __LINE__, cycle, count, core_id);
  // }
}



////////////////////////////////////////////////////////////////////
// You may find it useful to split victim selection from install
////////////////////////////////////////////////////////////////////

uns cache_find_victim(Cache *c, uns set_index, uns core_id){
	uns victim = c->num_ways; // Done this to check if there's invalid entry
                            // present in cache, in which case it will give
                            // index of that invalid entry
  Flag L2_access = false;

  if((c->num_ways == L2CACHE_ASSOC) && ((c->num_ways*c->num_sets*CACHE_LINESIZE) == L2CACHE_SIZE)) {
    L2_access = true;
  }
  // if it access L2 repl = 2

  // check if space is avaliable in a given set_index
  if (L2_access) { // On L2 access
    if(SWP_CORE0_WAYS == 0) { // If there is no Static Way Partition
      for (uns i=0; i<c->num_ways; i++) { // Scan through the whole cache
        if (c->sets[set_index].line[i].valid == false) {
          victim = i;
          break;
        }
      }      
    // this is wrong as now each core entry can go anywhere if there's space in cache
    } else if (SWP_CORE0_WAYS) { // Scan through whole cache
      for(uns i=0; i<c->num_ways; i++) {
        if (c->sets[set_index].line[i].valid == false) {
          // printf("[line: %d]@Cycle: %d in cache_install() empty L2 cache for core 1\n", __LINE__, cycle);
          victim = i;
          break;
        }        
      }
    }
  } else { // If icache/dcache which are per core.. scan through the entire cache
           // there's no SWP_CORE0_WAYS restriction 
    for (uns i=0; i<c->num_ways; i++) { 
      if (c->sets[set_index].line[i].valid == false) {
          // printf("[line: %d]@Cycle: %d in cache_install() empty L1 cache for core 0\n", __LINE__, cycle);
        victim = i;
        break;
      }
    }
  }  

  if (victim == c->num_ways) { // It means no entry is invalid cache is 
                               // completely filled
    if (L2_access) { // On L2 access
      if(SWP_CORE0_WAYS == 0) { // If there is no Static Way Partition
        victim = find_replacement(c, set_index, core_id, L2_access);
      } else if (SWP_CORE0_WAYS) { // Scan through cache per core basis
        victim = find_replacement(c, set_index, core_id, L2_access);
        // printf("[line: %d]@Cycle: %d [L2 Cache]replacing line: %d from set: %d of core: %d\n", __LINE__, cycle, victim, set_index, core_id);
      }
    } else { // If icache/dcache which are per core.. scan through the entire cache
             // there's no SWP_CORE0_WAYS restriction 
      victim = find_replacement(c, set_index, core_id, L2_access);
        // printf("[line: %d]@Cycle: %d [L1 Cache]replacing line: %d from set: %d of core: %d\n", __LINE__, cycle, victim, set_index, core_id);
    }
  }
	return victim;
}
////////////////////////////////////////////////////////////////////
// Find replacement: Common code called by cache_find_victim() function
////////////////////////////////////////////////////////////////////
uns find_replacement(Cache *c, uns set_index, uns core_id, Flag L2_access) {
  uns victim=0;
  uns C0_L2_CNT = 0; // this should count number of ways core0 occupying in L2

  // samity check: No invalid entry come to this function
  for(uns i=0; i< c->num_ways; i++) {
    assert(c->sets[set_index].line[i].valid); // No line is invalid here
  }
  if(L2_access) {
      // Implement replacement policies
    if(SWP_CORE0_WAYS == 0) {
      if (c->repl_policy) { // Random replacement policy
        victim = rand() % (c->num_ways);
      } else { // LRU
        uns smallest_cycle_count = cycle; 
        // Looping over all the ways to find the minimum time
        for (uns i=0; i<c->num_ways; i++) {
          if (smallest_cycle_count > c->sets[set_index].line[i].last_access_time) {
            smallest_cycle_count = c->sets[set_index].line[i].last_access_time;
            victim = i;
          }
        }
      }
      // If control coming to this function then L2 cache is full.. whoever came earlier
      // has occupied the cache line
    } else if (SWP_CORE0_WAYS) { // We are only following LRU in case of StaticWayPartition
      // Everytime you come here you want to initialize the counter and based on it's value
      // you would like to take the decision
      for (uns i = 0; i < c->num_ways; ++i) {
        if(c->sets[set_index].line[i].core_id == 0) {
          C0_L2_CNT++; // number of ways core0 having
        }
      }
      // printf("[@Cycle: %d; Line: %d] In find_replacement()\n", cycle, __LINE__ );
      // printf("C0_L2_CNT: %d\n", C0_L2_CNT);
      if (C0_L2_CNT > SWP_CORE0_WAYS) // Core0 has more ways
      {          
      // printf("[@Cycle: %d; Line: %d] In find_replacement()\n", cycle, __LINE__ );
        uns64 smallest_cycle_count = cycle;
        // Evict the LRU line from Core0
        // find it in the whole cache
        for (uns i = 0; i < c->num_ways; ++i)
        {
          if (c->sets[set_index].line[i].core_id == 0)
          {
            if (smallest_cycle_count > c->sets[set_index].line[i].last_access_time) {
              smallest_cycle_count = c->sets[set_index].line[i].last_access_time;
              victim = i;
            }                
          }
        }
      } else if (C0_L2_CNT < SWP_CORE0_WAYS) // Core0 has less ways
      {
      // printf("[@Cycle: %d; Line: %d] In find_replacement()\n", cycle, __LINE__ );
        uns64 smallest_cycle_count = cycle;
        // Evict the LRU line from Core1
        // find it in the whole cache
        for (uns i = 0; i < c->num_ways; ++i)
        {
          if (c->sets[set_index].line[i].core_id == 1)
          {
            if (smallest_cycle_count > c->sets[set_index].line[i].last_access_time) {
              smallest_cycle_count = c->sets[set_index].line[i].last_access_time;
              victim = i;
            }                
          }
        }       
      } else if (C0_L2_CNT == SWP_CORE0_WAYS) //Core0 has alloted lines
      {
      // printf("[@Cycle: %d; Line: %d] In find_replacement()\n", cycle, __LINE__ );
        uns64 smallest_cycle_count = cycle;
        // Evict the LRU line from Core1
        // find it in the whole cache
        for (uns i = 0; i < c->num_ways; ++i)
        {
          if (c->sets[set_index].line[i].core_id == core_id)
          {
            if (smallest_cycle_count > c->sets[set_index].line[i].last_access_time) {
              smallest_cycle_count = c->sets[set_index].line[i].last_access_time;
              victim = i;
            }                
          }
        }
      }
    }
  } else { // Access to private icache/dcache.. 
    if (c->repl_policy) { // Random replacement policy
      victim = rand() % (c->num_ways);
    } else { // LRU
      uns smallest_cycle_count = cycle; 
      // Looping over all the ways to find the minimum time
      for (uns i=0; i<c->num_ways; i++) {
        if (smallest_cycle_count > c->sets[set_index].line[i].last_access_time) {
          smallest_cycle_count = c->sets[set_index].line[i].last_access_time;
          victim = i;
        }
      }
    }
  }
  return victim;
}

