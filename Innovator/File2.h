#pragma once

#include <Innovator/Nodes.h>
#include <Innovator/Math/Matrix.h>
#include <Innovator/Scheme/Scheme2.h>

using namespace Innovator::Math;

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <iterator>

std::shared_ptr<Node> sampler(const List &)
{
  return std::make_shared<Sampler>();
};

std::shared_ptr<Node> image(const List & lst)
{
  std::string filename = std::any_cast<std::string>(lst[0]);
  return std::make_shared<Image>(filename);
};

std::shared_ptr<Separator> separator(const List & lst)
{
  std::vector<std::shared_ptr<Node>> args = cast<std::shared_ptr<Node>>(lst);
  return std::make_shared<Separator>(args);
};

std::shared_ptr<Node> descriptorsetlayoutbinding(const List & lst)
{
  uint32_t binding = 1;
  VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  return std::make_shared<DescriptorSetLayoutBinding>(binding, descriptorType, stageFlags);
}

class File {
public:
  File() 
  {
    this->scheme.env->outer = std::make_shared<Env>();
    this->scheme.env->outer->inner = {
      { "separator", fun_ptr(separator) },
      { "sampler", fun_ptr(sampler) },
      { "image", fun_ptr(image) },
      { "descriptorsetlayoutbinding", fun_ptr(descriptorsetlayoutbinding) }
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