#version 330

layout(location=0) in vec4 vertex;
layout(location=1) in vec4 normal;
layout(location=2) in vec2 texCoord;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform mat4 LP;
uniform mat4 LV;
uniform vec2 texOffset;

out vec4 interpNormal;
out vec4 shadowCoord;
out vec2 interpTexCoord;
out vec2 interpAtlasCoord;

void main() {
    interpTexCoord = texCoord;
    interpAtlasCoord = (texCoord * 0.25) + texOffset;
    interpNormal = normalize(M * normal);
    shadowCoord = LP * LV * M * vertex;
    gl_Position = P * V * M * vertex;
}