#version 450
const uint maxMeshMerge = 2;

layout (location =  2) in mat4 instancePosMatrix;
layout (location =  6) in vec4 instanceColor;
layout (location =  7) in mat4 instanceRotMatrix;
layout (location = 11) in mat4 motifPosMatrix;

layout (location =  0) in vec3 posA;
layout (location =  1) in vec3 posB;
layout (location = 15) in vec3 nrmA;
layout (location = 16) in vec3 nrmB;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform uint meshCount = 1;

layout (location = 99) uniform float meshParam = 0.0;

out vec4 iColorOut;
out vec3 normal;

void main()
{
    iColorOut = instanceColor;

    // Calculate interpolated position and normal
    vec3 nrm = nrmA;
    vec3 pos = posA;
    if(meshCount > 1)
    {
        pos *= (1.0 - meshParam);
        pos += posB * meshParam;

        // nrm *= (1.0 - meshParam);
        // nrm += nrmB * meshParam;
    }

    normal = nrm;
    gl_Position = projectionMatrix * viewMatrix * motifPosMatrix * instancePosMatrix * instanceRotMatrix * vec4(pos, 1.0);
}
