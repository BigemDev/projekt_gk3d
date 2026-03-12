#version 330

layout(location=0) in vec4 vertex;
layout(location=1) in vec4 normal;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

out vec4 interpNormal;

void main() {
    interpNormal = normalize(M * normal);
    gl_Position = P * V * M * vertex;
}