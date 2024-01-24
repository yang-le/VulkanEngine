// Random

// Shadow map related variables
#define NUM_SAMPLES 20
#define BLOCKER_SEARCH_NUM_SAMPLES NUM_SAMPLES
#define PCF_NUM_SAMPLES NUM_SAMPLES
#define NUM_RINGS 10

#define EPS 1e-3
#define PI 3.141592653589793
#define PI2 6.283185307179586

#define NEAR_PLANE 1e-2
#define FRUSTUM_SIZE 200.0f
#define SHADOW_MAP_SIZE 1600.0f

#define FILTER_RADIUS 10
#define FILTER_SIZE (FILTER_RADIUS / SHADOW_MAP_SIZE)

#define LIGHT_WORLD_SIZE 5
#define LIGHT_SIZE_UV (LIGHT_WORLD_SIZE / FRUSTUM_SIZE)

float rand_1to1(float x) {
  // -1 -1
    return fract(sin(x) * 10000.0);
}

float rand_2to1(vec2 uv) {
  // 0 - 1
    const float a = 12.9898, b = 78.233, c = 43758.5453;
    float dt = dot(uv.xy, vec2(a, b)), sn = mod(dt, PI);
    return fract(sin(sn) * c);
}

vec2 poissonDisk[NUM_SAMPLES];

void poissonDiskSamples(const in vec2 randomSeed) {

    float ANGLE_STEP = PI2 * float(NUM_RINGS) / float(NUM_SAMPLES);
    float INV_NUM_SAMPLES = 1.0 / float(NUM_SAMPLES);

    float angle = rand_2to1(randomSeed) * PI2;
    float radius = INV_NUM_SAMPLES;
    float radiusStep = radius;

    for(int i = 0; i < NUM_SAMPLES; i++) {
        poissonDisk[i] = vec2(cos(angle), sin(angle)) * pow(radius, 0.75);
        radius += radiusStep;
        angle += ANGLE_STEP;
    }
}

void uniformDiskSamples(const in vec2 randomSeed) {

    float randNum = rand_2to1(randomSeed);
    float sampleX = rand_1to1(randNum);
    float sampleY = rand_1to1(sampleX);

    float angle = sampleX * PI2;
    float radius = sqrt(sampleY);

    for(int i = 0; i < NUM_SAMPLES; i++) {
        poissonDisk[i] = vec2(radius * cos(angle), radius * sin(angle));

        sampleX = rand_1to1(sampleY);
        sampleY = rand_1to1(sampleX);

        angle = sampleX * PI2;
        radius = sqrt(sampleY);
    }
}

float findBlocker(sampler2D shadowMap, vec2 uv, float zReceiver) {
    uint blockerNum = 0;
    float blockerDepth = 0;
    float searchRadius = LIGHT_SIZE_UV * (zReceiver - NEAR_PLANE) / zReceiver;

    poissonDiskSamples(uv);
    for(int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; i++) {
        vec2 offset = poissonDisk[i] * searchRadius;
        float depth = texture(shadowMap, uv + offset).r;
        if(zReceiver > depth + EPS) {
            ++blockerNum;
            blockerDepth += depth;
        }
    }

    return blockerNum != 0 ? blockerDepth / blockerNum : -1;
}

float PCF(sampler2D shadowMap, vec4 coords, float filterSize) {
    poissonDiskSamples(coords.xy);
    uint visibility = 0;
    for(int i = 0; i < PCF_NUM_SAMPLES; i++) {
        vec2 offset = poissonDisk[i] * filterSize;
        float depth = texture(shadowMap, coords.st + offset).r;
        if(coords.z > depth + EPS)
            ++visibility;
    }
    return 1.0 - visibility / float(PCF_NUM_SAMPLES);
}

float PCSS(sampler2D shadowMap, vec4 coords) {
    // STEP 1: avgblocker depth
    float avgBlockerDepth = findBlocker(shadowMap, coords.st, coords.z);
    if(avgBlockerDepth < -EPS)
        return 1.0;

    // STEP 2: penumbra size
    float penumbra = LIGHT_SIZE_UV * (coords.z - avgBlockerDepth) / avgBlockerDepth;

    // STEP 3: filtering
    return PCF(shadowMap, coords, penumbra);
}

float useShadowMap(sampler2D shadowMap, vec4 shadowCoord) {
    float depth = texture(shadowMap, shadowCoord.st).r;
    return (shadowCoord.z > depth + EPS) ? 0.0 : 1.0;
}
