#pragma once

#include "catedu/ui/user.hpp"
#include <umka_api.h>

struct UmkaBindingsData
{
    UiPass *user;
};

struct UmkaModule
{
    void *umka;
    char *last_source_path;
    UmkaBindingsData *data;
    bool has_error;
    char error[512];

    static UmkaModule create(UmkaBindingsData *data);
    void destroy();

    bool load(const char *path);
    bool reload();

    UmkaError *call(const char *module, const char *function, size_t nparams,
                    UmkaStackSlot *params, UmkaStackSlot *result = nullptr);

    UmkaError *get_error();

    // Fetches the current Umka error (if any) into has_error/error and logs it
    void capture_error();
    void clear_error();
};
