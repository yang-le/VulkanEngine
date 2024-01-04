// blinnPhong

vec3 blinnPhong(
    vec3 Ka,
    float Ia,
    vec3 Kd,
    float Id,
    vec3 Ks,
    float Is,
    vec3 lightPos,
    vec3 eyePos,
    vec3 shadePos,
    vec3 normal,
    float alpha
) {
    vec3 ambient = Ka * Ia;
    vec3 lightVec = lightPos - shadePos;
    vec3 lightDir = normalize(lightVec);
    vec3 diffuse = Kd * Id * max(dot(lightDir, normal), 0);
    vec3 viewDir = normalize(eyePos - shadePos);
    vec3 halfDir = normalize(lightDir + viewDir);
    vec3 specular = Ks * Is * pow(max(dot(halfDir, normal), 0), alpha);
    return ambient + (diffuse + specular) / dot(lightVec, lightVec);
}
