#pragma once

#include <Innovator/Nodes.h>
#include <Innovator/Scheme/Scheme.h>

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <utility>

using namespace scm;

template <typename BaseType, typename SubType, typename... Arg, std::size_t... i>
std::shared_ptr<BaseType> make_shared_object_impl(const List & lst, std::index_sequence<i...>)
{
  return std::make_shared<SubType>(std::any_cast<Arg>(lst[i])...);
}

template <typename BaseType, typename SubType, typename... Arg>
std::shared_ptr<BaseType> make_shared_object(const List & lst)
{
  return make_shared_object_impl<BaseType, SubType, Arg...>(lst, std::index_sequence_for<Arg...>{});
}

template <typename Type, typename... Arg, std::size_t... i>
Type make_object_impl(const List & lst, std::index_sequence<i...>)
{
  return Type(std::any_cast<Arg>(lst[i])...);
}

template <typename Type, typename... Arg>
Type make_object(const List & lst)
{
  return make_object_impl<Type, Arg...>(lst, std::index_sequence_for<Arg...>{});
}

template <typename NodeType, typename... Arg>
std::shared_ptr<Node> node(const List & lst)
{
  return make_shared_object<Node, NodeType, Arg...>(lst);
}

std::shared_ptr<Separator> separator(const List & lst)
{
  auto args = any_cast<std::shared_ptr<Node>>(lst);
  return std::make_shared<Separator>(args);
};

template <typename T>
std::shared_ptr<Node> bufferdata(const List & lst)
{
  auto bufferdata = std::make_shared<InlineBufferData<T>>();
  auto values = any_cast<Number>(lst);
  bufferdata->values.resize(values.size());
  std::transform(values.begin(), values.end(), bufferdata->values.begin(), [](double value) {
    return static_cast<T>(value);
  });
  return bufferdata;
}

uint32_t count(const List & list)
{
  auto node = std::any_cast<std::shared_ptr<Node>>(list[0]);
  auto bufferdata = std::dynamic_pointer_cast<BufferData>(node);
  if (!bufferdata) {
    throw std::invalid_argument("count only works on BufferData nodes!");
  }
  return static_cast<uint32_t>(bufferdata->size() / bufferdata->stride());
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

std::shared_ptr<Separator> eval_file(const std::string & filename)
{
  env_ptr env = scm::global_env();

  env->outer = std::make_shared<Env>(
    std::unordered_map<std::string, std::any>{
    { "int32", fun_ptr(make_object<int32_t, Number>) },
    { "uint32", fun_ptr(make_object<uint32_t, Number>) },
    { "count", fun_ptr(count) },
    { "image", fun_ptr(node<ImageNode, std::string>) },
    { "shader", fun_ptr(node<Shader, std::string, VkShaderStageFlagBits>) },
    { "sampler", fun_ptr(node<Sampler>) },
    { "separator", fun_ptr(separator) },
    { "bufferdata_float", fun_ptr(bufferdata<float>) },
    { "bufferdata_uint32", fun_ptr(bufferdata<uint32_t>) },
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
  });

  std::ifstream input(filename, std::ios::in);
  
  const std::string code{ 
    std::istreambuf_iterator<char>(input), 
    std::istreambuf_iterator<char>() 
  };

  std::any exp = scm::read(code);
  std::any sep = scm::eval(exp, env);
  return std::any_cast<std::shared_ptr<Separator>>(sep);
}
