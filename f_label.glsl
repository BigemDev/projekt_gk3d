#version 330

in vec2 interpTexCoord;
uniform sampler2D tex;
out vec4 fragColor;

void main() {
    vec4 color = texture(tex, interpTexCoord);
    if (color.a < 0.1) discard;
    fragColor = color;
}