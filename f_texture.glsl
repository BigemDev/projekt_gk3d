#version 330

in vec2 interpTexCoord;
uniform sampler2D tex;
out vec4 fragColor;

void main() {
    fragColor = texture(tex, interpTexCoord);
}