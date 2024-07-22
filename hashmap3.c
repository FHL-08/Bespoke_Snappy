#include "hashmap3.h"

static inline uint64_t set_bit(uint64_t map, uint8_t position)
{
    return map | (1ull << ((MAP_WIDTH - 1) - position));
}
static inline uint64_t clear_bit(uint64_t map, uint8_t position)
{
    return map & ~(1ull << ((MAP_WIDTH - 1) - position));
}
static inline uint64_t setUpperNbits(uint64_t map, uint8_t N)
{
    if (N > 0) return map | ~((1ull << (MAP_WIDTH - N)) - 1);
    return map;
}
static inline uint64_t setLowerNbits(uint64_t map, uint8_t N)
{
    return map | ((1ull << N) - 1);
}
static inline uint32_t city_hash(uint32_t key, uint32_t N)
{
    size_t hash = (131 * 131 * 131 * (uint8_t)(key)) + (131 * 131 * (uint8_t)(key >> 8)) +
                  (131 * (uint8_t)(key >> 16)) + (uint8_t)(key >> 24);
    hash ^= (hash >> 17);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);
    return (uint32_t)(hash % N);
}
static inline fragment* initialise_fragment()
{
    fragment* new_fragment = (fragment*)malloc(sizeof(fragment));
    new_fragment->hash = 0u;
    new_fragment->key = 0u;
    new_fragment->flags.initialise = 0;
    return new_fragment;
}

HashTable* create_HashTable(uint32_t N)
{
    uint32_t tableSize = 1.25 * N;
    uint32_t MapSize = 1 + ((tableSize - (tableSize & (MAP_WIDTH - 1))) / MAP_WIDTH);
    HashTable* table = (HashTable*)malloc(sizeof(HashTable));
    table->Map = (fragment**)calloc(tableSize, sizeof(fragment*));
    table->size = tableSize;
    table->positionMap = (uint64_t*)calloc(MapSize, sizeof(uint64_t));
    table->positionMap[MapSize - 1] = setLowerNbits(0, (MapSize * MAP_WIDTH) - tableSize);
    for (uint32_t i = 0; i < tableSize; i++)
    {
        table->Map[i] = initialise_fragment();
    }
    return table;
}

fragment* insertToHashTable(HashTable** hashtable, uint32_t key, uint32_t position)
{
    uint32_t size = (*hashtable)->size, MapSize = 1 + ((size - (size & (MAP_WIDTH - 1))) / MAP_WIDTH);
    fragment **Map = (*hashtable)->Map, item = {city_hash(key, size), key, position, {{0}}};
    uint8_t numNeighborhoodFilled = Map[item.hash]->flags.internal.numNeighborhoodFilled;
    uint64_t* positionMap = (*hashtable)->positionMap;
    if (UNLIKELY(numNeighborhoodFilled == H)) return NULL;
    uint32_t hashOffset = 0;
    for (int i = 1; i <= H; i++)
    { // Scan through neighborhood
        hashOffset = i - 1;
        int index = (item.hash + hashOffset) % size;
        if (LIKELY(Map[index]->flags.internal.containsData == 0))
        {
            if (LIKELY(hashOffset < H))
            {
                uint32_t MapIndex = (index / MAP_WIDTH) % MapSize;
                positionMap[MapIndex] = set_bit(positionMap[MapIndex], index & (MAP_WIDTH - 1));
                Map[item.hash]->flags.internal.numNeighborhoodFilled++;
                uint8_t temp = Map[index]->flags.internal.numNeighborhoodFilled;
                *Map[index] = item;
                Map[index]->flags.internal.containsData = 1;
                Map[index]->flags.internal.numNeighborhoodFilled = temp;
                return Map[index];
            }
            break;
        }
        if (UNLIKELY(Map[index]->key == item.key))
        {
            Map[index]->flags.internal.duplicate_present = 1;
            return Map[index];
        }
    }
    uint32_t curr_pos = (item.hash + H) % size, MapIndex = curr_pos / MAP_WIDTH;
    uint8_t space_found = 0;
    do
    { // Find Empty Bucket
        uint64_t currMap = setUpperNbits(positionMap[MapIndex % MapSize], curr_pos & (MAP_WIDTH - 1));
        hashOffset = ((curr_pos - item.hash) + (__builtin_clzll(~currMap) - (curr_pos & (MAP_WIDTH - 1)))) % size;
        if (UNLIKELY(hashOffset >= HOP_LIMIT)) return NULL;
        if (~currMap != 0)
        {
            space_found = 1;
            break;
        }
        MapIndex++;
        curr_pos = (MapIndex % MapSize) * MAP_WIDTH;
    } while (curr_pos < (item.hash + size));
    // Hop empty space into neighborhood
    if (UNLIKELY(!space_found)) return NULL;
    int j = item.hash + hashOffset;
    uint32_t scaledj = j % size, neighbourhoodID = 0;
    uint32_t temp_numNeighborhoodFilled = 0;
    while ((j - item.hash) % size >= H)
    {
        fragment* emptyBin = Map[scaledj];
        temp_numNeighborhoodFilled = emptyBin->flags.internal.numNeighborhoodFilled;
        uint32_t index_min = (j - (H - 1)) % size;
        uint8_t found_swap = 0;
        for (uint32_t x = H - 1; x > 0; x--)
        { // find swappable item
            uint32_t current_index = (j - x) % size;
            neighbourhoodID = Map[current_index]->hash;
            if (UNLIKELY(neighbourhoodID < index_min)) continue;
            found_swap = 1;
            MapIndex = (j / MAP_WIDTH) % MapSize;
            positionMap[MapIndex] = set_bit(positionMap[MapIndex], j & (MAP_WIDTH - 1));
            j -= x;
            MapIndex = (j / MAP_WIDTH) % MapSize;
            positionMap[MapIndex] = clear_bit(positionMap[MapIndex], j & (MAP_WIDTH - 1));
            // Swap Positions
            scaledj = j % size;
            *emptyBin = *Map[scaledj];
            emptyBin->flags.internal.numNeighborhoodFilled = temp_numNeighborhoodFilled;
            break;
        }
        if (UNLIKELY(!found_swap)) return NULL;
    } // Insert Item
    positionMap[MapIndex] = set_bit(positionMap[MapIndex], j & (MAP_WIDTH - 1));
    Map[neighbourhoodID]->flags.internal.numNeighborhoodFilled++;
    uint8_t temp = Map[scaledj]->flags.internal.numNeighborhoodFilled;
    *Map[scaledj] = item;
    Map[scaledj]->flags.internal.numNeighborhoodFilled = temp;
    Map[scaledj]->flags.internal.containsData = 1;
    return Map[scaledj];
}

void free_table(HashTable** table)
{
    for (uint32_t i = 0; i < (*table)->size; i++)
    {
        free((*table)->Map[i]);
        (*table)->Map[i] = NULL;
    }
    free((*table)->positionMap);
    (*table)->positionMap = NULL;
    free((*table)->Map);
    (*table)->Map = NULL;
    free(*table);
    *table = NULL;
}