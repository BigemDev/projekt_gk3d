#version 330

layout(location=0) in vec4 vertex;

uniform mat4 LP;
uniform mat4 LV;
uniform mat4 M;

void main() {
    gl_Position = LP * LV * M * vertex;
}