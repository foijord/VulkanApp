#version 450

struct OctreeNode {
  uint data;
  uint children[8];
};

layout(binding = 1) buffer Octree {
  OctreeNode nodes[];
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

float mip_level(in vec3 texcoord) {
  vec3 dx = dFdx(texcoord);
  vec3 dy = dFdy(texcoord);
 
  float m = max(dot(dx, dx), dot(dy, dy));

  return 0.5 * log2(m);
}

void main() {

  int lod = 7 - int(mip_level(position * 512.0));

  lod = clamp(lod, 0, 8);

  uint node_index = 0;
  vec3 pos = position;

  for (int i = 0; i < 8; i++) {
    if (i >= lod) break;
    
    int child_index = 0;
    if (pos.x > 0.5) child_index += 4;
    if (pos.y > 0.5) child_index += 2;
    if (pos.z > 0.5) child_index += 1;
    
    // index = 8 * index + octant + 1;
    node_index = octree.nodes[node_index].children[child_index];
    pos = (pos - origins[child_index] * 0.5) * 2.0;
  }

  uint data = octree.nodes[node_index].data;
  FragColor = vec4(origins[data], 1);
}
