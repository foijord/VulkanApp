#version 450

layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec4 position;

void main() {
  vec3 dx = dFdx(position.xyz);
  vec3 dy = dFdy(position.xyz);
  vec3 n = normalize(cross(dy, dx));
  FragColor = vec4(n.zzz, 1.0);
}
