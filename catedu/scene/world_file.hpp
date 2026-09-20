#pragma once

#include "catedu/gui/editor/dispatcher.hpp"
struct WorldFile
{
    static bool save(const char *path, Dispatcher &dispatcher);
    static void export_json(const char *path, Dispatcher &dispatcher);
    static Dispatcher load(const char *path);
};
