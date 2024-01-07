#version 440

layout(location = 0) in vec2 inCoord;

layout(location = 0) out vec4 OutColor;

layout(binding = 0, set = 0) uniform sampler2D inTexture;

void main()
{
  OutColor = texture(inTexture, inCoord);
}
