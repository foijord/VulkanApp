#version 450

layout(location = 0) out vec2 texCoord;
 
void main()
{
  float x = -1.0 + float((gl_VertexIndex & 1) << 2);
  float y = -1.0 + float((gl_VertexIndex & 2) << 1);

  texCoord.x = x + 0.5 + 0.5;
  texCoord.y = y + 0.5 + 0.5;

  gl_Position = vec4(x, y, 0, 1);
}
