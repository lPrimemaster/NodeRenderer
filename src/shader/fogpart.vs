#version 450
layout (location = 0) in vec3 pos;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform float time;
uniform float ymotifsize;

float rand2D(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    // TODO: Make this an uniform variable
    vec3 camPos = inverse(viewMatrix)[3].xyz;
    vec3 newPos = pos;
    newPos.y -= (time / 10.0);
    newPos.y = mod(newPos.y, ymotifsize);

    gl_PointSize = clamp(3.0 - (distance(camPos, pos.xyz)) / 3.0, 0.0, 3.0);
    gl_Position = projectionMatrix * viewMatrix * vec4(newPos, 1.0);
}
 