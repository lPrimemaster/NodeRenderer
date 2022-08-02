#version 450
const uint maxMeshMerge = 2;

layout (location =  0) in vec3 posA;
layout (location =  1) in vec3 posB;
layout (location =  2) in vec3 nrmA;
layout (location =  3) in vec3 nrmB;

layout (location =  4) in vec4 instancePos;
layout (location =  5) in vec4 instanceColor;
layout (location =  6) in mat4 instanceRotMatrix;
layout (location = 10) in mat4 motifPosMatrix;


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

        nrm *= (1.0 - meshParam);
        nrm += nrmB * meshParam;
    }

    normal = nrm;
    gl_Position = projectionMatrix * viewMatrix * motifPosMatrix * (instancePos + (instanceRotMatrix * vec4(pos, 1.0)));
}
