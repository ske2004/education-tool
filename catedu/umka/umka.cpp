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
    char *string = (char *)params[0].ptrVal;

    result->uintVal = button(*_data->user, string);
}

static void umka_label(UmkaStackSlot *params, UmkaStackSlot *result)
{
    // Color is in the format 0xRRGGBBAA
    uint32_t color = params[0].uintVal;
    char *string = (char *)params[1].ptrVal;

    label(*_data->user, string, {1, 1}, UiMakeBrush::make_solid(color));
}

static void umka_nextrow(UmkaStackSlot *params, UmkaStackSlot *result)
{
    _data->user->end_generic();
    AutoLayoutElement element = {};
    element.layout.type = AutoLayout::row;
    _data->user->begin_generic(element, {}, {});
}

void apply_bindings(void *umka)
{
    umkaAddFunc(umka, "button", &umka_button);
    umkaAddFunc(umka, "label", &umka_label);
    umkaAddFunc(umka, "nextrow", &umka_nextrow);
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
        return false;
    }

    apply_bindings(umka);

    if (!umkaCompile(umka))
    {
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
        return umkaGetError(umka);
    }

    UmkaStackSlot dummy;
    if (result == nullptr)
    {
        result = &dummy;
    }

    assert(_data->user);

    _data = data;

    // TODO: Somehow handle error codes from umkaCall
    umkaCall(umka, offset, nparams, params, result);

    _data = nullptr;

    return umkaGetError(umka);
}

UmkaError *UmkaModule::get_error()
{
    return umkaGetError(umka);
}
