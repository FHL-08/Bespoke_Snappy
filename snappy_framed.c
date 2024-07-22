#include "hashmap3.h"
#include "snappy_bytes.h"
#include "snappy_comDecom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define snappyFramed
#ifdef snappyFramed
const char* filename = "compressed.10k.sz";
#else
const char* filename = "compressed.snappy";
#endif /* snappyFramed */

int main()
{
    struct stat sb;
    const char* infile = "../testdata/urls.10k";
    FILE* fp = fopen(infile, "rb");
    FILE* compression = fopen(filename, "w");
    if (fp == NULL || compression == NULL)
    {
        printf("Cannot open input/output file\n");
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
        text_size = sb.st_size;
    }

    char* text = (char*)malloc(text_size + 1);
    if (fread(text, text_size, 1, fp) != 1)
    {
        perror("Cannot read from file.\n");
        free(text);
        exit(0);
    }
    text[text_size] = '\0';

    size_t compressedDataSize = 10, numSubArray = (text_size + (0x10000) - 1) / (0x10000);
    uint8_t** output = (uint8_t**)malloc(sizeof(uint8_t*) * numSubArray);
    char** chunks = (char**)malloc(sizeof(char*) * numSubArray);
    size_t* chunkSizes = (size_t*)malloc(sizeof(size_t) * numSubArray);
    size_t* compressedChunkSizes = (size_t*)malloc(sizeof(size_t) * numSubArray);
    uint8_t* chunkTypes = (uint8_t*)malloc(sizeof(uint8_t) * numSubArray);
    splitArray(text, text_size, (0x10000), numSubArray, chunks, chunkSizes);

    uint32_t CRCtable[256];
    generate_CRCTable(CRCtable);
    for (int i = 0; i < numSubArray; i++)
    {
        size_t output_size = 0;
        snappyStreamCompress(chunks[i], chunkSizes[i], &output_size, &(output[i]));
        if (output_size >= chunkSizes[i])
        {
            chunkTypes[i] = 0x01;
            compressedDataSize += (8 + chunkSizes[i]);
            compressedChunkSizes[i] = chunkSizes[i];
        }
        else
        {
            chunkTypes[i] = 0x00;
            compressedDataSize += (8 + output_size);
            compressedChunkSizes[i] = output_size;
        }
    }
    uint8_t magic_number[10] = {0xff, 0x06, 0x00, 0x00, 0x73, 0x4e, 0x61, 0x50, 0x70, 0x59};
    uint8_t* compressed_data = (uint8_t*)malloc(compressedDataSize);
    memcpy(compressed_data, magic_number, 10);

    size_t startIdx = 10;
    for (int i = 0; i < numSubArray; i++)
    {
        uint8_t chunkHeader[4] = {chunkTypes[i], 0, 0, 0};
        uint32_t headerPos = 1;
        split_bytes(3, chunkHeader, compressedChunkSizes[i] + 4, &headerPos);
        memcpy(compressed_data + startIdx, chunkHeader, headerPos);
        startIdx += 4;
        uint8_t checkSum[4] = {0, 0, 0, 0};
        headerPos = 0;
        uint32_t CRCout = masked_CRC32C(CRCtable, chunks[i], chunkSizes[i]); // Mask Checksum
        split_bytes(4, checkSum, CRCout, &headerPos);
        memcpy(compressed_data + startIdx, checkSum, headerPos);
        startIdx += 4;
        memcpy(compressed_data + startIdx, output[i], compressedChunkSizes[i]);
        startIdx += compressedChunkSizes[i];
    }

    // Display Compression data
    fwrite(compressed_data, 1, compressedDataSize, compression);

    uint8_t* decompressed_output = NULL;
    size_t decompressedSize = 0;
    if (snappyStreamDecompress(compressed_data, compressedDataSize, &decompressedSize, &decompressed_output) == 1)
    {
        // Validate Decompressed data
        // printf("%s\n", decompressed_output);
        printf("Compression: %s\n", !memcmp(decompressed_output, text, decompressedSize) ? "valid" : "invalid");
    }
    else
    {
        printf("Cannot decompress the frame format.\n");
    }

    // Free used pointer memory
    free(text);
    free_2DArray((void**)output, numSubArray);
    free_2DArray((void**)chunks, numSubArray);
    free(chunkSizes);
    free(compressedChunkSizes);
    free(chunkTypes);
    free(compressed_data);
    free(decompressed_output);

    // Close Files
    fclose(fp);
    fclose(compression);
    return 0;
}
