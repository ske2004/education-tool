#include <vector>
#include "catedu/scene/world_file.hpp"
#include "tau/tau.h"
#include <stdio.h>
#include <string.h>

static const char *const WORLD_FILE_TEST_PATH = "bin/world_file_test.dat";

static size_t world_file_object_count(Dispatcher &disp)
{
    size_t n = 0;
    for (auto &obj : iter(disp.world.first->objects))
    {
        (void)obj;
        n++;
    }
    return n;
}

static Dispatcher world_file_make()
{
    Dispatcher disp = Dispatcher::create();
    Object o = {};
    o.type = Object::Type::road;
    o.floors = 1;
    disp.world.first->place_object(o);
    o.type = Object::Type::building;
    o.x = 10;
    disp.world.first->place_object(o);
    return disp;
}

TEST(WorldFile, RoundTripWithHeader)
{
    Dispatcher disp = world_file_make();
    size_t expected = world_file_object_count(disp);
    REQUIRE_TRUE(expected >= 1);
    REQUIRE_TRUE(WorldFile::save(WORLD_FILE_TEST_PATH, disp));

    FILE *f = fopen(WORLD_FILE_TEST_PATH, "rb");
    REQUIRE_TRUE(f);
    char magic[4] = {};
    REQUIRE_TRUE(fread(magic, 4, 1, f) == 1);
    fclose(f);
    CHECK_TRUE(memcmp(magic, "EDUW", 4) == 0);

    Dispatcher loaded = WorldFile::load(WORLD_FILE_TEST_PATH);
    CHECK_EQ(world_file_object_count(loaded), expected);
    remove(WORLD_FILE_TEST_PATH);
}

TEST(WorldFile, LegacyHeaderlessStillLoads)
{
    Dispatcher disp = world_file_make();
    size_t expected = world_file_object_count(disp);
    REQUIRE_TRUE(WorldFile::save(WORLD_FILE_TEST_PATH, disp));

    // Strip the 8 byte header to produce a v1 file.
    FILE *f = fopen(WORLD_FILE_TEST_PATH, "rb");
    REQUIRE_TRUE(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 8, SEEK_SET);
    std::vector<char> body(size - 8);
    REQUIRE_TRUE(fread(body.data(), 1, body.size(), f) == body.size());
    fclose(f);
    f = fopen(WORLD_FILE_TEST_PATH, "wb");
    fwrite(body.data(), 1, body.size(), f);
    fclose(f);

    Dispatcher loaded = WorldFile::load(WORLD_FILE_TEST_PATH);
    CHECK_EQ(world_file_object_count(loaded), expected);
    remove(WORLD_FILE_TEST_PATH);
}

TEST(WorldFile, NewerVersionRejected)
{
    Dispatcher disp = world_file_make();
    REQUIRE_TRUE(WorldFile::save(WORLD_FILE_TEST_PATH, disp));
    FILE *f = fopen(WORLD_FILE_TEST_PATH, "r+b");
    REQUIRE_TRUE(f);
    fseek(f, 4, SEEK_SET);
    uint32_t v = 999;
    fwrite(&v, sizeof(v), 1, f);
    fclose(f);

    Dispatcher loaded = WorldFile::load(WORLD_FILE_TEST_PATH);
    CHECK_EQ(world_file_object_count(loaded), (size_t)0);
    remove(WORLD_FILE_TEST_PATH);
}
