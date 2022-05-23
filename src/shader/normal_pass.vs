#version 450
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 nrm;
layout (location = 2) in mat4 instancePosMatrix;
layout (location = 6) in vec4 instanceColor;
layout (location = 7) in mat4 instanceRotMatrix;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix; // TODO: Remove

out vec4 iColorOut;
out vec3 normal;

void main()
{
    iColorOut = instanceColor;
    normal = nrm;
    gl_Position = projectionMatrix * viewMatrix * instancePosMatrix * instanceRotMatrix * modelMatrix * vec4(pos, 1.0);
}
