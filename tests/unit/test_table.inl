#include "catedu/core/storage/table.hpp"
#include "tau/tau.h"

TEST(Table, BasicTest)
{
    Table<int> test_table = {};

    TableId item1 = test_table.allocate(42);
    TableId item2 = test_table.allocate(8086);

    CHECK_EQ(*test_table.get(item1), 42);
    CHECK_EQ(*test_table.get(item2), 8086);

    CHECK_TRUE(test_table.remove(item1));

    CHECK_EQ(test_table.get(item1), nullptr);

    TableId item3 = test_table.allocate(123);

    CHECK_EQ(test_table.get(item1), nullptr);

    CHECK_TRUE(test_table.remove(item3));

    TableId item4 = test_table.allocate(456);

    CHECK_EQ(*test_table.get(item4), 456);

    CHECK_EQ(test_table.get(item1), nullptr);

    CHECK_EQ(test_table.get(item3), nullptr);

    CHECK_EQ(test_table.get({2, 1}), nullptr);
}
