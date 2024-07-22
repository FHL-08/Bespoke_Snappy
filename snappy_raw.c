// #include "hashmap2.h"
#include "hashmap3.h"
#include "snappy_bytes.h"
#include "snappy_comDecom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
// #define snappyFramed
#ifdef snappyFramed
const char* filename = "compressed.txt.sz";
#else
const char* filename = "compressed.jpeg.snappy";
#endif /* snappyFramed */

int main()
{
    struct stat sb;
    const char* infile = "../testdata/fireworks.jpeg";
    FILE* fp = fopen(infile, "rb");
    FILE* compression = fopen(filename, "w");
    if (fp == NULL || compression == NULL)
    {
        printf("Cannot open input/snappy file\n");
        exit(0);
    }

    size_t text_size = 0;
    if (stat(infile, &sb) == -1)
    {
        perror("Cannot create stat object.\n");
        exit(0);
    }
    else
    {
        // text_size = sb.st_size;
        text_size = (sb.st_size > 65536) ? 65536 : sb.st_size;
    }

    char* text = (char*)malloc(text_size + 1);
    if (fread(text, text_size, 1, fp) != 1)
    {
        perror("Cannot read from file.\n");
        free(text);
        exit(0);
    }
    text[text_size] = '\0';

    size_t memory_size = 32 + text_size * (7 / 6);
    uint8_t* output = (uint8_t*)malloc(memory_size);
    size_t output_size = 0;
    HashTable* table = create_HashTable(text_size);
    compress_snappy(&table, text, text_size, &output_size, &output);
    free_table(&table);
    // Display Compression data
    fwrite(output, 1, output_size, compression);
    size_t decompressedSize = 0, i = 0;
    do
    {
        decompressedSize |= (output[i] & 0x7F) << (7 * i);
    } while (output[i++] >> 7);
    uint8_t* decompressed_output = (uint8_t*)malloc(decompressedSize + 1);
    decompress_snappy(output + i, decompressedSize, &decompressed_output);
    decompressed_output[decompressedSize] = '\0';
    printf("Compression: %s\n", !memcmp(decompressed_output, text, decompressedSize) ? "valid" : "invalid");
    printf("original size = %ld, compressed size = %ld\n", text_size, output_size);
    // Free used pointer memory
    free(text);
    free(output);
    free(decompressed_output);

    // Close Files
    fclose(fp);
    fclose(compression);
    return 0;
}
