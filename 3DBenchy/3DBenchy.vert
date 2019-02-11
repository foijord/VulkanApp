#version 450

layout(std140, binding = 0) uniform Transform {
  mat4 ModelViewMatrix;
  mat4 ProjectionMatrix;
};

layout(location = 0) in vec3 Position;
layout(location = 0) out vec4 position;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() 
{
  position = ModelViewMatrix * vec4(Position, 1.0);
  gl_Position = ProjectionMatrix * position;
}
