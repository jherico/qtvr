#version 330 core

uniform mat4 Projection = mat4(1);
uniform mat4 ModelView = mat4(1);
uniform vec2 UvMultiplier = vec2(1);

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 3) in vec4 Color;
layout(location = 5) in mat4 InstanceTransform;

out vec4 vColor;
out vec2 vTexCoord;

void main() {
  gl_Position = Projection * ModelView * vec4(Position, 1);
  vTexCoord = TexCoord * UvMultiplier;
  vColor = Color;
}
