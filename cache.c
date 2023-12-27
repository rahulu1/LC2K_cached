/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <stdio.h>
#include <math.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

extern int mem_access(int addr, int write_flag, int write_data);
extern int get_num_mem_accesses(void);

enum actionType
{
    cacheToProcessor,
    processorToCache,
    memoryToCache,
    cacheToMemory,
    cacheToNowhere
};

typedef struct blockStruct
{
    int data[MAX_BLOCK_SIZE];
    int dirty;
    int lruLabel;
    int set;
    int tag;
    int filled; // once filled, never empty
} blockStruct;

typedef struct cacheStruct
{
    blockStruct blocks[MAX_CACHE_SIZE];
    int blockSize;
    int numSets;
    int blocksPerSet;
    int size;
    int hits;
    int misses;
    int writebacks;
    int dcb;
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;

void printAction(int, int, enum actionType);
void printCache(void);
int getSet(int addr);
int getTag(int addr);
int getOffset(int addr);
int getAddr(int block);

/*
 * Set up the cache with given command line parameters. This is 
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet){
    cache.blockSize = blockSize;
    cache.numSets = numSets;
    cache.blocksPerSet = blocksPerSet;
    cache.size = numSets * blocksPerSet;
    cache.hits = cache.misses = cache.writebacks = cache.dcb = 0;
    int  j = blocksPerSet - 1;
    for(int i = 0; i < cache.size; i++){
        cache.blocks[i].lruLabel = j;
        cache.blocks[i].dirty = 0;
        cache.blocks[i].set = 0;
        cache.blocks[i].tag = -1;
        cache.blocks[i].filled = 0;
        if(j == 0){
            j = blocksPerSet - 1;
        }
        else{
            j--;
        }
    }
    printf("Simulating a cache with %d total lines; each line has %d words\n", numSets*blocksPerSet, blockSize);
    printf("Each set in the cache contains %d lines; there are %d sets\n", blocksPerSet, numSets);
    return;
}

/*
 * Access the cache. This is the main part of the project,
 * and should call printAction as is appropriate.
 * It should only call mem_access when absolutely necessary.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads (fetch/lw) and 1 for writes (sw).
 * write_data is a word, and is only valid if write_flag is 1.
 * The return of mem_access is undefined if write_flag is 1.
 * Thus the return of cache_access is undefined if write_flag is 1.
 */
int cache_access(int addr, int write_flag, int write_data) {
    int hit = 0; // 1 if hit, 0 if miss.
    int currSet = getSet(addr); // set of addr to access
    int currTag = getTag(addr); // tag of addr to access
    int currBlock = 0; // block to access
    int currLRU = -1;
    
    // search for block by checking all tags in set
    for(int i = (currSet * cache.blocksPerSet); i < ((currSet+1) * cache.blocksPerSet); i++){
        if(cache.blocks[i].tag == currTag){
            hit = 1;
            currBlock = i;
            currLRU = cache.blocks[i].lruLabel;
            cache.blocks[i].lruLabel = -1;
            // don't need to keep searching if found
            break;
        }
    }
    
    if(hit){ // if hit
        cache.hits++;
        // increase LRUlabel of all blocks in set with lower LRU than currBlock
        for(int i = (currSet * cache.blocksPerSet); i < ((currSet+1) * cache.blocksPerSet); i++){
            if(cache.blocks[i].lruLabel < currLRU){
                cache.blocks[i].lruLabel++;
            }
        }
        if(write_flag){ // if writing to cache (sw)
            cache.blocks[currBlock].data[getOffset(addr)] = write_data; // write to correct addr
            cache.blocks[currBlock].dirty = 1; // set dirty for eviction
            cache.blocks[currBlock].filled = 1;
            cache.dcb++;
            printAction(addr, 1, processorToCache);
            return 0; // doesn't matter what you return for a sw
        }
        else{ // if reading from cache (fetch/lw)
            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[currBlock].data[getOffset(addr)];
        }
    }
    else{ // if miss
        // need to add block to cache. this loop selects the block
        // in the set with the highest LRU (first block if tie)
        cache.misses++;
        for(int i = (currSet * cache.blocksPerSet); i < ((currSet+1) * cache.blocksPerSet); i++){
            if(cache.blocks[i].lruLabel > currLRU){
                currLRU = cache.blocks[i].lruLabel;
                currBlock = i;
            }
            cache.blocks[i].lruLabel++;
        }
        
        // set block to be evicted to 0
        cache.blocks[currBlock].lruLabel = 0;
        
        // if dirty, write block back to mem
        if(cache.blocks[currBlock].filled){
            if(cache.blocks[currBlock].dirty){
                int wbStart = getAddr(currBlock);
                printAction(wbStart, cache.blockSize, cacheToMemory);
                cache.writebacks++;
                for(int i = 0; i < cache.blockSize; i++){
                    mem_access(wbStart+i, 1, cache.blocks[currBlock].data[i]);
                }
                cache.blocks[currBlock].dirty = 0;
                cache.dcb--;
            }
            else{
                int wbStart = getAddr(currBlock);
                printAction(wbStart, cache.blockSize, cacheToNowhere);
            }
        }
        
        int memStart = addr - getOffset(addr);
        
        // finds first address to import from memory
        
        cache.blocks[currBlock].tag = currTag;
        cache.blocks[currBlock].set = currSet;
        cache.blocks[currBlock].filled = 1;
        
        // write from memory to cache
        printAction(memStart, cache.blockSize, memoryToCache);
        for(int i = 0; i < cache.blockSize; i++){
            cache.blocks[currBlock].data[i] = mem_access(memStart+i, 0, 0);
        }
        
        if(write_flag){ // if writing to cache (sw)
            cache.blocks[currBlock].data[getOffset(addr)] = write_data; // write to correct addr
            cache.blocks[currBlock].dirty = 1; // set dirty for eviction
            cache.blocks[currBlock].filled = 1;
            cache.dcb++;
            printAction(addr, 1, processorToCache);
            return 0; // doesn't matter what you return for a sw
        }
        else{ // if reading from cache (fetch/lw)
            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[currBlock].data[getOffset(addr)];
        }
    }
    
    return mem_access(addr, write_flag, write_data);
}


/*
 * print end of run statistics like in the spec. This is not required,
 * but is very helpful in debugging.
 * This should be called once a halt is reached.
 * DO NOT delete this function, or else it won't compile.
 * DO NOT print $$$ in this function
 */
void printStats(void){
    printf("End of run statistics:\n");
    printf("hits %d, misses %d, writebacks %d\n", cache.hits, cache.misses, cache.writebacks);
    printf("%d dirty cache blocks left\n", cache.dcb);
    
    return;
}

/*
 * Log the specifics of each cache action.
 *
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -    cacheToProcessor: reading data from the cache to the processor
 *  -    processorToCache: writing data from the processor to the cache
 *  -    memoryToCache: reading data from the memory to the cache
 *  -    cacheToMemory: evicting cache data and writing it to the memory
 *  -    cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
    printf("$$$ transferring word [%d-%d] ", address, address + size - 1);

    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    }
    else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    }
    else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    }
    else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    }
    else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
}

/*
 * Prints the cache based on the configurations of the struct
 * This is for debugging only and is not graded, so you may
 * modify it, but that is not recommended.
 */
void printCache()
{
    printf("\ncache:\n");
    for (int set = 0; set < cache.numSets; ++set) {
        printf("\tset %i:\n", set);
        for (int block = 0; block < cache.blocksPerSet; ++block) {
            printf("\t\t[ %i ]: {", block);
            for (int index = 0; index < cache.blockSize; ++index) {
                printf(" %i", cache.blocks[set * cache.blocksPerSet + block].data[index]);
            }
            printf(" }\n");
        }
    }
    printf("end cache\n");
}

// returns which set this addr would belong to
int getSet(int addr){
    int setGet = 0;
    
    int blockBits = log2(cache.blockSize);
    int setMask = cache.numSets - 1;
    
    //gets rid of block bits
    
    setGet = addr >> blockBits;
    return (setGet & setMask);
    
}

int getTag(int addr){
    int tagGet = 0;
    
    int blockBits = log2(cache.blockSize);
    int setBits = log2(cache.numSets);
    
    //gets rid of block bits
    
    tagGet = addr >> (blockBits + setBits);
    return tagGet;
}

int getOffset(int addr){
    int blockMask = cache.blockSize - 1;
    
    return (addr & blockMask);
}

// returns start addr of a given block
int getAddr(int block){
    int blockBits = log2(cache.blockSize); // how many bits to rep offset
    int setBits = log2(cache.numSets); // how many bits to rep set
    int addr = cache.blocks[block].tag;
    
    addr = addr << (setBits + blockBits);
    addr += (cache.blocks[block].set << blockBits);
    
    return addr;
}
