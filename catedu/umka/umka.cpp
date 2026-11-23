#include "umka.hpp"
#include "catedu/core/alloc/allocator.hpp"
#include "catedu/ui/rendering/make_brush.hpp"
#include "catedu/ui/widgets.hpp"
#include "umka_api.h"
#include <cassert>
#include <cstring>

// HACK: For now Umka can't get userdata, so we have to store it globally,
// this is assigned on every call, so it is possible to have multiple instances.
thread_local UmkaBindingsData *_data;

static void on_umka_warning(UmkaError *warning)
{
    fprintf(stderr, "UMKA WARNING: %s\n", warning->msg);
}

static void umka_button(UmkaStackSlot *params, UmkaStackSlot *result)
{
    if (!_data || !_data->user)
    {
        if (result)
        {
            result->uintVal = 0;
        }
        return;
    }
    char *string = (char *)params[0].ptrVal;

    result->uintVal = button(*_data->user, string);
}

static void umka_label(UmkaStackSlot *params, UmkaStackSlot *result)
{
    if (!_data || !_data->user)
    {
        return;
    }
    // Color is in the format 0xRRGGBBAA
    uint32_t color = params[0].uintVal;
    char *string = (char *)params[1].ptrVal;

    label(*_data->user, string, {1, 1}, UiMakeBrush::make_solid(color));
}

static void umka_nextrow(UmkaStackSlot *params, UmkaStackSlot *result)
{
    if (!_data || !_data->user)
    {
        return;
    }
    _data->user->end_generic();
    AutoLayoutElement element = {};
    element.layout.type = AutoLayout::row;
    _data->user->begin_generic(element, {}, {});
}

static void umka_get_time_of_day(UmkaStackSlot *params, UmkaStackSlot *result)
{
    (void)params;
    if (!result)
    {
        return;
    }
    if (_data && _data->world)
    {
        result->realVal = (double)_data->world->time_of_day;
    }
    else
    {
        result->realVal = 0.0;
    }
}

static void umka_set_time_of_day(UmkaStackSlot *params, UmkaStackSlot *result)
{
    (void)result;
    if (_data && _data->world && params)
    {
        _data->world->time_of_day = (float)params[0].realVal;
    }
}

static void umka_has_item(UmkaStackSlot *params, UmkaStackSlot *result)
{
    if (!result)
    {
        return;
    }
    if (_data && _data->world && params && params[0].ptrVal)
    {
        const char *name = (const char *)params[0].ptrVal;
        result->intVal = _data->world->player_inventory.has_item(name, 1) ? 1 : 0;
    }
    else
    {
        result->intVal = 0;
    }
}

static void umka_add_item(UmkaStackSlot *params, UmkaStackSlot *result)
{
    (void)result;
    if (!_data || !_data->world || !params)
    {
        return;
    }

    // In Umka VM stack convention, fn add_item(name: str, amount: int) places
    // the last parameter (amount) at params[0] and the first (name) at params[1].
    // Also support direct C array slot ordering where params[0] is the pointer.
    const char *name = nullptr;
    int amount = 0;
    if ((uintptr_t)params[1].ptrVal > 0x10000 &&
        (uintptr_t)params[0].ptrVal <= 0x10000)
    {
        name = (const char *)params[1].ptrVal;
        amount = (int)params[0].intVal;
    }
    else if ((uintptr_t)params[0].ptrVal > 0x10000)
    {
        name = (const char *)params[0].ptrVal;
        amount = (int)params[1].intVal;
    }

    if (name && amount > 0)
    {
        _data->world->player_inventory.add_item(name, "", amount);
    }
}

void apply_bindings(void *umka)
{
    umkaAddFunc(umka, "button", &umka_button);
    umkaAddFunc(umka, "label", &umka_label);
    umkaAddFunc(umka, "nextrow", &umka_nextrow);
    umkaAddFunc(umka, "get_time_of_day", &umka_get_time_of_day);
    umkaAddFunc(umka, "set_time_of_day", &umka_set_time_of_day);
    umkaAddFunc(umka, "has_item", &umka_has_item);
    umkaAddFunc(umka, "add_item", &umka_add_item);
}

////////////////////////////////////////////////////////////////////////////////

UmkaModule UmkaModule::create(UmkaBindingsData *data)
{
    UmkaModule module = {};

    assert(data);
    module.data = data;

    return module;
}

void UmkaModule::destroy()
{
    ALLOCATOR_MALLOC.free(last_source_path);
    if (this->umka)
    {
        umkaFree(umka);
    }
}

bool UmkaModule::load(const char *source)
{
    assert(source && "Source cannot be null");

    if (source == nullptr)
    {
        return false;
    }

    clear_error();
    this->umka = umkaAlloc();

    // TODO: Better way to copy these strings
    ALLOCATOR_MALLOC.free(last_source_path);
    char *s = (char *)ALLOCATOR_MALLOC.alloc(strlen(source) + 1);
    memcpy(s, source, strlen(source) + 1);
    last_source_path = s;

    assert(umka);

    if (!umkaInit(umka, source, NULL, 1024 * 1024, NULL, 0, NULL, false, false,
                  on_umka_warning))
    {
        capture_error();
        return false;
    }

    apply_bindings(umka);

    if (!umkaCompile(umka))
    {
        capture_error();
        return false;
    }

    return true;
}

bool UmkaModule::reload()
{
    // Copy path before load() frees last_source_path (which source aliases)
    size_t len = strlen(last_source_path);
    char *path_copy = (char *)ALLOCATOR_MALLOC.alloc(len + 1);
    memcpy(path_copy, last_source_path, len + 1);

    umkaFree(umka);
    umka = nullptr;
    bool result = load(path_copy);
    ALLOCATOR_MALLOC.free(path_copy);
    return result;
}

UmkaError *UmkaModule::call(const char *module, const char *function,
                            size_t nparams, UmkaStackSlot *params,
                            UmkaStackSlot *result)
{
    int offset = umkaGetFunc(umka, module, function);

    if (offset < 0)
    {
        capture_error();
        return umkaGetError(umka);
    }

    UmkaStackSlot dummy;
    if (result == nullptr)
    {
        result = &dummy;
    }

    _data = data;
    assert(_data);

    clear_error();
    int call_status = umkaCall(umka, offset, nparams, params, result);

    _data = nullptr;

    if (call_status != 0 || !umkaAlive(umka))
    {
        capture_error();
    }

    return umkaGetError(umka);
}

UmkaError *UmkaModule::get_error()
{
    return umkaGetError(umka);
}

void UmkaModule::clear_error()
{
    has_error = false;
    error[0] = '\0';
}

void UmkaModule::capture_error()
{
    UmkaError *e = umka ? umkaGetError(umka) : nullptr;
    if (e == nullptr || e->msg == nullptr || e->msg[0] == '\0')
    {
        return;
    }

    has_error = true;
    snprintf(error, sizeof(error), "%s:%d:%d: %s",
             e->fileName ? e->fileName : "?", e->line, e->pos, e->msg);
    fprintf(stderr, "UMKA ERROR: %s\n", error);
}
