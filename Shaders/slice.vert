#version 450

layout(binding = 0) uniform Transform {
  mat4 ModelViewMatrix;
  mat4 ProjectionMatrix;
};

layout(binding = 2) uniform Offset {
  vec3 offset;
};

layout(location = 0) in vec3 Position;
layout(location = 0) out vec3 texcoord;
layout(location = 1) out vec3 position2;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() 
{
  vec3 vertex = Position;
  vertex += offset;
  
  texcoord = vertex * 0.5 + 0.5;
  
  position2 = vec3(ModelViewMatrix * vec4(vertex, 1.0));
  gl_Position = ProjectionMatrix * ModelViewMatrix * vec4(vertex, 1.0);
}
