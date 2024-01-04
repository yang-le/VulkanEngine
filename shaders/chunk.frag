#version 450

#include "constants.glsl"

layout(location = 0) flat in int voxel_id;
layout(location = 1) flat in int face_id;
layout(location = 2) in vec2 uv;
layout(location = 3) in float shading;
layout(location = 4) in float frag_world_pos_y;

layout(binding = 3) uniform bg_color_t {
    vec3 bg_color;
};
layout(binding = 4) uniform sampler2DArray u_texture_array;

layout(location = 0) out vec4 fragColor;

void main() {
    vec2 face_uv = uv;
    face_uv.x = (min(face_id, 2) - uv.x) / 3.0;

    vec4 tex = texture(u_texture_array, vec3(face_uv, voxel_id));
    vec3 tex_col = tex.rgb;
    tex_col = pow(tex_col, gamma);

    tex_col *= shading;

    // underwater effect
    if(frag_world_pos_y < water_line)
        tex_col *= vec3(0.0, 0.3, 1.0);

    // fog
    float fog_dist = gl_FragCoord.z / gl_FragCoord.w;
    tex_col = mix(tex_col, bg_color, (1.0 - exp2(-0.00001 * fog_dist * fog_dist)));

    tex_col = pow(tex_col, inv_gamma);
    fragColor = vec4(tex_col, tex.a);
    // may be we need an OIT algorithm here to handle the alpha value
    // see https://github.com/KhronosGroup/Vulkan-Samples/blob/main/samples/api/oit_linked_lists/README.adoc
}
