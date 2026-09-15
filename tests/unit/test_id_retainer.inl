#include "catedu/core/storage/id_retainer.hpp"
#include "tau/tau.h"

TEST(IdRetainer, BasicTest)
{
    auto retainer = IdRetainer<std::string>::init();

    retainer.begin_cycle();

    SECTION("test single depth")
    {
        retainer.push("left", "right");
        CHECK_TRUE(retainer.value() == "right");
        retainer.pop();
        retainer.push("up", "down");
        CHECK_TRUE(retainer.value() == "down");
        retainer.pop();
    }

    retainer.end_cycle();
    retainer.begin_cycle();

    SECTION("test single depth memory retention")
    {
        retainer.push("left");
        CHECK_TRUE(retainer.value() == "right");
        retainer.pop();
        retainer.push("up");
        CHECK_TRUE(retainer.value() == "down");
        retainer.pop();
    }

    retainer.end_cycle();
    retainer.begin_cycle();

    SECTION("test multiple depth")
    {
        retainer.push("alpha", "a");
        retainer.push("beta", "b");
        CHECK_TRUE(retainer.value() == "b");
        retainer.pop();
        CHECK_TRUE(retainer.value() == "a");
        retainer.pop();
    }

    retainer.end_cycle();
    retainer.begin_cycle();

    SECTION("test multiple depth retention")
    {
        retainer.push("alpha");
        retainer.push("beta");
        CHECK_TRUE(retainer.value() == "b");
        retainer.pop();
        CHECK_TRUE(retainer.value() == "a");
        retainer.pop();
    }

    retainer.end_cycle();
    retainer.begin_cycle();
    retainer.end_cycle();
    retainer.begin_cycle();

    SECTION("test lifetime pruning")
    {
        retainer.push("alpha");
        retainer.push("beta");
        CHECK_FALSE(retainer.value() == "b");
        retainer.pop();
        CHECK_FALSE(retainer.value() == "a");
        retainer.pop();
    }

    retainer.end_cycle();
}
