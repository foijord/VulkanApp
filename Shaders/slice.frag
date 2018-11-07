#version 450

layout(binding = 1) buffer Octree {
  vec4 nodes[1];
} octree;

layout(location = 0) out vec4 FragColor;

void main() {
  FragColor = octree.nodes[0];
}
