/**
 * @author hongh233
 * @description: This C program will implement a cache module that simulates a cache.
 * The cache I choose is a fully associative cache, and the size of each block is 64 bytes.
 * The cache will work on a fast memory.
 */

#include "cache.h"
#include <math.h>

/* typedef struct cache_base, represent a cache information part contains metadata and pointer to set array
 * @params: unsigned char initialized: the initialized status represent whether the cache has been initialized
 * @params: struct cache_set * cacheSetArray: a pointer point to set array
 */
typedef struct cache_base {
    unsigned char initialized;
    struct cache_set * cacheSetArray;
} cache_base;

/* typedef struct cache_set, represent a set that contain some lines
 * @params: struct cache_line * cacheLineArray: a pointer point to cache line array
 */
typedef struct cache_set {
    struct cache_line * cacheLineArray;
} cache_set;

/* typedef struct cache_line, represent one element of the line array, contain metadata and blocks
 * @params: unsigned int time: a time counter which used for LRU step
 * @params: unsigned char valid: the valid bit represent whether the line has been used
 * @params: unsigned int tag: the unique identifier for each lines
 * @params: unsigned char cacheBlock[64]: a place where we store data in the cache
 */
typedef struct cache_line {
    unsigned int time;
    unsigned char valid;
    unsigned int tag;
    unsigned char cacheBlock[64]; // in this architecture, we use 64 bytes in a single block
} cache_line;


/* void function, divide the address into tag and offset, according to the size of the block
 * @params: unsigned long address: the address that we want to divide
 * @params: unsigned long * offset: the offset value we want to get
 * @params: unsigned long * tag: the tag value we want to get
 * @return: none
 */
static void address_decomposer(unsigned long address, unsigned long * offset, unsigned long * tag) {

    // compute the size of a single block (we will not access (cache_line*)0, just for size computation)
    unsigned int sizeOfBlock = sizeof(((cache_line*)0)->cacheBlock);

    // compute the offsetBit according to the block size
    unsigned char offsetBit = floor(log2(sizeOfBlock));

    // compute the offset mask to get offset value from address
    unsigned long offsetMask = ~(0xffffffffffffffff << offsetBit);

    *offset = address & offsetMask; // get the offset value from address
    *tag = address >> offsetBit;    // right shift to get the tag value from address
}


/* void function, store a numOfElem-length data into valueTemp start from a specified location
 * @params: unsigned char valueTemp[8]: the value array we will store value into it
 * @params: unsigned char * block: the source block we want to take value from
 * @params: unsigned int numOfBlock: the number of elements we want to store into the valueTemp
 * @params: unsigned int startLocation: the start location where we want to store value to the valueTemp
 * @return: none
 */
static void cache_get_byElem(unsigned char valueTemp[8], const unsigned char * block,
                             unsigned int numOfElem, unsigned int startLocation) {

    // iteratively store element from block to valueTemp start from startLocation
    for (int i = 0; i < numOfElem; i++) {
        valueTemp[i + startLocation] = block[i];
    }
}


/* unsigned long function, reverse the endian order of an unsigned long value which stored in
 * an unsigned char array, convert it to an unsigned long value and return
 * @params: unsigned char valueTemp[8]: the value stored in array which we want to reverse the order
 * @return: the value with reversed-order endian of the valueTemp
 */
static unsigned long reverse_endian(const unsigned char valueTemp[8]) {

    unsigned long value = 0;  // the value we will return, initialize it to 0

    // iteratively move element from valueTemp to the value in reverse endian
    for (int i = 0; i < 8; i++) {
        value = (value << 8) | valueTemp[8 - i - 1];
    }
    return value;
}


/* cache_line * function, find evict line by using LRU (least recently used) strategy:
 * take the line with the biggest time as evict line, add 1 to the time from all the other lines,
 * set the time of the evict line to be 0, update the tag with the given tag
 * @params: cache_set * set: the reference to our using set
 * @params: unsigned long tag: the new tag that we want to assign to the evict line
 * @params: unsigned int numOfLines: the number of cache lines depend on the size of the fast memory
 * @return: the evict line that we have to find
 */
static cache_line * findEvict(cache_set * set, unsigned long tag, unsigned int numOfLines) {

    struct cache_line *evictedLine;  // the evicted line that we have to find

    // iteratively find the evicted line with the biggest time
    for (int i = 0; i < numOfLines; i++) {
        if (set->cacheLineArray[i].time == (numOfLines - 1)) {
            evictedLine = &set->cacheLineArray[i];
        }
    }

    // iteratively add 1 to time from all the other lines
    for (int i = 0; i < numOfLines; i++) {
        if (set->cacheLineArray[i].time < evictedLine->time) {
            set->cacheLineArray[i].time++;
        }
    }

    evictedLine->tag = tag;  // update the tag
    evictedLine->valid = 1;  // update the valid
    evictedLine->time = 0;   // set time to 0, which is the most recently used

    return evictedLine;
}


