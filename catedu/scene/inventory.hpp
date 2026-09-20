#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Item {
    std::string name;
    std::string description;
    int amount;
};

struct Inventory {
    std::vector<Item> items;
    int capacity = 16;

    void add_item(const std::string& name, const std::string& desc, int amount);
    bool remove_item(const std::string& name, int amount);
    bool has_item(const std::string& name, int amount) const;
};
