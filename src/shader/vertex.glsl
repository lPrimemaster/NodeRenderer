#version 450
layout (location = 0) in vec3 pos;
layout (location = 1) in mat4 instanceModelMatrix;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

void main()
{
    gl_Position = projectionMatrix * viewMatrix * instanceModelMatrix * modelMatrix * vec4(pos, 1.0);
}
