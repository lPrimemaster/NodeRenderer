#version 450 core
in vec2 texCoords;
out vec4 f_color;

uniform sampler2D screenNormalTexture;
uniform sampler2D screenDiffuseTexture;

const mat3 sx = mat3( 
    1.0, 2.0, 1.0, 
    0.0, 0.0, 0.0, 
   -1.0, -2.0, -1.0 
);
const mat3 sy = mat3( 
    1.0, 0.0, -1.0, 
    2.0, 0.0, -2.0, 
    1.0, 0.0, -1.0 
);

void main()
{
    mat3 I;
    // TODO: Two passes for x and y might be faster
    vec3 normal = texelFetch(screenNormalTexture, ivec2(gl_FragCoord), 0).rgb * 2.0 - 1.0;
    for (int i=0; i<3; i++)
    {
        for (int j=0; j<3; j++)
        {
            vec3 s = texelFetch(screenNormalTexture, ivec2(gl_FragCoord) + ivec2(i-1,j-1), 0).rgb * 2.0 - 1.0;
            I[i][j] = dot(normal, s);
        }
    }

    float gx = dot(sx[0], I[0]) + dot(sx[1], I[1]) + dot(sx[2], I[2]); 
    float gy = dot(sy[0], I[0]) + dot(sy[1], I[1]) + dot(sy[2], I[2]);

    float g = sqrt(pow(gx, 2.0) + pow(gy, 2.0));
    vec3 lineColor = vec3(g);
    vec3 screenDiffColor = texture(screenDiffuseTexture, texCoords.st).rgb;

    // This issues black lines
    f_color = vec4(screenDiffColor - lineColor, 1.0);
    // f_color = vec4(1.0, 0.0, 0.0, 1.0);
} 