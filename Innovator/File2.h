#pragma once

#include <Innovator/Nodes.h>
#include <Innovator/Scheme/Scheme2.h>

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <utility>

template <typename NodeType, typename... Args, std::size_t... i>
std::shared_ptr<Node> node_impl(const List & lst, std::index_sequence<i...>)
{
  return std::make_shared<NodeType>(std::any_cast<Args>(lst[i])...);
}

template <typename NodeType, typename... Args>
std::shared_ptr<Node> node(const List & lst)
{
  return node_impl<NodeType, Args...>(lst, std::index_sequence_for<Args...>{});
}

std::shared_ptr<Separator> separator(const List & lst)
{
  auto args = any_cast<std::shared_ptr<Node>>(lst);
  return std::make_shared<Separator>(args);
};

template <typename T>
T number_t(const List & lst)
{
  return static_cast<T>(std::any_cast<Number>(lst[0]));
}

template <typename T>
std::any number_list_t(const List & lst)
{
  auto values = any_cast<Number>(lst);
  std::vector<T> result(values.size());
  std::transform(values.begin(), values.end(), result.begin(), [](double value) {
    return static_cast<T>(value);
  });
  return result;
}

std::shared_ptr<Node> bufferdata(const List & lst)
{
  if (lst[0].type() == typeid(std::vector<uint32_t>)) {
    return std::make_shared<InlineBufferData<uint32_t>>(std::any_cast<std::vector<uint32_t>>(lst[0]));
  }
  return std::make_shared<InlineBufferData<float>>(std::any_cast<std::vector<float>>(lst[0]));
}

VkBufferUsageFlags bufferusageflags(const List & lst)
{
  std::vector<VkBufferUsageFlagBits> flagbits = any_cast<VkBufferUsageFlagBits>(lst);
  VkBufferUsageFlags flags = 0;
  for (auto bit : flagbits) {
    flags |= bit;
  }
  return flags;
}

