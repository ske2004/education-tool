#include "inventory.hpp"

void Inventory::add_item(const std::string& name, const std::string& desc, int amount) {
    for (auto& item : items) {
        if (item.name == name) {
            item.amount += amount;
            return;
        }
    }
    
    if (items.size() < (size_t)capacity) {
        items.push_back({name, desc, amount});
    }
}

bool Inventory::remove_item(const std::string& name, int amount) {
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it->name == name) {
            if (it->amount >= amount) {
                it->amount -= amount;
                if (it->amount == 0) {
                    items.erase(it);
                }
                return true;
            }
            return false;
        }
    }
    return false;
}

bool Inventory::has_item(const std::string& name, int amount) const {
    for (const auto& item : items) {
        if (item.name == name) {
            return item.amount >= amount;
        }
    }
    return false;
}
