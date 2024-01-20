#version 450

layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputColour;
layout(input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputDepth;
layout(input_attachment_index = 2, set = 1, binding = 2) uniform subpassInput inputNormal;
layout(input_attachment_index = 3, set = 1, binding = 3) uniform subpassInput inputShadow;
layout(input_attachment_index = 4, set = 1, binding = 4) uniform subpassInput inputPos;

layout(location = 0) in vec4 vPosWorld;
layout(location = 1) in mat4 vWorldToScreen;

layout(binding = 3) uniform uLightDir_t {
    vec3 uLightDir;
};
layout(binding = 4) uniform uCameraPos_t {
    vec3 uCameraPos;
};
layout(binding = 5) uniform uLightRadiance_t {
    vec3 uLightRadiance;
};

layout(location = 0) out vec4 outColor;

#define M_PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307
#define INV_PI 0.31830988618
#define INV_TWO_PI 0.15915494309

float Rand1(inout float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

vec2 Rand2(inout float p) {
    return vec2(Rand1(p), Rand1(p));
}

float InitRand(vec2 uv) {
    vec3 p3 = fract(vec3(uv.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 SampleHemisphereUniform(inout float s, out float pdf) {
    vec2 uv = Rand2(s);
    float z = uv.x;
    float phi = uv.y * TWO_PI;
    float sinTheta = sqrt(1.0 - z * z);
    vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
    pdf = INV_TWO_PI;
    return dir;
}

vec3 SampleHemisphereCos(inout float s, out float pdf) {
    vec2 uv = Rand2(s);
    float z = sqrt(1.0 - uv.x);
    float phi = uv.y * TWO_PI;
    float sinTheta = sqrt(uv.x);
    vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
    pdf = z * INV_PI;
    return dir;
}

void LocalBasis(vec3 n, out vec3 b1, out vec3 b2) {
    float sign_ = sign(n.z);
    if(n.z == 0.0) {
        sign_ = 1.0;
    }
    float a = -1.0 / (sign_ + n.z);
    float b = n.x * n.y * a;
    b1 = vec3(1.0 + sign_ * n.x * n.x * a, sign_ * b, -sign_ * n.x);
    b2 = vec3(b, sign_ + n.y * n.y * a, -n.y);
}

vec4 Project(vec4 a) {
    return a / a.w;
}

float GetDepth(vec3 posWorld) {
    float depth = (vWorldToScreen * vec4(posWorld, 1.0)).w;
    return depth;
}

/*
 * Transform point from world space to screen space([0, 1] x [0, 1])
 *
 */
vec2 GetScreenCoordinate(vec3 posWorld) {
    vec2 uv = Project(vWorldToScreen * vec4(posWorld, 1.0)).xy * 0.5 + 0.5;
    return uv;
}

vec3 GetGBufferDiffuse() {
    vec3 diffuse = subpassLoad(inputColour).rgb;
    diffuse = pow(diffuse, vec3(2.2));
    return diffuse;
}

/*
 * Evaluate diffuse bsdf value.
 *
 * wi, wo are all in world space.
 *
 */
vec3 EvalDiffuse(vec3 wi, vec3 wo) {
    vec3 albedo = GetGBufferDiffuse();
    vec3 normal = subpassLoad(inputNormal).xyz;
    return INV_PI * albedo * max(0, dot(normal, wi));
}

/*
 * Evaluate directional light with shadow map
 *
 */
vec3 EvalDirectionalLight() {
    vec3 Le = subpassLoad(inputShadow).x * uLightRadiance;
    return Le;
}

bool RayMarch(vec3 ori, vec3 dir, out vec3 hitPos) {
    return false;
}

#define SAMPLE_NUM 1

void main() {
    float s = InitRand(gl_FragCoord.xy);

    vec3 wi = normalize(uLightDir);
    vec3 wo = normalize(uCameraPos - vPosWorld.xyz);

    vec3 L = EvalDiffuse(wi, wo) * EvalDirectionalLight();
    vec3 color = pow(clamp(L, vec3(0.0), vec3(1.0)), vec3(1.0 / 2.2));
    outColor = vec4(color, 1.0);
}
