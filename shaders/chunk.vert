#version 450

layout(location = 0) in uint packed_data;

layout(binding = 0) uniform m_proj_t { mat4 m_proj; };
layout(binding = 1) uniform m_view_t { mat4 m_view; };
layout(binding = 2) uniform m_model_t { mat4 m_model; };

layout(location = 0) out int voxel_id;
layout(location = 1) out int face_id;
layout(location = 2) out vec2 uv;
layout(location = 3) out float shading;
layout(location = 4) out float frag_world_pos_y;

const float ao_values[4] = float[4](0.1, 0.25, 0.5, 1.0);

const float face_shading[6] = float[6](
    1.0, 0.5,   // top bottom
    0.5, 0.8,   // right left
    0.5, 0.8    // front back
);

const vec2 uv_coords[4] = vec2[4](
    vec2(0, 0), vec2(0, 1),
    vec2(1, 0), vec2(1, 1)
);

const int uv_indices[24] = int[24](
    1, 0, 2, 1, 2, 3,   // tex coords indices for vertices of an even face
    3, 0, 2, 3, 1, 0,   // odd face
    3, 1, 0, 3, 0, 2,   // even flipped face
    1, 2, 3, 1, 0, 2    // odd flipped face
);

int x, y, z;
int ao_id;
int flip_id;

void unpack(uint packed_data) {
    // a, b, c, d, e, f, g = x, y, z, voxel_id, face_id, ao_id, flip_id
    uint b_bit = 6u, c_bit = 6u, d_bit = 8u, e_bit = 3u, f_bit = 2u, g_bit = 1u;
    uint b_mask = 63u, c_mask = 63u, d_mask = 255u, e_mask = 7u, f_mask = 3u, g_mask = 1u;
    //
    uint fg_bit = f_bit + g_bit;
    uint efg_bit = e_bit + fg_bit;
    uint defg_bit = d_bit + efg_bit;
    uint cdefg_bit = c_bit + defg_bit;
    uint bcdefg_bit = b_bit + cdefg_bit;
    // unpacking vertex data
    x = int(packed_data >> bcdefg_bit);
    y = int((packed_data >> cdefg_bit) & b_mask);
    z = int((packed_data >> defg_bit) & c_mask);
    //
    voxel_id = int((packed_data >> efg_bit) & d_mask);
    face_id = int((packed_data >> fg_bit) & e_mask);
    ao_id = int((packed_data >> g_bit) & f_mask);
    flip_id = int(packed_data & g_mask);
}

void main() {
    unpack(packed_data);

    int uv_index = gl_VertexIndex % 6 + ((face_id & 1) + flip_id * 2) * 6;
    uv = uv_coords[uv_indices[uv_index]];

    shading = face_shading[face_id] * ao_values[ao_id];

    vec4 in_position = m_model * vec4(x, y, z, 1.0);
    frag_world_pos_y = in_position.y;

    gl_Position = m_proj * m_view * in_position;
}
