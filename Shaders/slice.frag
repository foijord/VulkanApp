#version 450

layout(binding = 1) buffer Octree {
  vec4 nodes[];
} octree;

layout(location = 0) in vec3 position;
layout(location = 0) out vec4 FragColor;

vec3 origins[8] = {
  vec3(0, 0, 0),
  vec3(0, 0, 1),
  vec3(0, 1, 0),
  vec3(0, 1, 1),
  vec3(1, 0, 0),
  vec3(1, 0, 1),
  vec3(1, 1, 0),
  vec3(1, 1, 1)
};

void main() {
  int index = 0;
  vec3 pos = position;

  for (int i = 0; i < 8; i++) {
    int octant = 0;
    if (pos.x > 0.5) octant += 4;
    if (pos.y > 0.5) octant += 2;
    if (pos.z > 0.5) octant += 1;
    
    index = 8 * index + octant + 1;
    pos = (pos - origins[octant] * 0.5) * 2.0;
  }

  FragColor = octree.nodes[index];
}
