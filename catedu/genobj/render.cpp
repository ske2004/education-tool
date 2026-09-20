#include "render.hpp"
#include <math.h>
static Matrix4 box_to_matrix(Box3 box)
{
    Vector3 min = box.min;
    Vector3 max = box.max;

    return Matrix4::translate((min + max) * 0.5f) * Matrix4::scale_v(max - min);
}

static void apply_lighting(Params &params, float time_of_day)
{
    float t = time_of_day / 24.0f;
    float angle = t * 3.14159265f * 2.0f;
    
    float sun_x = cosf(angle);
    float sun_y = -sinf(angle);
    float sun_z = 0.5f;
    
    params.light_dir = {-sun_x, -sun_y, sun_z};
    
    float day_intensity = sinf(angle - 3.14159265f / 2.0f) * 0.5f + 0.5f;
    
    Vector4 day_light = {0.9f, 0.9f, 0.8f, 1.0f};
    Vector4 day_ambient = {0.6f, 0.6f, 0.8f, 1.0f};
    Vector4 night_light = {0.1f, 0.2f, 0.4f, 1.0f};
    Vector4 night_ambient = {0.05f, 0.05f, 0.15f, 1.0f};
    Vector4 sunset_light = {1.0f, 0.5f, 0.2f, 1.0f};
    Vector4 sunset_ambient = {0.3f, 0.2f, 0.3f, 1.0f};
    
    if (day_intensity > 0.5f) {
        float f = (day_intensity - 0.5f) * 2.0f;
        params.light_color = {
            sunset_light.x + (day_light.x - sunset_light.x) * f,
            sunset_light.y + (day_light.y - sunset_light.y) * f,
            sunset_light.z + (day_light.z - sunset_light.z) * f, 1.0f};
        params.ambient_color = {
            sunset_ambient.x + (day_ambient.x - sunset_ambient.x) * f,
            sunset_ambient.y + (day_ambient.y - sunset_ambient.y) * f,
            sunset_ambient.z + (day_ambient.z - sunset_ambient.z) * f, 1.0f};
    } else {
        float f = day_intensity * 2.0f;
        params.light_color = {
            night_light.x + (sunset_light.x - night_light.x) * f,
            night_light.y + (sunset_light.y - night_light.y) * f,
            night_light.z + (sunset_light.z - night_light.z) * f, 1.0f};
        params.ambient_color = {
            night_ambient.x + (sunset_ambient.x - night_ambient.x) * f,
            night_ambient.y + (sunset_ambient.y - night_ambient.y) * f,
            night_ambient.z + (sunset_ambient.z - night_ambient.z) * f, 1.0f};
    }
}

static void render_component(Renderer &renderer, const GenResources &resources,
                             const GeneratedObjectComponent &component,
                             Matrix4 matrix, float time_of_day)
{
    Params vs_params = {};
    vs_params.color_mul = component.color.to_vector4();
    vs_params.model = matrix * box_to_matrix(component.box);
    vs_params.lightness = 0;
    apply_lighting(vs_params, time_of_day);
    
    if (component.shaded)
    {
        renderer.render_model(resources.box_shaded, vs_params);
    }
    else
    {
        renderer.render_model(resources.box, vs_params);
    }
}

void genobj_render_object(Renderer &renderer, const GenResources &resources,
                          const GeneratedObject &object, Matrix4 matrix, float time_of_day)
{
    for (size_t i = 0; i < object.num_components; i++)
    {
        render_component(renderer, resources, object.components[i], matrix, time_of_day);
    }
}
