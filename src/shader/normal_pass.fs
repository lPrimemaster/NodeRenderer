#version 450
in vec3 normal;
in vec4 iColorOut;
out vec4 f_color;


void main()
{
    gl_FragData[0] = vec4(abs(normal), 1.0);
    gl_FragData[1] = iColorOut;
}
