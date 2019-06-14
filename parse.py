import xml.etree.ElementTree as ET


def print_enums(enum_names):
    tree = ET.parse('vk.xml') # https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/master/xml/vk.xml
    root = tree.getroot()
    for enum in root.iter('enums'):
        if enum.attrib['name'] in enum_names:
            print(f'// {enum.attrib["name"]}')
            for child in enum:
                name = child.attrib['name']
                value = child.attrib.get('value') or child.attrib.get('bitpos')
                print(f'{{ "{name}", {value} }}')


if __name__ == "__main__":
    enum_names = ['VkFormat', 
                  'VkShaderStageFlagBits', 
                  'VkBufferUsageFlagBits', 
                  'VkVertexInputRate', 
                  'VkDescriptorType', 
                  'VkPrimitiveTopology', 
                  'VkIndexType']

    print_enums(enum_names)