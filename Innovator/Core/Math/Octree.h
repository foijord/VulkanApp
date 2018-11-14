#pragma once

#include <Innovator/Core/Misc/Defines.h>
#include <Innovator/Core/Math/Matrix.h>
#include <Innovator/Nodes.h>

#include <vector>

using namespace Innovator::Core::Math;

inline std::vector<int32_t> data;

static void read_data()
{
  std::ifstream input("seismic.hac_256", std::ios::binary);
  std::vector<char> bytes = std::vector<char>((std::istreambuf_iterator<char>(input)), 
    (std::istreambuf_iterator<char>()));

  data.resize(bytes.size());

  for (size_t i = 0; i < bytes.size(); i++) {
    data[i] = static_cast<int32_t>(bytes[i]);
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

static std::vector<OctreeNode> create_octree(const uint32_t num_levels)
{
  read_data();

  const uint32_t num_nodes = ((1 << (3 * num_levels)) - 1) / 7;
  std::vector<OctreeNode> octree(num_nodes);
  octree[0].data = data[0];

  populate_octree(octree, 0, 0, num_levels - 1);

  return octree;
}


inline std::shared_ptr<Separator> create_octree()
{
  //     5------7
  //    /|     /|
  //   / |    / |
  //  1------3  |
  //  |  4---|--6
  //  z x    | /
  //  |/     |/
  //  0--y---2

  std::vector<uint32_t> indices{
    1,  4,  0,
    4,  9,  0,
    4,  5,  9,
    8,  5,  4,
    1,  8,  4,
    1, 10,  8,
    10,  3, 8,
    8,  3,  5,
    3,  2,  5,
    3,  7,  2,
    3, 10,  7,
    10,  6, 7,
    6, 11,  7,
    6,  0, 11,
    6,  1,  0,
    10,  1, 6,
    11,  0, 9,
    2, 11,  9,
    5,  2,  9,
    11,  2, 7
  };
  
  const float t = float(1 + std::pow(5, 0.5)) / 2;  // golden ratio
  std::vector<float> vertices{
    -1,  0,  t,
     1,  0,  t,
    -1,  0, -t,
     1,  0, -t,
     0,  t,  1,
     0,  t, -1,
     0, -t,  1,
     0, -t, -1,
     t,  1,  0,
    -t,  1,  0,
     t, -1,  0,
    -t, -1,  0
  };


  return std::make_shared<Separator>(std::vector<std::shared_ptr<Node>>{

    std::make_shared<TransformBuffer>(),
    std::make_shared<DescriptorSetLayoutBinding>(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS),

    // wireframe cube outline
    std::make_shared<Separator>(std::vector<std::shared_ptr<Node>>{
      std::make_shared<GLSLShader>("Shaders/wireframe.vert", VK_SHADER_STAGE_VERTEX_BIT),
      std::make_shared<GLSLShader>("Shaders/wireframe.frag", VK_SHADER_STAGE_FRAGMENT_BIT),

      std::make_shared<BufferData<float>>(std::vector<float>({
        0, 0, 0, // 0
        0, 0, 1, // 1
        0, 1, 0, // 2
        0, 1, 1, // 3
        1, 0, 0, // 4
        1, 0, 1, // 5
        1, 1, 0, // 6
        1, 1, 1, // 7
        })),
      std::make_shared<CpuMemoryBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
      std::make_shared<GpuMemoryBuffer>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
      std::make_shared<VertexInputAttributeDescription>(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
      std::make_shared<VertexInputBindingDescription>(0, 3, VK_VERTEX_INPUT_RATE_VERTEX),

      std::make_shared<BufferData<uint32_t>>(std::vector<uint32_t>({ 0, 1, 3, 2, 0, 4, 6, 2, 3, 7, 6, 4, 5, 7, 3, 1, 5, 4, 0, 1 })),
      std::make_shared<CpuMemoryBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
      std::make_shared<GpuMemoryBuffer>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
      std::make_shared<IndexBufferDescription>(VK_INDEX_TYPE_UINT32),

      std::make_shared<IndexedDrawCommand>(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP)
    }),

    // slices
    std::make_shared<Separator>(std::vector<std::shared_ptr<Node>>{
      std::make_shared<GLSLShader>("Shaders/slice.vert", VK_SHADER_STAGE_VERTEX_BIT),
      std::make_shared<GLSLShader>("Shaders/slice.frag", VK_SHADER_STAGE_FRAGMENT_BIT),

      std::make_shared<BufferData<uint32_t>>(indices),
      std::make_shared<CpuMemoryBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
      std::make_shared<GpuMemoryBuffer>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
      std::make_shared<IndexBufferDescription>(VK_INDEX_TYPE_UINT32),

      std::make_shared<BufferData<float>>(vertices),
      std::make_shared<CpuMemoryBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
      std::make_shared<GpuMemoryBuffer>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
      std::make_shared<VertexInputAttributeDescription>(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
      std::make_shared<VertexInputBindingDescription>(0, 3, VK_VERTEX_INPUT_RATE_VERTEX),

      //std::make_shared<BufferData<uint32_t>>(std::vector<uint32_t>({ 0, 1, 7, 7, 6, 0 })),
      //std::make_shared<CpuMemoryBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
      //std::make_shared<GpuMemoryBuffer>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
      //std::make_shared<IndexBufferDescription>(VK_INDEX_TYPE_UINT32),
      //  
      //std::make_shared<BufferData<uint32_t>>(std::vector<uint32_t>({ 2, 3, 5, 5, 4, 2 })),
      //std::make_shared<CpuMemoryBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
      //std::make_shared<GpuMemoryBuffer>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
      //std::make_shared<IndexBufferDescription>(VK_INDEX_TYPE_UINT32),

      std::make_shared<BufferData<OctreeNode>>(create_octree(9)),
      std::make_shared<CpuMemoryBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
      std::make_shared<GpuMemoryBuffer>(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
      std::make_shared<DescriptorSetLayoutBinding>(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS),

      std::make_shared<IndexedDrawCommand>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
    }),
  });
}
