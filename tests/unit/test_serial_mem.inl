#include "catedu/core/memory/serial_mem.hpp"
#include "tau/tau.h"

const char *example_string = "Hello world!";
size_t example_string_length = 13;

struct BigStructure
{
    char dummy[32];
};

TEST(SerialMem, BasicTest)
{
    uint8_t dest_buffer[32];

    SECTION("write")
    {
        auto serial =
            SerialMem::from_buffer({dest_buffer, 32}, SerialMem::Write);

        CHECK_EQ((unsigned long)serial.write_value<size_t>(64), 0);
        CHECK_EQ((unsigned long)serial.write_value(example_string_length), 0);
        CHECK_EQ((unsigned long)serial.write_buffer(
                     {(void *)example_string, example_string_length}),
                 0);
        CHECK_EQ((unsigned long)serial.write_value<BigStructure>({}),
                 (unsigned long)(sizeof(BigStructure) - 3));
    }

    SECTION("read")
    {
        auto serial =
            SerialMem::from_buffer({dest_buffer, 32}, SerialMem::Read);

        size_t value;
        size_t string_length;
        char string[32];
        BigStructure dummy;

        CHECK_EQ((unsigned long)serial.read_value<size_t>(&value), 0);
        CHECK_EQ((unsigned long)serial.read_value(&string_length), 0);
        printf("%zu %zu %zu\n", value, serial.at, string_length);
        CHECK_EQ((unsigned long)serial.read_buffer({string, string_length}), 0);
        CHECK_GT((unsigned long)serial.read_value<BigStructure>(&dummy), 0);

        CHECK_EQ((unsigned long)value, 64);
        CHECK_EQ((unsigned long)string_length,
                 (unsigned long)example_string_length);
        CHECK_STREQ(string, example_string);
    }
}
