#version 450 core
in vec2 texCoords;
out vec4 f_color;

uniform sampler2D inputTexture;

void main()
{
    f_color = texture(inputTexture, texCoords.st);
}
