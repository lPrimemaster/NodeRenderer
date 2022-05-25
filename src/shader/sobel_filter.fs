#version 450 core
in vec2 texCoords;
out vec4 f_color;

uniform sampler2D screenNormalTexture;
uniform sampler2D screenDiffuseTexture;
uniform sampler2D screenDepthTexture;

uniform float time;

uniform float fogMax = 50.0;
uniform float fogMin = 10.0;

const float zNear = 0.1;
const float zFar = 100.0;

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

float sobelNormal()
{
    mat3 I;
    // NOTE: Two passes - one for x and one for y might be faster
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

    return sqrt(pow(gx, 2.0) + pow(gy, 2.0));
}

float linearizeDepth(float z)
{
    // return zNear * zFar / (zFar + z * (zNear - zFar));
    return (2.0 * zNear) / (zFar + zNear - z * (zFar - zNear));  
}

float sobelDepth()
{
    mat3 I;
    // NOTE: Two passes - one for x and one for y might be faster
    for (int i=0; i<3; i++)
    {
        for (int j=0; j<3; j++)
        {
            float s = linearizeDepth(texelFetch(screenDepthTexture, ivec2(gl_FragCoord) + ivec2(i-1,j-1), 0).r);
            I[i][j] = s;
        }
    }

    float gx = dot(sx[0], I[0]) + dot(sx[1], I[1]) + dot(sx[2], I[2]); 
    float gy = dot(sy[0], I[0]) + dot(sy[1], I[1]) + dot(sy[2], I[2]);

    return sqrt(pow(gx, 2.0) + pow(gy, 2.0));
}

float fogFactor(float z)
{
    if(z >= fogMax) return 1.0;
    if(z <= fogMin) return 0.0;

    return 1.0 - (fogMax - z) / (fogMax - fogMin);
}

void main()
{
    float depthCameraDistance = linearizeDepth(texture(screenDepthTexture, texCoords.st).r) * zFar;
    float fog = fogFactor(depthCameraDistance);

    // No need to calculate edges on a fog occluded instance
    if(fog >= 1.0)
    {
        // discard;
        f_color = vec4(0.0);
        return;
    }

    float gn = sobelNormal();

    if(gn > 0.5)
    {
        f_color = vec4(0.0, 0.0, 0.0, 1.0);
    }
    else
    {
        float gd = sobelDepth();
        if(gd > 0.1)
        {
            f_color = vec4(0.0, 0.0, 0.0, 1.0);
        }
        else
        {
            f_color = texture(screenDiffuseTexture, texCoords.st);
        }
    }
    f_color.a = 1.0 - fog;
    // f_color = texture(screenDiffuseTexture, texCoords.st);
    // f_color = vec4(vec3(linearizeDepth(texelFetch(screenDepthTexture, ivec2(gl_FragCoord), 0).r)), 1.0);
}

// TODO: Fog color and density control