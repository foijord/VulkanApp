#version 450

layout(std140, binding = 0) uniform Transform {
  mat4 ModelViewMatrix;
  mat4 ProjectionMatrix;
};

layout(location = 0) in vec3 Position;
layout(location = 0) out vec2 texCoord;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() 
{
  texCoord = Position.xy;
  gl_Position = ProjectionMatrix * ModelViewMatrix * vec4(Position, 1.0);
}
