#version 450

layout (location = 0) out vec4 discarded_color_attch;
layout (location = 1) out vec4 f_diffuse;

void main()
{
    discarded_color_attch = vec4(0, 0, 0, 0);

    if(dot(gl_PointCoord-0.5,gl_PointCoord-0.5)>0.25)
        discard;
    else
        f_diffuse = vec4(1.0, 1.0, 1.0, 0.3);
}
