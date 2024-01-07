#version 440

vec2 VertexArray[] = {
  vec2(-1.f, -1.f),
  vec2(1.f, -1.f),
  vec2(-1.f, 1.f),
  vec2(1.f, -1.f),
  vec2(1.f, 1.f),
  vec2(-1.f, 1.f)
};

vec2 CoordArray[] = {
  vec2(0.f, 0.f),
  vec2(1.f, 0.f),
  vec2(0.f, 1.f),
  vec2(1.f, 0.f),
  vec2(1.f, 1.f),
  vec2(0.f, 1.f)
};

layout(location = 0) out vec2 outCoord;

void main()
{
  gl_Position = vec4(VertexArray[gl_VertexIndex], 0.f, 1.f);
  outCoord = CoordArray[gl_VertexIndex];
}

