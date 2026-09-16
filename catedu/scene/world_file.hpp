#pragma once

#include "catedu/gui/editor/dispatcher.hpp"
struct WorldFile
{
    static void spy(const char *path);
    static bool save(const char *path, Dispatcher &dispatcher);
    static Dispatcher load(const char *path);
};
