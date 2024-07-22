#ifndef HASHMAP3_H
#define HASHMAP3_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define H 64 // Currently Capped at 255. Change the numNeighborFilled Bit Field to change upper limit.
#define MAP_WIDTH 64
#define HOP_LIMIT 2 * MAP_WIDTH
#define UNLIKELY(x) (__builtin_expect(x, 0))
#define LIKELY(x) (__builtin_expect(!!(x), 1))

typedef struct fragment
{
    uint32_t hash;
    uint32_t key;
    uint32_t position;
    union {
        struct
        {
            uint16_t containsData : 1;
            uint16_t duplicate_present : 1;
            uint16_t numNeighborhoodFilled : 8;
            uint16_t waste : 6;
        } internal;
        uint16_t initialise;
    } flags;
} fragment;

typedef struct HashTable
{
    uint32_t size;
    uint64_t* positionMap;
    fragment** Map;
} HashTable;

HashTable* create_HashTable(uint32_t N);
fragment* insertToHashTable(HashTable** hashtable, uint32_t key, uint32_t position);
void free_table(HashTable** table);

#endif /* HASHMAP2_H */