/* void function, doing LRU (least recently used) step for the hit line:
 * add 1 to time from all the lines that less than this line's time,
 * set the line's time to be 0
 * @params: cache_set * set: the reference to our using set
 * @params: cache_line * line: the hit line
 * @params: unsigned int numOfLines: the number of cache lines depend on the size of the fast memory
 * @return: none
 */
static void setLRU(cache_set * set, cache_line * line, unsigned int numOfLines) {

    // iteratively add 1 to time from all the other lines that less than this line's time
    for (int i = 0; i < numOfLines; i++) {
        if (set->cacheLineArray[i].time < line->time) {
            set->cacheLineArray[i].time++;
        }
    }

    line->time = 0;  // set the line's time to be 0
}


/* void function, initialize the cache, set up all the pointers and structures,
 * include the cache base, cache set and cache line
 * @params: none
 * @return: none
 */
static void init() {

    // the number of cache lines depend on the size of the fast memory
    unsigned int numOfLines = (c_info.F_size - sizeof(cache_base) - sizeof(cache_set)) / sizeof(cache_line);

    /* initialization of the cache_base: create a pointer point to the start of the fast memory,
     * set initialized flag to 1 which means the cache has been initialized, set the cache set array
     * at the end of the cache base address
     */
    struct cache_base * cacheBase = c_info.F_memory;
    cacheBase->initialized = 1;
    cacheBase->cacheSetArray = (struct cache_set *) ((char *) cacheBase + sizeof(cache_base));

    /* initialization of the cache_set: create a pointer point to the start of the cache set array,
     * set the cache line array at the end of the cache set address
     */
    struct cache_set * cacheSet = &(cacheBase->cacheSetArray[0]);
    cacheSet->cacheLineArray = (struct cache_line *) ((char *) cacheBase + sizeof(cache_base) + sizeof(cache_set));

    /* iteratively initialize each cache line: create a pointer point to the corresponding line address,
     * initialize valid and tag to be 0 and set time increase 1 in order (for LRU)
     */
    for (int j = 0; j < numOfLines; j++) {
        struct cache_line * cacheLine = &(cacheSet->cacheLineArray[j]);
        cacheLine->time = j;
        cacheLine->valid = 0;
        cacheLine->tag = 0;
    }
}


/* int function, takes a memory address and a pointer to a value and loads a word
 * located at memory address and copies it into the location pointed to by value
 * @params: unsigned long address: the location of the value to be loaded.
 *          Addresses are the memory references from the reference stream.
 * @params: unsigned long *value: a pointer to a buffer of where the word is to be copied into
 * @return: 1 on success and 0 on failure
 */
