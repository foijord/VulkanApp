#version 450

layout(binding = 0) uniform Transform {
  mat4 ModelViewMatrix;
  mat4 ProjectionMatrix;
};

layout(location = 0) in vec3 Position;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() 
{
  vec3 position = Position * 2.0 - 1.0;
  gl_Position = ProjectionMatrix * ModelViewMatrix * vec4(position, 1.0);
}
