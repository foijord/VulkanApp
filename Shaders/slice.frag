#version 450

layout(binding = 1) uniform sampler2D Texture;
layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 FragColor;

void main() {
  FragColor = texture(Texture, texCoord);
}