class File {
public:
  File() 
  {
    this->scheme.env->outer = std::make_shared<Env>();
    this->scheme.env->outer->inner = {
      { "int32", fun_ptr(number_t<int32_t>) },
      { "uint32", fun_ptr(number_t<uint32_t>) },
      { "list_f32", fun_ptr(number_list_t<float>) },
      { "list_ui32", fun_ptr(number_list_t<uint32_t>) },
      { "image", fun_ptr(node<Image, std::string>) },
      { "shader", fun_ptr(node<Shader, std::string, VkShaderStageFlagBits>) },
      { "sampler", fun_ptr(node<Sampler>) },
      { "separator", fun_ptr(separator) },
      { "bufferdata", fun_ptr(bufferdata) },
      { "bufferusageflags", fun_ptr(bufferusageflags) },
      { "cpumemorybuffer", fun_ptr(node<CpuMemoryBuffer, VkBufferUsageFlags>) },
      { "gpumemorybuffer", fun_ptr(node<GpuMemoryBuffer, VkBufferUsageFlags>) },
      { "transformbuffer", fun_ptr(node<TransformBuffer>) },
      { "indexeddrawcommand", fun_ptr(node<IndexedDrawCommand, uint32_t, uint32_t, uint32_t, int32_t, uint32_t, VkPrimitiveTopology>) },
      { "indexbufferdescription", fun_ptr(node<IndexBufferDescription, VkIndexType>) },
      { "descriptorsetlayoutbinding", fun_ptr(node<DescriptorSetLayoutBinding, uint32_t, VkDescriptorType, VkShaderStageFlagBits>) },
      { "vertexinputbindingdescription", fun_ptr(node<VertexInputBindingDescription, uint32_t, uint32_t, VkVertexInputRate>) },
      { "vertexinputattributedescription", fun_ptr(node<VertexInputAttributeDescription, uint32_t, uint32_t, VkFormat, uint32_t>) },

      { "VK_SHADER_STAGE_VERTEX_BIT", VK_SHADER_STAGE_VERTEX_BIT },
      { "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT },
      { "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT },
      { "VK_SHADER_STAGE_GEOMETRY_BIT", VK_SHADER_STAGE_GEOMETRY_BIT },
      { "VK_SHADER_STAGE_FRAGMENT_BIT", VK_SHADER_STAGE_FRAGMENT_BIT },
      { "VK_SHADER_STAGE_COMPUTE_BIT", VK_SHADER_STAGE_COMPUTE_BIT },

      { "VK_BUFFER_USAGE_TRANSFER_SRC_BIT", VK_BUFFER_USAGE_TRANSFER_SRC_BIT },
      { "VK_BUFFER_USAGE_TRANSFER_DST_BIT", VK_BUFFER_USAGE_TRANSFER_DST_BIT },
      { "VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT", VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT },
      { "VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT", VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT },
      { "VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT", VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT },
      { "VK_BUFFER_USAGE_STORAGE_BUFFER_BIT", VK_BUFFER_USAGE_STORAGE_BUFFER_BIT },
      { "VK_BUFFER_USAGE_INDEX_BUFFER_BIT", VK_BUFFER_USAGE_INDEX_BUFFER_BIT },
      { "VK_BUFFER_USAGE_VERTEX_BUFFER_BIT", VK_BUFFER_USAGE_VERTEX_BUFFER_BIT },
      { "VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT", VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT },

      { "VK_DESCRIPTOR_TYPE_SAMPLER", VK_DESCRIPTOR_TYPE_SAMPLER },
      { "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER", VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
      { "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE", VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
      { "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE", VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
      { "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER", VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER },
      { "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER", VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER },
      { "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
      { "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
      { "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC },
      { "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC },
      { "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT", VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT },

      { "VK_PRIMITIVE_TOPOLOGY_POINT_LIST", VK_PRIMITIVE_TOPOLOGY_POINT_LIST },
      { "VK_PRIMITIVE_TOPOLOGY_LINE_LIST", VK_PRIMITIVE_TOPOLOGY_LINE_LIST },
      { "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP", VK_PRIMITIVE_TOPOLOGY_LINE_STRIP },
      { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST },
      { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP },
      { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN },
      { "VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY", VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY },
      { "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY", VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY },
      { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY },
      { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY },
      { "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST", VK_PRIMITIVE_TOPOLOGY_PATCH_LIST },

      { "VK_INDEX_TYPE_UINT16", VK_INDEX_TYPE_UINT16 },
      { "VK_INDEX_TYPE_UINT32", VK_INDEX_TYPE_UINT32 },

      { "VK_VERTEX_INPUT_RATE_VERTEX", VK_VERTEX_INPUT_RATE_VERTEX },
      { "VK_VERTEX_INPUT_RATE_INSTANCE", VK_VERTEX_INPUT_RATE_INSTANCE },

      { "VK_FORMAT_R32_UINT", VK_FORMAT_R32_UINT },
      { "VK_FORMAT_R32_SINT", VK_FORMAT_R32_SINT },
      { "VK_FORMAT_R32_SFLOAT", VK_FORMAT_R32_SFLOAT },
      { "VK_FORMAT_R32G32_UINT", VK_FORMAT_R32G32_UINT },
      { "VK_FORMAT_R32G32_SINT", VK_FORMAT_R32G32_SINT },
      { "VK_FORMAT_R32G32_SFLOAT", VK_FORMAT_R32G32_SFLOAT },
      { "VK_FORMAT_R32G32B32_UINT", VK_FORMAT_R32G32B32_UINT },
      { "VK_FORMAT_R32G32B32_SINT", VK_FORMAT_R32G32B32_SINT },
      { "VK_FORMAT_R32G32B32_SFLOAT", VK_FORMAT_R32G32B32_SFLOAT },
      { "VK_FORMAT_R32G32B32A32_UINT", VK_FORMAT_R32G32B32A32_UINT },
      { "VK_FORMAT_R32G32B32A32_SINT", VK_FORMAT_R32G32B32A32_SINT },
      { "VK_FORMAT_R32G32B32A32_SFLOAT", VK_FORMAT_R32G32B32A32_SFLOAT }
    };
  }

  std::shared_ptr<Separator> open(const std::string & filename)
  {
    std::ifstream input(filename, std::ios::in);
    
    const std::string code{ 
      std::istreambuf_iterator<char>(input), 
      std::istreambuf_iterator<char>() 
    };

    std::any sep = this->scheme.eval(code);
    return std::any_cast<std::shared_ptr<Separator>>(sep);
  }

  Scheme scheme;
};