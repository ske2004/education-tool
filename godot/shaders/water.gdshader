// Water for ponds, rivers and fountains: small moving ripples, a glint on
// ripple crests and a bit of reflection at grazing angles. Uses nothing
// the Compatibility renderer (web) lacks.
shader_type spatial;
render_mode cull_disabled;

uniform vec4 deep : source_color = vec4(0.16, 0.42, 0.66, 1.0);
uniform vec4 shallow : source_color = vec4(0.45, 0.75, 0.88, 1.0);
// Metres per second the ripples flow along the mesh's own z axis (rivers).
uniform float flow = 0.0;

vec2 ripples(vec2 p, float t) {
	// Sum of a few waves; returns the slope (d/dx, d/dy).
	vec2 slope = vec2(0.0);
	slope += vec2(1.7, 0.4) * cos(dot(p, vec2(1.7, 0.4)) + t * 1.3) * 0.12;
	slope += vec2(-0.6, 2.3) * cos(dot(p, vec2(-0.6, 2.3)) + t * 1.1) * 0.09;
	slope += vec2(3.1, 2.9) * cos(dot(p, vec2(3.1, 2.9)) - t * 1.9) * 0.04;
	slope += vec2(-4.3, 1.1) * cos(dot(p, vec2(-4.3, 1.1)) + t * 2.6) * 0.025;
	return slope;
}

void fragment() {
	// Ripples are laid out in world space, so they keep their size on any mesh.
	vec3 world = (INV_VIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
	vec2 along = normalize((MODEL_MATRIX * vec4(0.0, 0.0, 1.0, 0.0)).xz);
	vec2 p = world.xz - along * TIME * flow;
	vec2 slope = ripples(p * 1.8, TIME);
	NORMAL_MAP = normalize(vec3(-slope.x, -slope.y, 1.0)) * 0.5 + 0.5;
	NORMAL_MAP_DEPTH = 0.6;
	float facing = clamp(dot(NORMAL, VIEW), 0.0, 1.0);
	float fresnel = pow(1.0 - facing, 3.0);
	float glint = smoothstep(0.55, 0.68, length(slope)) * 0.2;
	// Shade the ripples into the colour too, so they show even when looking
	// straight down and on meshes without tangents.
	float shade = 1.0 + 0.35 * (slope.x + slope.y);
	ALBEDO = mix(deep.rgb, shallow.rgb, 0.35 + 0.4 * fresnel) * shade + glint;
	ROUGHNESS = 0.06;
	SPECULAR = 0.7;
	ALPHA = mix(0.82, 0.95, fresnel);
}
