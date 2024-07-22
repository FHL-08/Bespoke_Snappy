#include "snappy_comDecom.h"

void generate_CRCTable(uint32_t* table)
{
    for (int bytes = 0; bytes < 256; ++bytes)
    {
        uint32_t remainder = bytes;
        for (int i = 0; i < 8; i++)
        {
            if (remainder & 1)
            {
                remainder = (remainder >> 1) ^ 0x82F63B78;
            }
            else
            {
                remainder >>= 1;
            }
        }
        table[bytes] = remainder;
    }
}

uint32_t masked_CRC32C(uint32_t* table, char* data, size_t data_size)
{
    uint32_t checksum = 0xFFFFFFFF;
    for (int i = 0; i < data_size; ++i)
    {
        checksum = table[(checksum ^ (unsigned)data[i]) & 0xFF] ^ (checksum >> 8);
    }
    checksum ^= 0xFFFFFFFF;
    return ((checksum >> 15) | (checksum << 17)) + 0xa282ead8;
}

void compress_snappy(HashTable** table, const char* text, size_t text_size, size_t* output_size,
                     uint8_t** compressed_out)
{
    // Snappy Compression
    uint8_t num_bytes = numBytes(text_size, 7), output_pointer = 0;
    for (; output_pointer < num_bytes; output_pointer++)
    {
        uint8_t flag = (num_bytes - output_pointer) == 1 ? 0 : 1;
        (*compressed_out)[output_pointer] = (flag << 7) | ((text_size >> (7 * output_pointer)) & 0x7F);
    }
    uint32_t data_pointer = output_pointer, text_pointer = 0, skip = 32;
    for (size_t i = 0; i + 3 < text_size;)
    {
        uint32_t data;
        memcpy(&data, text + i, 4);
        fragment* head = insertToHashTable(table, data, i);
        if (UNLIKELY(head && head->flags.internal.duplicate_present))
        {
            skip = 32;
            uint32_t size = 4, offset = i - head->position;
            const char *prev_rev = text + head->position - 1, *curr_rev = text + i - 1;
            const char *prev_for = text + head->position + 4, *curr_for = text + i + 4;
            head->position = i;
            while ((curr_for < text + text_size) && (*(curr_for++) == *(prev_for++)) && (size < 64))
            { // Check Characters after current fragment
                size++;
            }
            uint8_t branch = 0;
            while (UNLIKELY((i > text_pointer) && (prev_rev > text) && (*(curr_rev--) == *(prev_rev--)) && (size < 64)))
            { // Check Characters before current fragment
                branch = 1;
                size++;
                i--;
            }
            if (branch)
            {
                memcpy(&data, text + i, 4);
                insertToHashTable(table, data, i);
            }

            if (UNLIKELY(size == 4 && offset > 65536u)) // Special Check
            {
                i += (size - 1);
                goto match_skip;
            }

            /* Emit Literal */
            uint32_t length = i - text_pointer;
            if (length > 0)
            {
                uint8_t num_bytes1 = numBytes(length, 8),
                        tag_byte = (length > 60) ? ((60 + num_bytes1 - 1) << 2) : ((length - 1) << 2);
                (*compressed_out)[data_pointer++] = tag_byte;
                if (length > 60)
                {
                    split_bytes(num_bytes1, *compressed_out, length - 1, &data_pointer);
                }
                memcpy(*compressed_out + data_pointer, text + text_pointer, length);
                text_pointer += length;
                data_pointer += length;
            }

            /* Emit Copy */
            if (size <= (0x07 + 4) && (32 - __builtin_clz(offset)) <= 11)
            {
                (*compressed_out)[data_pointer++] = (((size - 4) << 2) | 0x01) | ((offset & 0x700) >> 3);
                (*compressed_out)[data_pointer++] = (offset & 0xFF);
            }
            else
            {
                uint8_t num_bytes2 = numBytes(offset, 8) > 2 ? 4 : 2;
                switch (num_bytes2)
                {
                case 2:
                    (*compressed_out)[data_pointer++] = (((size - 1) << 2) | 0x02);
                    break;
                case 4:
                    (*compressed_out)[data_pointer++] = (((size - 1) << 2) | 0x03);
                    break;
                }
                split_bytes(num_bytes2, *compressed_out, offset, &data_pointer);
            }
            text_pointer += size;
            i += (size - 1);
        }
    // Hueristic Match Skipping
    match_skip:
        if (skip > 32 + 16)
        {
            uint32_t bytes_between_hash_lookups = skip >> 5;
            skip += bytes_between_hash_lookups;
            i += bytes_between_hash_lookups;
        }
        else
        {
            skip++;
            i++;
        }
    }
    // Store the remaining bytes as a literal element
    if ((size_t)text_pointer < text_size)
    {
        uint32_t length = text_size - text_pointer;
        uint8_t num_bytes1 = numBytes(length, 8),
                tag_byte = (length > 60) ? ((60 + num_bytes1 - 1) << 2) : ((length - 1) << 2);
        (*compressed_out)[data_pointer++] = tag_byte;
        if (length > 60)
        {
            split_bytes(num_bytes1, *compressed_out, length - 1, &data_pointer);
        }
        memcpy(*compressed_out + data_pointer, text + text_pointer, length);
        text_pointer += length;
        data_pointer += length;
    }
    // Store the size of the compressed message
    (*compressed_out)[data_pointer] = '\0';
    *output_size = data_pointer;
    return;
}

