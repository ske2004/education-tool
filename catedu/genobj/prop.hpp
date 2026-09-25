#pragma once
#include "generator.hpp"
#include <stddef.h>

// A prop drawn with a model from the resource spec instead of generated boxes.
struct PropModel
{
    const char *id;
    // Lifts the model so it rests on the ground.
    float y_offset;
};

extern const PropModel PROP_MODELS[];
extern const size_t PROP_MODEL_COUNT;

// Every prop ID the editor offers, generated props first.
extern const char *const PROP_IDS[];
extern const size_t PROP_ID_COUNT;

const PropModel *find_prop_model(const char *id);
bool genmesh_is_generated_prop(const char *id);
GeneratedObject genmesh_generate_prop(const char* id = nullptr);
