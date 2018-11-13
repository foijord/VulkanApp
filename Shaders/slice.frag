#version 450

layout(binding = 1) buffer Octree {
  int nodes[];
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

  int index = 0;
  vec3 pos = position;

  for (int i = 0; i < 9; i++) {
    if (i >= lod) break;
    int octant = 0;
    if (pos.x > 0.5) octant += 4;
    if (pos.y > 0.5) octant += 2;
    if (pos.z > 0.5) octant += 1;
    
    index = 8 * index + octant + 1;
    pos = (pos - origins[octant] * 0.5) * 2.0;
  }

  int octant = octree.nodes[index];
  FragColor = vec4(origins[octant], 1);
}
