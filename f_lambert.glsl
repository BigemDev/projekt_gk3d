#version 330

in vec4 interpNormal;

uniform vec4 lightDir;
uniform vec4 color;

out vec4 fragColor;

void main() {
    vec3 L = normalize(lightDir.xyz);
    vec3 N = normalize(interpNormal.xyz);
    float diff = max(dot(N, L), 0.0);
    float ambient = 0.05;
    fragColor = color * (ambient + diff);
}