void snappyStreamCompress(const char* text, size_t text_size, size_t* output_size, uint8_t** compress_out)
{
    HashTable* table = create_HashTable(text_size);
    size_t memory_size = 32 + text_size * (7.0 / 6.0);
    uint8_t* compressed_data = (uint8_t*)malloc(memory_size);
    compress_snappy(&table, text, text_size, output_size, &compressed_data);

    if (*output_size < text_size)
    {
        uint8_t* output = (uint8_t*)malloc(*output_size);
        memcpy(output, compressed_data, *output_size);
        *compress_out = output;
    }
    else
    {
        uint8_t* output = (uint8_t*)malloc(text_size);
        memcpy(output, text, text_size);
        *compress_out = output;
    }
    free(compressed_data);
    free_table(&table);
}

void decompress_snappy(uint8_t* output, size_t uncompressed_size, uint8_t** decompress_out)
{
    size_t compressed_pointer = 0, decompressed_pointer = 0;
    // Decompress the data using the Snappy Format.
    while (decompressed_pointer < uncompressed_size)
    {
        unsigned int dict_length = 0, dict_offset = 0, compare = output[compressed_pointer] >> 2;
        switch (output[compressed_pointer] & 0x03)
        {
        case 0: {
            unsigned int literal_size;
            if (compare < 60)
            {
                literal_size = compare + 1;
                compressed_pointer++;
            }
            else
            {
                literal_size = 1 + decode_bytes(output + (compressed_pointer + 1), compare - 59);
                compressed_pointer += (compare - 58);
            }

            if (decompressed_pointer + literal_size > uncompressed_size)
            {
                printf("Uncompressed Size: %lu\n Literal Size: %u\n, Decompressed Pointer: %lu\n", uncompressed_size,
                       literal_size, decompressed_pointer);
                fprintf(stderr, "Error: Invalid Literal Size. Ending Decompression...\n");
                return;
            }
            memcpy(*decompress_out + decompressed_pointer, output + compressed_pointer, literal_size);
            decompressed_pointer += literal_size;
            compressed_pointer += literal_size;
            break;
        }
        case 1: {
            dict_length = 4 + ((output[compressed_pointer] >> 2) & 0x07);
            dict_offset = ((((output[compressed_pointer] >> 5) & 0x07) << 8) | output[compressed_pointer + 1]);
            if (dict_offset > decompressed_pointer)
            {
                fprintf(stderr, "Error: Invalid Offset for Type-1 copy. Ending Decompression...\n");
                return;
            }
            new_memcpy(*decompress_out + decompressed_pointer, *decompress_out + decompressed_pointer - dict_offset,
                       dict_length);
            compressed_pointer += 2;
            decompressed_pointer += dict_length;
            break;
        }
        case 2: {
            dict_length = 1 + ((output[compressed_pointer] >> 2) & 0x3F);
            dict_offset = decode_bytes(output + (compressed_pointer + 1), 2);
            if (dict_offset > decompressed_pointer)
            {
                fprintf(stderr, "Error: Invalid Offset for Type-2 copy. Ending Decompression...\n");
                return;
            }
            new_memcpy(*decompress_out + decompressed_pointer, *decompress_out + decompressed_pointer - dict_offset,
                       dict_length);
            compressed_pointer += 3;
            decompressed_pointer += dict_length;
            break;
        }
        case 3: {
            dict_length = 1 + ((output[compressed_pointer] >> 2) & 0x3F);
            dict_offset = decode_bytes(output + (compressed_pointer + 1), 4);
            if (dict_offset > decompressed_pointer)
            {
                fprintf(stderr, "Error: Invalid Offset for Type-3 copy. Ending Decompression...\n");
                return;
            }
            new_memcpy(*decompress_out + decompressed_pointer, *decompress_out + decompressed_pointer - dict_offset,
                       dict_length);
            compressed_pointer += 5;
            decompressed_pointer += dict_length;
            break;
        }
        }
    }
}

