#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "cache.h"

extern uns64 SWP_CORE0_WAYS; // Static Way Partitioning
extern uns64 cycle; // You can use this as timestamp for LRU
extern uns64 CACHE_LINESIZE;
extern uns64 L2CACHE_SIZE;
extern uns64 L2CACHE_ASSOC;

////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE INIT FUNCTION -----------
////////////////////////////////////////////////////////////////////

Cache  *cache_new(uns64 size, uns64 assoc, uns64 linesize, uns64 repl_policy){

   Cache *c = (Cache *) calloc (1, sizeof (Cache));
   c->num_ways = assoc;
   c->repl_policy = repl_policy;

   if(c->num_ways > MAX_WAYS){
     printf("Change MAX_WAYS in cache.h to support %llu ways\n", c->num_ways);
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
    // printf("Cycle: %lu \t In: %s \t c->stat_write_access: %lu \n", cycle, __func__, c->stat_write_access);
  } else {
    c->stat_read_access++;
    // printf("Cycle: %lu \t In: %s \t c->stat_read_access: %lu \n", cycle, __func__, c->stat_read_access);
  }

  // Loop-over all the Cache-lines/ways in a given 'set_index'
  for (uns k=0; k<c->num_ways; k++) { // complete this for-loop
  // If cache_line is valid and core_id is same as the one mentioned in the parameter
    if ((c->sets[set_index].line[k].valid == true) && 
        (c->sets[set_index].line[k].core_id == core_id)) {
      // update the counters..
      if (c->sets[set_index].line[k].tag == lineaddr) {
        outcome = HIT;
        // Tushar: Only update the 'last_access_time' of a Cache-line if there's a hit
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
// Note: the system provides the cache with the line address
// Install the line: determine victim using repl policy (LRU/RAND)
// copy victim into last_evicted_line for tracking writebacks
////////////////////////////////////////////////////////////////////

void cache_install(Cache *c, Addr lineaddr, uns is_write, uns core_id){

  // Find victim using cache_find_victim
  // Initialize the evicted entry (c->last_evicted_line)
  // Initialize the victime entry

  // Tushar: lineaddr  assume that it is VA or PA whichever you want at this stage
  // lineaddr doesnot have your byte-offset.. basically lineaddr = (tag + index)
  // just to make things easy you can put your entire 'lineaddr' in tag

  unsigned set_index = lineaddr % c->num_sets;
  unsigned victim_index = c->num_ways;
  Flag L2_access = false;

  if((c->num_ways == L2CACHE_ASSOC) && ((c->num_ways*c->num_sets*CACHE_LINESIZE) == L2CACHE_SIZE)) {
    // printf("@Cycle: %d \t L2 Cache access\n", cycle);
    L2_access = true;
    // assert(0);
  }

  // check if space is avaliable in a given set_index

  if (L2_access) { // On L2 access
    if(SWP_CORE0_WAYS == 0) { // If there is no Static Way Partition
      for (uns i=0; i<c->num_ways; i++) { // Scan through the whole cache
        if (c->sets[set_index].line[i].valid == false) {
          victim_index = i;
          break;
        }
      }      
    } else if (SWP_CORE0_WAYS) { // Scan through cache per core basis
      if(core_id == 0) { // Core0
        for(uns i=0; i<SWP_CORE0_WAYS; i++) { // core0 will scan till SWP_CORE0_WAYS
          if (c->sets[set_index].line[i].valid == false) {
            victim_index = i;
            break;
          }
        }
      } else if (core_id == 1) { // Core1
        for(uns i=SWP_CORE0_WAYS; i<c->num_ways; i++) { // core1 will scan from SWP_CORE0_WAYS onwards..
          if (c->sets[set_index].line[i].valid == false) {
            victim_index = i;
            break;
          }        
        }
      }
    }
  } else { // If icache/dcache which are per core.. scan through the entire cache
           // there's no SWP_CORE0_WAYS restriction 
      for (uns i=0; i<c->num_ways; i++) { 
        if (c->sets[set_index].line[i].valid == false) {
          victim_index = i;
          break;
        }
      }
  }

  // At this point all the Cache-lines in the set are valid and we have to evict a line
  // to replace it with new one.
  if(victim_index == c->num_ways) { // Check this condition for PART: E.. would it be still the same?
                                    // yes, 'num_ways' is just a flag here which is used to check if 
                                    // 'victim_index' has been assigned any value.. it will not be assigned
                                    // 'c->num_ways' anyway. So if it's value is still 'c->num_ways' all cache
                                    // lines in their respective sets are filled.
    victim_index = cache_find_victim (c, set_index, core_id); // Got the addr of the victim(evicted-line)
    if (c->sets[set_index].line[victim_index].dirty) { //If line getting evicted is dirty
      // printf("c->sets[set_index].line[victim_index].core_id: %d\n", c->sets[set_index].line[victim_index].core_id);
      // printf("core_id: %d\n", core_id);
      // assert(c->sets[set_index].line[victim_index].core_id == core_id);
      c->stat_dirty_evicts++;
    // printf("Cycle: %lu \t In: %s \t c->stat_dirty_evicts: %lu \n", cycle, __func__, c->stat_dirty_evicts);
      // printf("@Cycle: %lu \t stat_dirty_evicts: %lu\n", cycle, c->stat_dirty_evicts);
    }

    // Initialize the evicted entry
    c->last_evicted_line = c->sets[set_index].line[victim_index];
  } else {
    c->last_evicted_line.valid = false;
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

////////////////////////////////////////////////////////////////////
// You may find it useful to split victim selection from install
////////////////////////////////////////////////////////////////////

// Check: Return the address of the victim to be evicted
// I think if should be the 'way' of the victim in the given
// set to be evicted... i.e the index of the line.
uns cache_find_victim(Cache *c, uns set_index, uns core_id){
	uns victim=0; 
  Flag L2_access = false;
  if((c->num_ways == L2CACHE_ASSOC) && ((c->num_ways*c->num_sets*CACHE_LINESIZE) == L2CACHE_SIZE)) {
    L2_access = true;
  }

  if (L2_access) { // On L2 access
    if(SWP_CORE0_WAYS == 0) { // If there is no Static Way Partition
      victim = find_replacement(c, set_index, core_id, L2_access);
    } else if (SWP_CORE0_WAYS) { // Scan through cache per core basis
      victim = find_replacement(c, set_index, core_id, L2_access);
    }
  } else { // If icache/dcache which are per core.. scan through the entire cache
           // there's no SWP_CORE0_WAYS restriction 
    victim = find_replacement(c, set_index, core_id, L2_access);
  }

	return victim;
}
// Find replacement
uns find_replacement(Cache *c, uns set_index, uns core_id, Flag L2_access) {
  uns victim=0;
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
    } else if (SWP_CORE0_WAYS) { // We are only following LRU in case of StaticWayPartition
      // your code goes heres
      if(core_id == 0) {
        uns64 smallest_cycle_count = cycle;
        // Find the LRU in core'0 alloted ways in the given set.
        for(uns i=0; i<SWP_CORE0_WAYS; i++) {
          if (smallest_cycle_count > c->sets[set_index].line[i].last_access_time) {
            smallest_cycle_count = c->sets[set_index].line[i].last_access_time;
            victim = i;
          }        
        }
      } else if(core_id == 1) {
        uns64 smallest_cycle_count = cycle;
        // Find the LRU in core1's alloted ways in the given set.
        for(uns i=SWP_CORE0_WAYS; i<c->num_ways; i++) {
          if (smallest_cycle_count > c->sets[set_index].line[i].last_access_time) {
            smallest_cycle_count = c->sets[set_index].line[i].last_access_time;
            victim = i;
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

