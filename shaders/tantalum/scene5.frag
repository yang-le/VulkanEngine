#version 450
#include "tantalum/trace.frag"

#include "tantalum/bsdf.glsl"
#include "tantalum/intersect.glsl"
#include "tantalum/csg-intersect.glsl"

void intersect(Ray ray, inout Intersection isect) {
    bboxIntersect(ray, vec2(0.0), vec2(1.78, 1.0), 0.0, isect);
    planoConcaveLensIntersect(ray, vec2(0.8, 0.0), 0.6, 0.3, 0.6, 1.0, isect);
}

vec2 Sample(inout vec4 state, Intersection isect, float lambda, vec2 wiLocal, inout vec3 throughput) {
    if(isect.mat == 1.0) {
        return sampleMirror(wiLocal);
    } else {
        throughput *= vec3(0.5);
        return sampleDiffuse(state, wiLocal);
    }
}
