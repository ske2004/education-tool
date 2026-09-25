#pragma once
#include "catedu/core/storage/table.hpp"
#include "catedu/rendering/3d/model.hpp"

struct SpecModel
{
    const char *name;
    Model model;
    bool if_editor_only;
};

struct ResourceSpec
{
    Table<SpecModel> models;

    TableId find_model_by_name(const char *name);
    void deinit();
};
