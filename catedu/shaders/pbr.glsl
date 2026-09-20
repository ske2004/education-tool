////////////////////////////////////////////////////////////////////////////////
@vs pbr_vs
uniform pbr_vs_params {
    vec4 color_mul;
    mat4 model;
    mat4 viewproj;
    vec4 light_color;
    vec4 ambient_color;
    vec3 light_dir;
    float lightness;
};

in vec3 position;
in vec3 normal;
in vec2 uv;

out float frag_lightness;
out vec4 color;
out vec2 frag_uv;
out vec3 frag_normal;
out vec4 frag_pos;

void main() {
    frag_pos = viewproj * model * vec4(position, 1.0);

    vec3 transformed_normal = normalize((model * vec4(normal, 0.0)).xyz);
    float dp = max(0.0, dot(-normalize(light_dir), transformed_normal));
    vec4 all = vec4(dp, dp, dp, 1) * light_color + ambient_color;

    frag_lightness = lightness;

    color = color_mul*mix(all, vec4(1, 1, 1, 1), lightness);

    frag_uv = uv;
    frag_normal = normal;
    gl_Position = frag_pos;
}

@end
////////////////////////////////////////////////////////////////////////////////
@fs pbr_fs

uniform texture2D image;
uniform sampler image_sampler;
in float frag_lightness;
in vec4 color;
in vec2 frag_uv;
in vec3 frag_normal;
in vec4 frag_pos;
out vec4 frag_color;

void main() {
    vec4 sample_value = texture(sampler2D(image, image_sampler), frag_uv);

    // show fog
    float fog = smoothstep(0.6, 1.0, distance(vec3(0.0, 0.0, 0.0), frag_pos.xyz) / 200.0);
    fog = (1.0-frag_lightness)*fog;
    const vec4 fog_color = vec4(0.52549019607, 0.63921568627, 0.76862745098, 1.0);

    frag_color = mix(color, fog_color, fog) * sample_value;
}
@end
////////////////////////////////////////////////////////////////////////////////
@program pbr_prog pbr_vs pbr_fs
