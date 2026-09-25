#include "resource_spec.hpp"

void ResourceSpec::deinit()
{
    for (auto [id, spec_model] : iter(models))
    {
        spec_model.model.deinit();
    }
    models.deinit();
}

TableId ResourceSpec::find_model_by_name(const char *name)
{
    for (auto [id, model] : iter(models))
    {
        if (strcmp(name, model.name) == 0)
        {
            return id;
        }
    }

    return {};
}
