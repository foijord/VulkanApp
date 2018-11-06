#pragma once

#include <Innovator/Core/Misc/Defines.h>

#include <map>
#include <array>

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