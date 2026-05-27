#version 330

layout(location=0) in vec4 vertex;
layout(location=2) in vec2 texCoord;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

out vec2 interpTexCoord;

void main() {
    interpTexCoord = texCoord;
    gl_Position = P * V * M * vertex;
}