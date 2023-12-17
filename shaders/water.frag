#version 450

layout(location = 0) in vec2 uv;

layout(binding = 2) uniform sampler2D u_texture_0;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 tex_col = texture(u_texture_0, uv).rgb;

    // fog
    float fog_dist = gl_FragCoord.z / gl_FragCoord.w;
    float alpha = mix(0.5, 0.0, 1.0 - exp(-0.000002 * fog_dist * fog_dist));

    fragColor = vec4(tex_col, alpha);
}
