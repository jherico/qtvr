#version 330 core

in vec4 vColor;
in vec3 vPos;

out vec4 FragColor;

void main() {
  vec3 corner = abs(abs(vPos) - vec3(0.5, 0.5, 0.5));
  if ((corner.x <= 0.025 && corner.y <= 0.025 && corner.z > 0.025)
      || (corner.x <= 0.025 && corner.y > 0.025 && corner.z <= 0.025)
      || (corner.x > 0.025 && corner.y <= 0.025 && corner.z <= 0.025)
      || length(corner) <= 0.05) {
    FragColor = vec4(0, 0, 0, 1);
  } else {
    FragColor = vColor;
  }
}
