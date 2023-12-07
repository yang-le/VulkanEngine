#version 450

const vec3 gamma = vec3(2.2);
const vec3 inv_gamma = 1 / gamma;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

layout(binding = 4) uniform sampler2D u_texture_0;


void main() {
    vec3 tex_col = texture(u_texture_0, uv).rgb;
    // tex_col = pow(tex_col, gamma);

    // fog
    float fog_dist = gl_FragCoord.z / gl_FragCoord.w;
    float alpha = mix(0.5, 0.0, 1.0 - exp(-0.000002 * fog_dist * fog_dist));

    // gamma correction
    // tex_col = pow(tex_col, inv_gamma);
    fragColor = vec4(tex_col, alpha);
}