extern int cache_get(unsigned long address, unsigned long *value) {

    // the number of cache lines depend on the size of the fast memory
    unsigned int numOfLines = (c_info.F_size - sizeof(cache_base) - sizeof(cache_set)) / sizeof(cache_line);

    // a char array temporarily hold the unsigned long value we want to return in reverse order
    unsigned char valueTemp[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    // create the cacheBase, which contains an initialized flag and pointer to set array
    cache_base * cacheBase = (cache_base *)c_info.F_memory;

    // check if the cache system has been initialized, if not, initialize the cache
    if (!cacheBase->initialized) {
        init();
    }


    // break up the address into tag and offset
    unsigned long offset;   // offset of the address
    unsigned long tag;      // tag of the address
    address_decomposer(address, &offset, &tag);

    // get the set we will use in the cache (there is only one set in our cache)
    cache_set * set = &(cacheBase->cacheSetArray[0]);

    // compute the size of a single block (we will not access (cache_line*)0, just for size computation)
    unsigned int sizeOfBlock = sizeof(((cache_line*)0)->cacheBlock);

    /* if the offset is not in the last seven elements of the block, check the hit and miss as usual,
     * otherwise, the data will cross two lines of the cache, in this case we will check differently
     */
    if (offset + 8 <= sizeOfBlock) {

        /* iteratively use the tag to determine if block of memory
         * that includes the address is in one of the lines in the set
         */
        for (int i = 0; i < numOfLines; i++) {
            cache_line * line = &(set->cacheLineArray[i]);

            // if the tag match and valid bit is 1, there is a cache hit
            if (line->valid && line->tag == tag) {

                // update all the line's time according to LRU rule
                setLRU(set, line, numOfLines);
                // copy the word to valueTemp, the offset is used to locate the word in current line
                cache_get_byElem(valueTemp, line->cacheBlock + offset, 8, 0);
                // reverse the order of valueTemp and copy it to the value
                *value = reverse_endian(valueTemp);
                return 1;
            }
        }

        /* if we didn't find any line hit, there's a cache miss
         * find an evictedLine according to the LRU rules
         */
        cache_line *evictedLine = findEvict(set, tag, numOfLines);

        /* if successfully load data from memory to cache, store value to the valueTemp,
         * reverse the endian order, assign the value to *value, and return 1
         * otherwise, return 0 since we fail to find the value
         */
        if (memget(address - offset, evictedLine->cacheBlock, 64)) {

            // copy the word to valueTemp, the offset is used to locate the word in current line
            cache_get_byElem(valueTemp, evictedLine->cacheBlock + offset, 8, 0);
            // reverse the order of valueTemp and copy it to the value
            *value = reverse_endian(valueTemp);
            return 1;

        } else {
            return 0;
        }



    /* if the offset is the last seven elements of the block, check hit and miss in both lines of the cache
     * In the last of the line1, it stores the first several elements of the value
     * In the start of the line2, it stores the last several elements of the value
     */
    } else {
        unsigned char isHitLine1 = 0;  // the hit flag represent whether line1 is hit
        unsigned char isHitLine2 = 0;  // the hit flag represent whether line2 is hit

        unsigned long newAddress = address + (sizeOfBlock - offset);  // the expected line2 address

        // break up the new address into tag and offset
        unsigned long newOffset = 0;    // offset of the new address
        unsigned long newTag = 0;       // tag of the new address
        address_decomposer(newAddress, &newOffset, &newTag);

        // check whether line1 is hit, if hit, update part of the value (from line1) to the valueTemp
        for (int i = 0; i < numOfLines; i++) {
            cache_line * line = &(set->cacheLineArray[i]);
            if (line->valid && line->tag == tag) {

                // update all the line's time according to LRU rule
                setLRU(set, line, numOfLines);

                /* copy part of the word to valueTemp, which start from offset, end to the end of the block
                 * the offset is used to locate the word in current line. And the value will store from
                 * the start of the valueTemp array
                 */
                cache_get_byElem(valueTemp, line->cacheBlock + offset,
                                 sizeOfBlock - offset, 0);

                isHitLine1 = 1;  // set line1's hit flag
                break;
            }
        }

        // check whether line2 is hit, if hit, update part of the value (from line2) to the valueTemp
        for (int i = 0; i < numOfLines; i++) {
            cache_line * line = &(set->cacheLineArray[i]);
            if (line->valid && line->tag == newTag) {

                // update all the line's time according to LRU rule
                setLRU(set, line, numOfLines);

                /* copy part of the word to valueTemp, which start from start of the block, end until
                 * the char array is full. And the value will store from sizeOfBlock - offset since
                 * elements before this index is from line1 part, elements after this index is line2 part.
                 */
                cache_get_byElem(valueTemp, line->cacheBlock,
                                 8 - (sizeOfBlock - offset), sizeOfBlock-offset);

                isHitLine2 = 1;  // set line2's hit flag
                break;
            }
        }

        /* if the line1 not hit, there's a cache miss, find an evict line,
         * get data from the main memory and store it to the line1, then
         * store part of the data (last several elements) from cache to valueTemp
         */
        if (isHitLine1 == 0) {

            // find an evictedLine according to the LRU rules
            cache_line *evictedLine = findEvict(set, tag, numOfLines);

            /* get data from main memory to line1, if success, copy part of the
             * word (last several elements) from line1 to valueTemp
             * otherwise, return 0 since we fail to find the value
             */
            if(memget(address - offset, evictedLine->cacheBlock, sizeOfBlock)) {
                cache_get_byElem(valueTemp, evictedLine->cacheBlock + offset,
                                 sizeOfBlock - offset, 0);
            } else {
                return 0;
            }
        }

        /* if the line2 not hit, there's a cache miss, find an evict line,
         * get data from the main memory and store it to the line2, then
         * store part of the data (first several elements) from cache to valueTemp
         */
        if (isHitLine2 == 0) {

            // find an evictedLine according to the LRU rules
            cache_line *evictedLine = findEvict(set, newTag, numOfLines);

            /* get data from main memory to line2, if success, copy part of the
             * word (first several elements) from line2 to valueTemp
             * otherwise, return 0 since we fail to find the value
             */
            if(memget(newAddress - newOffset, evictedLine->cacheBlock, sizeOfBlock)) {
                cache_get_byElem(valueTemp, evictedLine->cacheBlock,
                                 8 - (sizeOfBlock - offset), sizeOfBlock-offset);
            } else {
                return 0;
            }
        }

        // reverse the order of valueTemp and copy it to the value
        *value = reverse_endian(valueTemp);
    }

    return 0;
}


