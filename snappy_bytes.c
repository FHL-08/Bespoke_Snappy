#include "snappy_bytes.h"

uint32_t decode_bytes(uint8_t* u, int size)
{
    uint32_t result = 0;
    for (int i = (size - 1); i >= 0; i--)
    {
        result = (result << 8) | u[i];
    }
    return result;
}

void split_bytes(int size, uint8_t* array, size_t data, uint32_t* pointer)
{
    memcpy(&array[*pointer], &data, size);
    *pointer += size;
}

int numBytes(uint32_t offset, int chunkSize)
{
    int numBits = 32 - __builtin_clz(++offset); // Counts the number of bits
    int result = (numBits + (chunkSize - 1)) / chunkSize;
    return result;
}

void new_memcpy(uint8_t* dest, uint8_t* src, size_t n)
{
    if (dest == NULL || src == NULL)
    {
        fprintf(stderr, "NULL pointer detected\n");
        return;
    }

    if (src + n >= dest)
    {
        uint8_t buffer[64] = {0};
        size_t interval = dest - src;
        memcpy(buffer, src, interval);
        while (n > interval)
        {
            memcpy(dest, buffer, interval);
            dest += interval;
            n -= interval;
        }
        memcpy(dest, buffer, n);
    }
    else
    {
        memcpy(dest, src, n);
    }
}

void splitArray(char* inputArray, size_t inputSize, size_t subarraySize, size_t numSubArray, char** outputArray,
                size_t* arraySizes)
{
    if (inputSize == 0 || subarraySize == 0)
    {
        printf("Invalid input sizes.\n");
        return;
    }

    for (size_t i = 0; i < numSubArray; ++i)
    {
        // Calculate the start and end indices for the current subarray
        size_t startIdx = i * subarraySize;
        size_t endIdx = (i + 1) * subarraySize;
        if (endIdx > inputSize)
        {
            endIdx = inputSize; // Adjust the end index for the last subarray
        }
        size_t interval = endIdx - startIdx;

        // Allocate memory for the subarray
        char* subarray = (char*)malloc(interval + 1);
        if (subarray == NULL)
        {
            perror("Memory allocation error");
            exit(EXIT_FAILURE);
        }

        // Copy the elements from the original array to the subarray
        memcpy(subarray, inputArray + startIdx, interval);
        subarray[endIdx - startIdx] = '\0'; // Null-terminate the subarray
        outputArray[i] = subarray;
        arraySizes[i] = interval;
    }
}

void free_2DArray(void** data, int size)
{
    for (int i = 0; i < size; ++i)
    {
        if (data[i] != NULL)
        {
            free(data[i]);
        }
    }
    free(data);
}