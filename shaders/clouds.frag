#version 450

layout(location = 0) out vec4 fragColor;

void main() {
    const vec3 cloud_color = vec3(1);
    const vec3 bg_color = vec3(0.58, 0.83, 0.99);

    float fog_dist = gl_FragCoord.z / gl_FragCoord.w;
    vec3 col = mix(cloud_color, bg_color, 1.0 - exp(-0.000001 * fog_dist * fog_dist));

    fragColor = vec4(col, 0.8);
}
