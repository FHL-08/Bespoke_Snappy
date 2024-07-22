#ifndef SNAPPYBYTES_H
#define SNAPPYBYTES_H

// #include "hashmap2.h"
#include "hashmap3.h"
#include <stdint.h>

uint32_t decode_bytes(uint8_t* u, int size);
int numBytes(uint32_t offset, int chunkSize);
void split_bytes(int size, uint8_t* array, size_t data, uint32_t* pointer);
void new_memcpy(uint8_t* dest, uint8_t* src, size_t n);
void splitArray(char* inputArray, size_t inputSize, size_t subarraySize, size_t numSubArray, char** outputArray,
                size_t* arraySizes);
void free_2DArray(void** data, int size);

#endif /* SNAPPYBYTES_H */