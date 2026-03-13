#version 330

in vec4 interpNormal;
in vec4 shadowCoord;

uniform vec4 lightDir;
uniform vec4 color;
uniform sampler2DShadow shadowMap;

out vec4 fragColor;

void main() {
    vec3 L = normalize(lightDir.xyz);
    vec3 N = normalize(interpNormal.xyz);
    float diff = max(dot(N, L), 0.0);

    vec3 sc = shadowCoord.xyz / shadowCoord.w;
    sc = sc * 0.5 + 0.5;

    float bias = 0.005;
    sc.z -= bias;

    float shadow = texture(shadowMap, sc);

    float ambient = 0.3;
    fragColor = color * (ambient + diff * shadow);
}