#version 330 core

uniform samplerCube cubemap;

in vec3 texCoord;
out vec4 fragColor;

void main (void) {
  fragColor = texture(cubemap, texCoord);
}
