// blinnPhong

vec3 blinnPhong(vec3 color) {
    color = pow(color, gamma);

    vec3 ambient = 0.05 * color;

    vec3 lightDir = normalize(uLightPos);
    vec3 normal = normalize(vNormal);
    float diff = max(dot(lightDir, normal), 0.0);
    float light_atten_coff = uLightIntensity / pow(length(uLightPos - vFragPos), 2.0);
    vec3 diffuse = diff * light_atten_coff * color;

    vec3 viewDir = normalize(uCameraPos - vFragPos);
    vec3 halfDir = normalize((lightDir + viewDir));
    float spec = pow(max(dot(halfDir, normal), 0.0), 32.0);
    vec3 specular = uKs * light_atten_coff * spec;

    vec3 radiance = (ambient + diffuse + specular);
    vec3 phongColor = pow(radiance, inv_gamma);
    return phongColor;
}
