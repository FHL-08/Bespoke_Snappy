// #include "hashmap2.h"
#include "hashmap3.h"
#include "snappy_bytes.h"
#include "snappy_comDecom.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main()
{
    /* compressed.10k.sz */
    char compressedFile[] = "compressor_output.snappy", decompressedFile[] = "output.txt";
    struct stat sb;
    FILE* compressedData = fopen(compressedFile, "rb");
    FILE* decompressedData = fopen(decompressedFile, "w");
    if (compressedData == NULL || decompressedData == NULL)
    {
        printf("Cannot open files.\n");
        exit(0);
    }

    size_t text_size = 0;
    if (stat(compressedFile, &sb) == -1)
    {
        perror("Cannot create stat object.\n");
        goto stat_error;
    }
    else
    {
        text_size = sb.st_size;
    }
    uint8_t* text = (uint8_t*)malloc(text_size + 1);
    if (fread(text, text_size, 1, compressedData) != 1)
    {
        perror("Cannot read from file.\n");
        goto error;
    }
    text[text_size] = '\0';

    uint8_t* decompressed_output = NULL;
    size_t decompressedSize = 0;
    if (/* snappyStreamDecompress(text, text_size, &decompressedSize, &decompressed_output) != 1 */ 0)
    {
        printf("Cannot decompress this stream.\n");
    }
    else
    {
        size_t tempSize = 0, compressed_pointer = 0, i = 0;
        do
        {
            tempSize |= (text[compressed_pointer] & 0x7F) << (7 * i++);
        } while (text[compressed_pointer++] >> 7);
        decompressedSize += tempSize;
        printf("%lu, %lu\n", decompressedSize, compressed_pointer);
        decompressed_output = (uint8_t*)malloc(decompressedSize + 1);
        decompress_snappy(text + compressed_pointer, decompressedSize, &decompressed_output);
        decompressed_output[decompressedSize] = '\0';
        if (fwrite(decompressed_output, decompressedSize, 1, decompressedData) != 1)
        {
            perror("Cannot write to output file.\n");
            goto error;
        }
    }

error:
    free(text);
    free(decompressed_output);
stat_error:
    fclose(compressedData);
    fclose(decompressedData);
    return 0;
}
