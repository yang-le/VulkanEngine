#version 450
#include "tantalum/preamble.glsl"
#include "tantalum/rand.glsl"

layout(binding = 0) uniform sampler2D RngData;
layout(binding = 1) uniform sampler2D Spectrum;
layout(binding = 2) uniform sampler2D Emission;
layout(binding = 3) uniform sampler2D ICDF;
layout(binding = 4) uniform sampler2D PDF;
layout(binding = 5) uniform EmitterPos_t {
    vec2 EmitterPos;
};
layout(binding = 6) uniform EmitterDir_t {
    vec2 EmitterDir;
};
layout(binding = 7) uniform EmitterPower_t {
    float EmitterPower;
};
layout(binding = 8) uniform SpatialSpread_t {
    float SpatialSpread;
};
layout(binding = 9) uniform AngularSpread_t {
    vec2 AngularSpread;
};

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragPosDir;
layout(location = 1) out vec4 fragRng;
layout(location = 2) out vec4 fragRgbLambda;

void main() {
    vec4 state = texture(RngData, vTexCoord);

    float theta = AngularSpread.x + (rand(state) - 0.5) * AngularSpread.y;
    vec2 dir = vec2(cos(theta), sin(theta));
    vec2 pos = EmitterPos + (rand(state) - 0.5) * SpatialSpread * vec2(-EmitterDir.y, EmitterDir.x);

    float randL = rand(state);
    float spectrumOffset = texture(ICDF, vec2(randL, 0.5)).r + rand(state) * (1.0 / 256.0);
    float lambda = 360.0 + (750.0 - 360.0) * spectrumOffset;
    vec3 rgb = EmitterPower * texture(Emission, vec2(spectrumOffset, 0.5)).r * texture(Spectrum, vec2(spectrumOffset, 0.5)).rgb / texture(PDF, vec2(spectrumOffset, 0.5)).r;

    fragPosDir = vec4(pos, dir);
    fragRng = state;
    fragRgbLambda = vec4(rgb, lambda);
}
