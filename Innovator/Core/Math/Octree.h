#pragma once

#include <Innovator/Core/Misc/Defines.h>
#include <Innovator/Core/Math/Matrix.h>

#include <map>
#include <array>
#include <vector>

using namespace Innovator::Core::Math;

template <typename T>
static void populate_octree(std::vector<T> & octree, const size_t num_levels, const size_t index = 0, const size_t level = 0) {

  if (level == 0) {
    size_t num_nodes = 8;
    for (size_t i = 0; i < num_levels; i++) {
      num_nodes *= 8;
    }
    num_nodes = (num_nodes - 1) / 7;
    octree.resize(num_nodes);
    octree[0] = 0;
  }

  if (level < num_levels) {
    octree[index * 8 + 1] = 0;
    octree[index * 8 + 2] = 1;
    octree[index * 8 + 3] = 2;
    octree[index * 8 + 4] = 3;
    octree[index * 8 + 5] = 4;
    octree[index * 8 + 6] = 5;
    octree[index * 8 + 7] = 6;
    octree[index * 8 + 8] = 7;

    for (size_t i = 1; i <= 8; i++) {
      populate_octree(octree, num_levels, index * 8 + i, level + 1);
    }
  }
}

using Index = std::array<uint16_t, 4>;
using Key = uint64_t;

class OctreeNode {
public:
  explicit OctreeNode(Index index = { 0, 0, 0, 0 })
    : index(index) {}

  Index index;
};

class Octree {
public:
  NO_COPY_OR_ASSIGNMENT(Octree);

  Octree()
  {
    this->tree[0] = OctreeNode();
  }

  ~Octree() = default;

  static uint16_t IndexToOctant(const Index & index)
  {
    return ((index[0] % 2) * 4) + ((index[1] % 2) * 2) + ((index[2] % 2) * 1);
  }

  static std::array<Index, 8> getChildren(const Index & index)
  {
    const auto x = index[0];
    const auto y = index[1];
    const auto z = index[2];
    const auto d = index[3];

    return {
      Index{ x * 2u + 0u, y * 2u + 0u, z + 0u, d + 1u },
      Index{ x * 2u + 0u, y * 2u + 0u, z + 1u, d + 1u },
      Index{ x * 2u + 0u, y * 2u + 1u, z + 0u, d + 1u },
      Index{ x * 2u + 0u, y * 2u + 1u, z + 1u, d + 1u },
      Index{ x * 2u + 1u, y * 2u + 0u, z + 0u, d + 1u },
      Index{ x * 2u + 1u, y * 2u + 0u, z + 1u, d + 1u },
      Index{ x * 2u + 1u, y * 2u + 1u, z + 0u, d + 1u },
      Index{ x * 2u + 1u, y * 2u + 1u, z + 1u, d + 1u },
    };
  }

  uint64_t key(const Index & index)
  {
    return 0;
  }

  Index index(Key & key)
  {
    
  }


private:
  std::map<uint64_t, OctreeNode> tree;
};