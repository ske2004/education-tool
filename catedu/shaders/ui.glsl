////////////////////////////////////////////////////////////////////////////////
@vs ui_vs
uniform ui_vs_params {
    mat4 mvp;
    vec4 color_top;
    vec4 color_bottom;
    vec4 color_mul;
    vec2 uv_min, uv_max;
    vec4 radius;
    vec4 gaps_x;
    vec4 gaps_y;
    vec4 filter_color_top;
    vec4 filter_color_bottom;
    float fliph;
};


in vec2 position;
in vec2 uv;
out vec4 color;
out vec4 frag_radius;
out vec4 frag_gaps_x;
out vec4 frag_gaps_y;
out vec2 frag_uv;
out vec2 frag_uv_min;
out vec2 frag_uv_max;

void main() {
    gl_Position = mvp * vec4(position, 0.0, 1.0);

    color = mix(color_top*filter_color_top, color_bottom*filter_color_bottom, uv.y);
    frag_radius = radius;
    frag_gaps_x = gaps_x;
    frag_gaps_y = gaps_y;
    if (fliph > 0.5) {
        frag_uv = vec2(uv.x, 1.0-uv.y);
    } else {
        frag_uv = uv;
    }
    frag_uv_min = uv_min;
    frag_uv_max = uv_max;
}

@end
////////////////////////////////////////////////////////////////////////////////
@fs ui_fs

uniform texture2D image;
uniform sampler image_sampler;
in vec4 color;
in vec4 frag_radius;
in vec4 frag_gaps_x;
in vec4 frag_gaps_y;
in vec2 frag_uv;
out vec4 frag_color;
in vec2 frag_uv_min;
in vec2 frag_uv_max;

@include math.glsl

void main() {
    vec2 uv = frag_uv * (frag_uv_max - frag_uv_min) + frag_uv_min;

    vec4 sample_value = texture(sampler2D(image, image_sampler), uv) * color;

    vec2 point = (frag_uv-vec2(0.5, 0.5))*2;

    if (superellipse_intersect_point(frag_radius, point, frag_gaps_x, frag_gaps_y)) {
        frag_color = sample_value;
    } else {
        frag_color = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
@end
////////////////////////////////////////////////////////////////////////////////
@program ui_prog ui_vs ui_fs
