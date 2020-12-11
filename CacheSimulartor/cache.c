//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"
#include<stdio.h>


#define ICACHE 0
#define DCACHE 1
#define L2CACHE 2


const char *studentName = "Zihao Kong";
const char *studentID   = "A15502295";
const char *email       = "zikong@ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//
typedef enum {false, true} bool;
struct {
  int name;
  uint32_t** tags;
  bool** status;
  uint32_t** LRU_bits;
  uint32_t asso;
  uint32_t hit_time;
  bool inclusive;

  int tag_bit;
  int index_bit;
  int data_offset;
} typedef CACHE;


CACHE icache;
CACHE dcache;
CACHE l2cache;

//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
void
init_cache()
{
  // Initialize cache stats
  icacheRefs        = 0;
  icacheMisses      = 0;
  icachePenalties   = 0;
  dcacheRefs        = 0;
  dcacheMisses      = 0;
  dcachePenalties   = 0;
  l2cacheRefs       = 0;
  l2cacheMisses     = 0;
  l2cachePenalties  = 0;
  
  int data_bit = 0;
  int temp = blocksize;
  while(temp != 1) {
    temp = temp >> 1;
    data_bit++;
  }


  icache.name = ICACHE;
  icache.tags = (uint32_t**) malloc(sizeof(uint32_t*)*icacheAssoc);
  icache.status = (bool**) malloc(sizeof(bool*)*icacheAssoc);
  icache.LRU_bits = (uint32_t**) malloc(sizeof(uint32_t*)*icacheAssoc);
  for(int i = 0; i < icacheAssoc;i++){
    icache.tags[i] = (uint32_t*) calloc(icacheSets,sizeof(uint32_t));
    icache.status[i] = (bool*) calloc(icacheSets,sizeof(bool));
    icache.LRU_bits[i] = (bool*) calloc(icacheSets,sizeof(uint32_t));
  }
  icache.asso = icacheAssoc;
  icache.inclusive = 0;
  icache.hit_time = icacheHitTime;
  icache.data_offset = data_bit;
  
  int index = 0;
  temp = icacheSets;
  if(temp != 0){
    while(temp!= 1){
      temp = temp >> 1;
      index++;
    }
  }
  icache.index_bit = index; 
  icache.tag_bit = sizeof(int)*8 - icache.index_bit - icache.data_offset;



  dcache.name = DCACHE;
  dcache.tags = (uint32_t**) malloc(sizeof(uint32_t*)*dcacheAssoc);
  dcache.status = (bool**) malloc(sizeof(bool*)*dcacheAssoc);
  dcache.LRU_bits = (uint32_t**) malloc(sizeof(uint32_t*)*dcacheAssoc);
  for(int i = 0; i < dcacheAssoc;i++){
    dcache.tags[i] = (uint32_t*) calloc(dcacheSets,sizeof(uint32_t));
    dcache.status[i] = (bool*) calloc(dcacheSets,sizeof(bool));
    dcache.LRU_bits[i] = (bool*) calloc(dcacheSets,sizeof(uint32_t));
  }
  dcache.asso = dcacheAssoc;
  dcache.inclusive = 0;
  dcache.hit_time = dcacheHitTime;

  dcache.data_offset = data_bit;
  
  index = 0;
  temp = dcacheSets;
  if(temp != 0){
    while(temp!= 1){
      temp = temp >> 1;
      index++;
    }
  }
  dcache.index_bit = index; 
  dcache.tag_bit = sizeof(int)*8 - dcache.index_bit - dcache.data_offset;



  l2cache.name = L2CACHE;
  l2cache.tags = (uint32_t**) malloc(sizeof(uint32_t*)*l2cacheAssoc);
  l2cache.status = (bool**) malloc(sizeof(bool*)*l2cacheAssoc);
  l2cache.LRU_bits = (uint32_t**) malloc(sizeof(uint32_t*)*l2cacheAssoc);
  for(int i = 0; i < l2cacheAssoc;i++){
    l2cache.tags[i] = (uint32_t*) calloc(l2cacheSets,sizeof(uint32_t));
    l2cache.status[i] = (bool*) calloc(l2cacheSets,sizeof(bool));
    l2cache.LRU_bits[i] = (bool*) calloc(l2cacheSets,sizeof(uint32_t));
  }
  l2cache.asso = l2cacheAssoc;
  l2cache.inclusive = inclusive;
  l2cache.hit_time = l2cacheHitTime;

  l2cache.data_offset = data_bit;
  
  index = 0;
  temp = l2cacheSets;
  if(temp != 0){
    while(temp!= 1){
      temp = temp >> 1;
      index++;
    }
  }
  l2cache.index_bit = index; 
  l2cache.tag_bit = sizeof(int)*8 - l2cache.index_bit - l2cache.data_offset;

}



