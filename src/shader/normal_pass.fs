#version 450
in vec3 normal;
in vec4 iColorOut;

layout (location = 0) out vec4 f_normal;
layout (location = 1) out vec4 f_diffuse;


void main()
{
    f_normal  = vec4(abs(normal), 1.0);
    f_diffuse = iColorOut;
}
