#pragma once

#include <Innovator/Core/Misc/Defines.h>
#include <Innovator/Core/Math/Matrix.h>

#include <vector>

using namespace Innovator::Core::Math;

inline std::vector<uint32_t> data;

static void read_data()
{
  std::ifstream input("hackatree.hac_128", std::ios::binary);
  std::vector<char> bytes = std::vector<char>((std::istreambuf_iterator<char>(input)), 
    (std::istreambuf_iterator<char>()));

  data.resize(bytes.size());

  for (size_t i = 0; i < bytes.size(); i++) {
    data[i] = static_cast<uint32_t>(bytes[i]);
  }
}

struct OctreeNode {
  uint32_t data;
  uint32_t children[8];
};

static void populate_octree(std::vector<OctreeNode> & octree,
                            const uint32_t node_index,
                            const uint32_t subdivision,
                            const uint32_t num_subdivisions)
{
  if (subdivision == num_subdivisions) {
    return;
  }

  for (uint32_t i = 1; i <= 8; i++) {
    const uint32_t child_index = node_index * 8 + i;
    octree[node_index].children[i - 1] = child_index;
    octree[child_index].data = data[child_index];
    populate_octree(octree, child_index, subdivision + 1, num_subdivisions);
  }
}

static void populate_octree(std::vector<OctreeNode> & octree, 
                            const uint32_t num_levels)
{
  read_data();

  const uint32_t num_nodes = ((1 << (3 * num_levels)) - 1) / 7;
  octree.resize(num_nodes);
  octree[0].data = data[0];

  populate_octree(octree, 0, 0, num_levels - 1);
}
