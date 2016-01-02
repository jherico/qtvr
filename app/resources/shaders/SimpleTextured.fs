#version 330 core

uniform sampler2D sampler;
uniform float Alpha = 1.0;

in vec2 vTexCoord;
out vec4 vFragColor;

void main() {
    vec4 c = texture(sampler, vTexCoord);
    c.a = min(Alpha, c.a);
    vFragColor = c; 
}
