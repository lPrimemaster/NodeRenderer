#version 450
layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uvs;

uniform uint meshCount;
uniform uint meshSize;

layout (location = 99) uniform float meshParam;

out vec2 texCoords;

void main()
{
    texCoords = uvs;
    gl_Position = vec4(pos, 0.0, 1.0);
}
