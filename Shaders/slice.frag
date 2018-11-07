#version 450

layout(std140, binding = 0) buffer Octree {
  vec4 data[];
};

layout(location = 0) out vec4 FragColor;

void main() {
  FragColor = data[0];
}
