#version 450

layout(location = 0) in vec3 vColor;

layout(location = 0) out vec4 outFragColor;

vec3 toneMapping(vec3 color) {
    vec3 result;
    for(int i = 0; i < 3; ++i) {
        if(color[i] <= 0.0031308)
            result[i] = 12.92 * color[i];
        else
            result[i] = (1.0 + 0.055) * pow(color[i], 1.0 / 2.4) - 0.055;
    }
    return result;
}

void main() {
    outFragColor = vec4(toneMapping(vColor), 1.0);
}
