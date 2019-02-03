#version 450

layout(std140, binding = 0) uniform Transform {
  mat4 ModelViewMatrix;
  mat4 ProjectionMatrix;
};

layout(location = 0) in vec3 Position;
layout(location = 0) out vec3 vertex;

void main()
{
  vertex = Position;
  gl_Position = ProjectionMatrix * ModelViewMatrix * vec4(Position, 1.0);
}
