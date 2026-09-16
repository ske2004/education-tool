#include "file_buffer.hpp"
#include <catedu/core/alloc/allocator.hpp>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

FileBuffer FileBuffer::read_whole_file(FILE *f)
{
    fseek(f, 0, SEEK_END);
    long tell_result = ftell(f);
    if (tell_result < 0)
    {
        fprintf(stderr, "FileBuffer::read_whole_file: ftell failed\n");
        return {nullptr, 0};
    }
    size_t file_size = (size_t)tell_result;
    fseek(f, 0, SEEK_SET);

    uint8_t *data = (uint8_t *)ALLOCATOR_MALLOC.alloc(file_size);

    size_t read = fread(data, 1, file_size, f);
    if (read < file_size)
    {
        fprintf(stderr, "FileBuffer::read_whole_file: short read (%zu of %zu bytes)\n", read, file_size);
    }

    return {data, read};
}

void FileBuffer::write_whole_file(FILE *f)
{
    fwrite(data, 1, size, f);
}

void FileBuffer::deinit()
{
    ALLOCATOR_MALLOC.free(data);
}
