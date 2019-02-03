#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 FragColor;

layout(std140, binding = 4) buffer buf {
  vec4 values[];
};

void main()
{
  if (gl_FragCoord.x < 128.0 && gl_FragCoord.y < 128.0) {
    FragColor = values[uint(gl_FragCoord.y + gl_FragCoord.x * 128.0)];
  } else {
    FragColor = vec4(texCoord, 0, 1);
  }
}
