#version 450

layout(location = 0) in vec2 vScreenUV;
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
layout(binding = 6) uniform sampler2D uGDiffuse;
layout(binding = 7) uniform sampler2D uGDepth;
layout(binding = 8) uniform sampler2D uGNormalWorld;
layout(binding = 9) uniform sampler2D uGShadow;
layout(binding = 10) uniform sampler2D uGPosWorld;

layout(binding = 12) uniform uSampleNum_t {
    int uSampleNum;
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

float GetGBufferDepth(vec2 uv) {
    float depth = texture(uGDepth, uv).x;
    if(depth < 1e-2) {
        depth = 1000.0;
    }
    return depth;
}

vec3 GetGBufferNormalWorld(vec2 uv) {
    vec3 normal = texture(uGNormalWorld, uv).xyz;
    return normal;
}

vec3 GetGBufferPosWorld(vec2 uv) {
    vec3 posWorld = texture(uGPosWorld, uv).xyz;
    return posWorld;
}

float GetGBufferuShadow(vec2 uv) {
    float visibility = texture(uGShadow, uv).x;
    return visibility;
}

vec3 GetGBufferDiffuse(vec2 uv) {
    vec3 diffuse = texture(uGDiffuse, uv).xyz;
    diffuse = pow(diffuse, vec3(2.2));
    return diffuse;
}

/*
 * Evaluate diffuse bsdf value.
 *
 * wi, wo are all in world space.
 * uv is in screen space, [0, 1] x [0, 1].
 *
 */
vec3 EvalDiffuse(vec3 wi, vec3 wo, vec2 uv) {
    vec3 albedo = GetGBufferDiffuse(uv);
    vec3 normal = GetGBufferNormalWorld(uv);
    return INV_PI * albedo * max(0, dot(normal, wi));
}

/*
 * Evaluate directional light with shadow map
 * uv is in screen space, [0, 1] x [0, 1].
 *
 */
vec3 EvalDirectionalLight(vec2 uv) {
    vec3 Le = GetGBufferuShadow(uv) * uLightRadiance;
    return Le;
}

bool RayMarch0(vec3 ori, vec3 dir, out vec3 hitPos) {
    float step = 0.05;
    const int totalStepTimes = 150;

    vec3 stepDir = normalize(dir) * step;
    vec3 curPos = ori;
    for(int curStepTimes = 0; curStepTimes < totalStepTimes; curStepTimes++) {
        vec2 screenUV = GetScreenCoordinate(curPos);
        float rayDepth = GetDepth(curPos);
        float gBufferDepth = GetGBufferDepth(screenUV);

        if(rayDepth - gBufferDepth > 0.0001) {
            hitPos = curPos;
            return true;
        }

        curPos += stepDir;
    }

    return false;
}

bool RayMarch(vec3 ori, vec3 dir, out vec3 hitPos) {
    const float EPS = 5e-2;
    const float threshold = 0.1;
    const int totalStepTimes = 20;

    float step = 0.8;   // initial step
    vec3 curPos = ori;
    for(int i = 0; i < totalStepTimes; i++) {
        vec3 nextPos = curPos + dir * step;
        vec2 uvScreen = GetScreenCoordinate(curPos);

        if(all(bvec4(lessThanEqual(vec2(0.0), uvScreen), lessThanEqual(uvScreen, vec2(1.0)))) &&
            GetDepth(nextPos) < GetGBufferDepth(GetScreenCoordinate(nextPos))) {
            curPos += dir * step;
            continue;
        }

        if(step < EPS) {
            float s1 = GetGBufferDepth(GetScreenCoordinate(curPos)) - GetDepth(curPos) + EPS;
            float s2 = GetDepth(nextPos) - GetGBufferDepth(GetScreenCoordinate(nextPos)) + EPS;
            if(s1 < threshold && s2 < threshold) {
                hitPos = curPos + 2 * dir * step * s1 / (s1 + s2);
                return true;
            }
            break;
        }

        step *= 0.5;
    }

    return false;
}

// test Screen Space Ray Tracing
vec3 EvalReflect(vec3 wi, vec3 wo, vec2 uv) {
    vec3 worldPos = GetGBufferPosWorld(uv);
    vec3 worldNormal = GetGBufferNormalWorld(uv);
    vec3 relfectDir = normalize(reflect(-wo, worldNormal));
    vec3 hitPos;
    if(RayMarch(worldPos, relfectDir, hitPos)) {
        vec2 screenUV = GetScreenCoordinate(hitPos);
        return GetGBufferDiffuse(screenUV);
    } else {
        return vec3(0.);
    }
}

#define SAMPLE_NUM uSampleNum

void main() {
    float s = InitRand(gl_FragCoord.xy);

    vec3 worldPos = GetGBufferPosWorld(vScreenUV);
    vec3 wi = normalize(uLightDir);
    vec3 wo = normalize(uCameraPos - worldPos);

    vec3 L = EvalDiffuse(wi, wo, vScreenUV) * EvalDirectionalLight(vScreenUV);

    // test Screen Space Ray Tracing
    // vec3 L = (GetGBufferDiffuse(screenUV) + EvalReflect(wi, wo, screenUV)) / 2.;

    vec3 b1, b2;
    vec3 normal = GetGBufferNormalWorld(vScreenUV);
    LocalBasis(normal, b1, b2);

    vec3 L_ind = vec3(0);
    for(int i = 0; i < SAMPLE_NUM; ++i) {
        float pdf;
        vec3 localDir = SampleHemisphereCos(s, pdf);
        vec3 dir = normalize(mat3(b1, b2, normal) * localDir);

        vec3 position_1;
        if(RayMarch(worldPos, dir, position_1)) {
            vec2 hitScreenUV = GetScreenCoordinate(position_1);
            L_ind += EvalDiffuse(dir, wo, vScreenUV) / pdf * EvalDiffuse(wi, dir, hitScreenUV) * EvalDirectionalLight(hitScreenUV);
        }
    }

    L_ind /= SAMPLE_NUM;

    L += L_ind;

    vec3 color = pow(clamp(L, vec3(0.0), vec3(1.0)), vec3(1.0 / 2.2));
    outColor = vec4(color, 1.0);
}