void L1_cache_validate(uint32_t addr){

  uint32_t i_tag = addr >> (icache.data_offset + icache.index_bit);
  uint32_t i_index = addr % (1<< (icache.data_offset + icache.index_bit));
  i_index = i_index >> icache.data_offset;

  //found data in icache
  for(int i = 0; i < icache.asso;i++){
    if(icache.status[i][i_index] == true && icache.tags[i][i_index] == i_tag){
      icache.status[i][i_index] = 0;
      icache.tags[i][i_index] = 0;
      icache.LRU_bits[i][i_index] = 0;
    }
  }

  uint32_t d_tag = addr >> (dcache.data_offset + dcache.index_bit);
  uint32_t d_index = addr % (1<< (dcache.data_offset + dcache.index_bit));
  d_index = d_index >> dcache.data_offset;

  for(int i = 0; i < dcache.asso;i++){
    if(dcache.status[i][d_index] == true && dcache.tags[i][d_index] == d_tag){
      dcache.status[i][d_index] = 0;
      dcache.tags[i][d_index] = 0;
      dcache.LRU_bits[i][d_index] = 0;
    }
  }
}




uint32_t cache_access(CACHE* cache, uint32_t addr){
  uint32_t tag = addr >> ((cache->data_offset)+(cache->index_bit));
  uint32_t index = addr % (1<< ((cache->data_offset) + (cache->index_bit)));
  index = index >> cache->data_offset;

  uint32_t memtime = cache->hit_time;


  for(int i = 0;i < cache->asso;i++){
    if(cache->status[i][index] == true && cache->tags[i][index] == tag){
      cache->LRU_bits[i][index] = 0;

      if(cache->name == ICACHE){
        for(int i = 0;i<icache.asso;i++) cache->LRU_bits[i][index] +=1;
      } else if(cache->name == DCACHE){
        for(int i = 0;i<dcache.asso;i++) cache->LRU_bits[i][index] +=1;
      } else if(cache->name == L2CACHE){
        for(int i = 0;i<l2cache.asso;i++) cache->LRU_bits[i][index] +=1;
      }
      
      return memtime;
    }
  }


  // CACHEMISS

  //if the current cache is not L2 cache, access L2 cache.
  if(cache->name != L2CACHE){
    int extra_time = l2cache_access(addr);
    memtime += extra_time;
  }
  else{
    int extra_time = memspeed;
    memtime += extra_time;
  }

  uint32_t entry_num = 0;
  
  
  while(entry_num < (cache->asso)){
    if(cache->status[entry_num][index] == 1) entry_num++;
    else break;
  }
  // evict
  if(entry_num == cache->asso){

    int max=0, max_index=0;
    for(int i = 0;i < cache->asso;i++){
      if(cache->status[i][index] == 1 && cache->LRU_bits[i][index] > max){
        max = cache->LRU_bits[i][index];
        max_index = i;
      }
    }

    //inclusive policy
    if(cache->name == L2CACHE && cache->inclusive == 1){


      uint32_t evicted_tag = cache->tags[max_index][index];
      uint32_t evicted_index = index;
      uint32_t reconstructed = evicted_tag << (cache->data_offset + cache->index_bit);
      reconstructed = reconstructed | (evicted_index << cache->data_offset); 

      L1_cache_validate(reconstructed);
    }


    cache->tags[max_index][index] = tag;
    cache->LRU_bits[max_index][index] = 0;
  } else {

    cache->status[entry_num][index] = 1;
    cache->tags[entry_num][index] = tag;
    cache->LRU_bits[entry_num][index] = 0;
  }


  if(cache->name == ICACHE){
    for(int i = 0;i<icache.asso;i++) cache->LRU_bits[i][index] +=1;
  } else if(cache->name == DCACHE){
    for(int i = 0;i<dcache.asso;i++) cache->LRU_bits[i][index] +=1;
  } else if(cache->name == L2CACHE){
    for(int i = 0;i<l2cache.asso;i++) cache->LRU_bits[i][index] +=1;
  }

  return memtime;
}





// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
  uint32_t total_cycles;
  if(icache.index_bit != 0){
    icacheRefs++;
    total_cycles = cache_access(&icache, addr);
    
    if(total_cycles != icache.hit_time){
      icacheMisses++;
      icachePenalties += total_cycles - icache.hit_time;
    } 
  } else {
    total_cycles = l2cache_access(addr);
  }
  

  return total_cycles;
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
  uint32_t total_cycles;
  if(dcache.index_bit != 0){
    dcacheRefs++;
    total_cycles = cache_access(&dcache, addr);

    if(total_cycles != dcache.hit_time){
      dcacheMisses++;
      dcachePenalties += total_cycles - dcache.hit_time;
    } 
  } else{
    total_cycles = l2cache_access(addr);
  }

  
  return total_cycles;
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr)
{

  l2cacheRefs++;
  uint32_t total_cycles = cache_access(&l2cache, addr);
  if(total_cycles != l2cache.hit_time){
    l2cacheMisses++;
    l2cachePenalties += total_cycles - l2cache.hit_time;
  }
  return total_cycles;
}