int snappyStreamDecompress(uint8_t* output, size_t output_size, size_t* decompressed_size, uint8_t** decompress_out)
{
    size_t compressed_pointer = 0, decompressed_pointer = 0, uncompressed_size = 0, compressed_size = 0;
    if (output[compressed_pointer++] == 0xff)
    {
        uint32_t frameSize = decode_bytes(output + compressed_pointer, 3);
        compressed_pointer += 3;
        if (memcmp(output + compressed_pointer, "sNaPpY", frameSize) != 0)
        {
            perror("This is not a valid snappy framed document.\n");
            return -1;
        }
        compressed_pointer += 6;
    }
    else
    {
        perror("This is not a valid snappy framed document.\n");
        return -1;
    }

    uint8_t* tempOut = (uint8_t*)malloc(sizeof(uint8_t));
    // Decompress the data using the Snappy algorithm.
    while (compressed_pointer + 1 < output_size)
    {
        uint8_t chunkType = output[compressed_pointer++];
        compressed_size = decode_bytes(output + compressed_pointer, 3) - 4;
        compressed_pointer += 3;
        compressed_pointer += 4; // skip CRC-32C checksum bytes

        if (chunkType == 0x00)
        {
            size_t tempSize = 0, i = 0;
            do
            {
                tempSize |= (output[compressed_pointer] & 0x7F) << (7 * i++);
            } while (output[compressed_pointer++] >> 7);
            uncompressed_size += tempSize;
            /* Assign Memory for Decompression */
            uint8_t* temp = (uint8_t*)realloc(tempOut, uncompressed_size + 1);
            if (temp == NULL)
            {
                perror("realloc() failed!\n");
                free(tempOut);
                tempOut = NULL;
                return -1;
            }
            tempOut = temp;
            temp = NULL;
            /* Decompress Data */
            uint8_t* decompressPoint = tempOut + decompressed_pointer;
            decompress_snappy(output + compressed_pointer, tempSize, &decompressPoint);
            compressed_pointer += compressed_size - i;
            decompressed_pointer += tempSize;
        }
        else if (chunkType == 0x01)
        {
            /* Assign Memory for Decompression */
            uncompressed_size += compressed_size;
            uint8_t* temp = (uint8_t*)realloc(tempOut, uncompressed_size + 1);
            if (temp == NULL)
            {
                perror("realloc() failed!\n");
                free(tempOut);
                tempOut = NULL;
                return -1;
            }
            tempOut = temp;
            temp = NULL;
            /* Copy Data */
            memcpy(tempOut + decompressed_pointer, output + compressed_pointer, compressed_size);
            decompressed_pointer += compressed_size;
            compressed_pointer += compressed_size;
        }
        else if (chunkType < 0x80)
        {
            printf("Unsupported identifier 0x%02x\n", chunkType);
            return -1;
        }
        else
        {
            decompressed_pointer += compressed_size;
        }
    }
    tempOut[uncompressed_size] = '\0';
    *decompress_out = tempOut;
    *decompressed_size = uncompressed_size;
    return 1;
}