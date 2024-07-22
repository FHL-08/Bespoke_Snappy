#ifndef SNAPPYCOMDECOM_H
#define SNAPPYCOMDECOM_H

// #include "hashmap2.h"
#include "hashmap3.h"
#include "snappy_bytes.h"

void generate_CRCTable(uint32_t* table);
uint32_t masked_CRC32C(uint32_t* table, char* data, size_t data_size);
void compress_snappy(HashTable** table, const char* text, size_t text_size, size_t* output_size,
                     uint8_t** compressed_out);
void snappyStreamCompress(const char* text, size_t text_size, size_t* output_size, uint8_t** compress_out);
void decompress_snappy(uint8_t* output, size_t uncompressed_size, uint8_t** decompress_out);
int snappyStreamDecompress(uint8_t* output, size_t output_size, size_t* decompressed_size, uint8_t** decompress_out);
#endif /* SNAPPYCOMDECOM_H */