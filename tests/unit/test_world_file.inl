#include "catedu/scene/world_file.hpp"
#include "tau/tau.h"
#include <stdio.h>
#include <string.h>

static int world_file_test_count_objects(Dispatcher &dispatcher)
{
    int count = 0;
    for (auto &place : iter(dispatcher.world.places))
    {
        for (auto &_ : iter(place.objects))
        {
            (void)_;
            count++;
        }
    }
    return count;
}

TEST(WorldFile, LoadsLegacyAndRoundTripsV2)
{
    // assets/world.dat is in the pre-versioning (v1) format.
    Dispatcher legacy = WorldFile::load("assets/world.dat");
    int legacy_count = world_file_test_count_objects(legacy);
    CHECK_GT(legacy_count, 0);

    // Tag one object with v2-only data.
    Object *tagged = nullptr;
    for (auto &obj : iter(legacy.world.first->objects))
    {
        tagged = &obj;
        break;
    }
    REQUIRE_NE(tagged, nullptr);
    float tagged_x = tagged->x;
    float tagged_y = tagged->y;
    tagged->z = 1.5f;
    strcpy(tagged->id, "bench");

    const char *path = "bin/test_world_roundtrip.dat";
    REQUIRE_TRUE(WorldFile::save(path, legacy));

    Dispatcher reloaded = WorldFile::load(path);
    CHECK_EQ(world_file_test_count_objects(reloaded), legacy_count);

    bool found = false;
    for (auto &obj : iter(reloaded.world.first->objects))
    {
        if (obj.x == tagged_x && obj.y == tagged_y)
        {
            found = true;
            CHECK_EQ(obj.z, 1.5f);
            CHECK_STREQ(obj.id, "bench");
        }
    }
    CHECK_TRUE(found);

    legacy.destroy();
    reloaded.destroy();
    remove(path);
}

TEST(WorldFile, LoadsUnversionedFileWithPropId)
{
    // Layout written between 8ba44ed and versioning: no header, objects
    // carry a 32-byte id but no z.
    const char *path = "bin/test_world_v1.dat";
    FILE *f = fopen(path, "wb");
    REQUIRE_NE(f, nullptr);

    uint32_t place_count = 1, obj_count = 1;
    bool interior = false;
    int32_t type = (int32_t)Object::Type::prop;
    int32_t floors = 0;
    float x = 3.0f, y = 4.0f;
    uint32_t place = 0;
    char id[32] = "bench";

    fwrite(&place_count, sizeof(place_count), 1, f);
    fwrite(&obj_count, sizeof(obj_count), 1, f);
    fwrite(&interior, sizeof(interior), 1, f);
    fwrite(&type, sizeof(type), 1, f);
    fwrite(&floors, sizeof(floors), 1, f);
    fwrite(&x, sizeof(x), 1, f);
    fwrite(&y, sizeof(y), 1, f);
    fwrite(&place, sizeof(place), 1, f);
    fwrite(id, sizeof(id), 1, f);
    fclose(f);

    Dispatcher d = WorldFile::load(path);
    REQUIRE_EQ(world_file_test_count_objects(d), 1);
    for (auto &obj : iter(d.world.first->objects))
    {
        CHECK_EQ((int)obj.type, (int)Object::Type::prop);
        CHECK_EQ(obj.x, 3.0f);
        CHECK_EQ(obj.y, 4.0f);
        CHECK_STREQ(obj.id, "bench");
    }

    d.destroy();
    remove(path);
}

TEST(WorldFile, RejectsNewerVersion)
{
    const char *path = "bin/test_world_future.dat";
    FILE *f = fopen(path, "wb");
    REQUIRE_NE(f, nullptr);
    uint32_t header[3] = {0x57554445, 999, 5};
    fwrite(header, sizeof(header), 1, f);
    fclose(f);

    Dispatcher d = WorldFile::load(path);
    CHECK_EQ(world_file_test_count_objects(d), 0);

    d.destroy();
    remove(path);
}
