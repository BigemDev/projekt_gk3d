#version 330

in vec2 interpTexCoord;
in vec2 interpAtlasCoord;
in vec4 interpNormal;
in vec4 shadowCoord;

uniform vec4 lightDir;
uniform sampler2DShadow shadowMap;
uniform sampler2D tex;
uniform sampler2D atlas;
uniform vec2 texOffset;

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
    vec4 color=texture(tex, interpTexCoord);
    
    fragColor = mix(texture(atlas, interpAtlasCoord), color,color.a)  * (ambient + diff * shadow);
}