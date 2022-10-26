#include "raymarch_renderer.h"
#include "renderer.h"

static GLuint CreateDefaultProgram()
{
    GLuint program = glCreateProgram();

    int err = Utils::LoadShaderFromFile(GL_VERTEX_SHADER, "shader/raymarch/vertex.vs", &program);
    err |= Utils::LoadShaderFromFile(GL_FRAGMENT_SHADER, "shader/raymarch/fragment.fs", &program);

    if(err)
    {
        L_ERROR("Failed to create program.");
        glDeleteProgram(program);
    }

    return program;
}

static std::string LoadCSTemplateFile()
{
    const std::string file = "shader/raymarch/compute.cs.template";
    std::string content;
    std::ifstream f(file, std::ios::in);

    if(!f.is_open())
    {
        L_ERROR("Could not load template file: %s", file.c_str());
        return "";
    }

    std::string line;
    while(!f.eof())
    {
        std::getline(f, line);
        content.append(line + '\n');
    }

    f.close();

    return content;    
}

static void ReloadCSProgram(const std::string& user_code, GLuint* program)
{
    // Load the template once (cached file)
    static std::string template_compute_shader = LoadCSTemplateFile();

    // Copy the user code into the template
    std::string shader_code = template_compute_shader;
    shader_code.replace(template_compute_shader.find("$user_code"), sizeof("$user_code") - 1, user_code);

    if(glIsProgram(*program))
    {
        L_DEBUG("Deleting compute shader.");
        glUseProgram(0); // Just make sure it is not active
        glDeleteProgram(*program);
    }

    *program = glCreateProgram();

    int err = Utils::LoadShaderFromString(GL_COMPUTE_SHADER, shader_code, program);

    if(err)
    {
        L_ERROR("Failed to create program.");
        glDeleteProgram(*program);
    }
    else
    {
        glUniform1i(glGetUniformLocation(*program, "inputTexture"), 0);
    }
}

RayMarchRenderer::RayMarchRendererDraw::RayMarchRendererDraw(GLFWwindow* window, Renderer::ScreenRenderData* screenRenderData, Renderer::Camera* camera)
{
    _screen_render_data = screenRenderData;
    this->camera = camera;
    screen_size_x = (unsigned int)screenRenderData->screen_size[0];
    screen_size_y = (unsigned int)screenRenderData->screen_size[1];

    _program_compute = 0;
    _program_render = CreateDefaultProgram();

    // Example from : https://www.shadertoy.com/view/4lSXzh
    static const char* example_run = 
    R"(
        mat2 rot(float th){ float cs = cos(th), si = sin(th); return mat2(cs, -si, si, cs); }

        float Voronesque( in vec3 p )
        {
            // Skewing the cubic grid, then determining the first vertex.
            vec3 i  = floor(p + dot(p, vec3(.333333)) );  p -= i - dot(i, vec3(.166666)) ;
            
            // Breaking the skewed cube into tetrahedra with partitioning planes, then determining which side of the 
            // intersecting planes the skewed point is on. Ie: Determining which tetrahedron the point is in.
            vec3 i1 = step(p.yzx, p), i2 = max(i1, 1. - i1.zxy); i1 = min(i1, 1. - i1.zxy);    
            
            // Using the above to calculate the other three vertices. Now we have all four tetrahedral vertices.
            vec3 p1 = p - i1 + .166666, p2 = p - i2 + .333333, p3 = p - .5;
            
            vec3 rnd = vec3(7, 157, 113); // I use this combination to pay homage to Shadertoy.com. :)
            
            // Falloff values from the skewed point to each of the tetrahedral points.
            vec4 v = max(0.5 - vec4(dot(p, p), dot(p1, p1), dot(p2, p2), dot(p3, p3)), 0.);
            
            // Assigning four random values to each of the points above. 
            vec4 d = vec4( dot(i, rnd), dot(i + i1, rnd), dot(i + i2, rnd), dot(i + 1., rnd) ); 
            
            // Further randomizing "d," then combining it with "v" to produce the final random falloff values. 
            // Range [0, 1]
            d = fract(sin(d)*262144.)*v*2.; 
        
            // Reusing "v" to determine the largest, and second largest falloff values. Analogous to distance.
            v.x = max(d.x, d.y), v.y = max(d.z, d.w), v.z = max(min(d.x, d.y), min(d.z, d.w)), v.w = min(v.x, v.y); 
        
            // Maximum minus second order, for that beveled Voronoi look. Range [0, 1].
            return  max(v.x, v.y) - max(v.z, v.w);  
            
            //return max(v.x, v.y); // Maximum, or regular value for the regular Voronoi aesthetic.  Range [0, 1].
        }

        vec4 render(in ivec2 pixel)
        {
            vec2 fragCoord = vec2(pixel);
            vec2 iResolution = vec2(1280.0, 720.0);
            float iTime = 1.0;
            // Screen coordinates, plus some movement about the center.
            vec2 uv = (fragCoord - iResolution.xy*0.5)/iResolution.y + vec2(.5*cos(iTime*.5), .25*sin(iTime*.5));
            
            // Unit direction ray.
            vec3 rd = normalize(vec3(uv, 1));
            rd.xy *= rot(sin(iTime*.25)*.5); // Very subtle look around, just to show it's a 3D effect.
            rd.xz *= rot(sin(iTime*.25)*.5);
        
            // Screen color. Initialized to black.
            vec3 col = vec3(0);
            
            // Ray intersection of a cylinder (radius one) - centered at the origin - from a ray-origin that has XY 
            // coordinates also centered at the origin.
            //float sDist = max(dot(rd.xy, rd.xy), 1e-16); // Analogous to the surface function.
            //sDist = 1./sqrt(sDist); // Ray origin to surface distance.
            
            // Same as above, but using a Minkowski distance and scaling factor. I tried it on a whim, and it seemed 
            // to work. I know, not scientific at all, but it kind of makes sense. They'll let anyone get behind a 
            // computer these days. :)
            vec2 scale = vec2(.75, 1);
            float power = 6.;
            // Analogous to the surface function.
            float sDist = max(dot( pow(abs(rd.xy)*scale, vec2(power)), vec2(1) ), 1e-16); 
            sDist = 1./pow( sDist, 1./power ); // Ray origin to surface distance.
                
            // Using the the distance "sDist" above to calculate the surface position. Ie: sp = ro + rd*t;
            // I've hardcoded "ro" to reduce line count. Note that "ro.xy" is centered on zero. The cheap
            // ray-intersection formula above relies on that.
            vec3 sp = vec3(0, 0, iTime*2.) + rd*sDist;
    
            // The surface normal. Based on the derivative of the surface description function. See above.
            //vec3 sn = normalize(vec3(-sp.xy, 0.)); // Cylinder normal.
            vec3 sn = normalize(-sign(sp)*vec3(pow(abs(sp.xy)*scale, vec2(power-1.)), 0.)); // Minkowski normal.
            
            // Bump mapping.
            //
            // I wanted to make this example as simple as possible, but it's only a few extra lines. Note the 
            // larger "eps" number. Increasing the value spreads the samples out, which effectively blurs the 
            // result, thus reducing the jaggies. The downside is loss of bump precision, which isn't noticeable 
            // in this particular example. Decrease the value to "0.001" to see what I'm talking about.
            const vec2 eps = vec2(.025, 0.);
            float c = Voronesque(sp*2.5); // Base value. Used below to color the surface.
            // 3D gradient vector... of sorts. Based on the bump function. In this case, Voronoi.                
            vec3 gr = (vec3(Voronesque((sp-eps.xyy)*2.5), Voronesque((sp-eps.yxy)*2.5), 
                            Voronesque((sp-eps.yyx)*2.5))-c)/eps.x;
            gr -= sn*dot(sn, gr); // There's a reason for this... but I need more room. :)
            sn = normalize(sn + gr*.1); // Combining the bump gradient vector with the object surface normal.

            // Lighting.
            //
            // The light is hovering just in front of the viewer.
            vec3 lp = vec3(0, 0, iTime*2. + 3.);
            vec3 ld = lp - sp; // Light direction.
            float dist = max(length(ld), .001); // Distance from light to the surface.
            ld /= dist; // Use the distance to normalize "ld."

            // Light attenuation, based on the distance above.
            float atten = min(1.5/max(1. + dist*.25 + dist*dist*.5, .001), 1.);
            
        
            float diff = max(dot(sn, ld), 0.); // Diffuse light value.
            float spec = pow(max(dot(reflect(-ld, sn), -rd), 0.), 16.); // Specular highlighting.
            // Adding some fake, reflective environment information.
            float ref = Voronesque((sp + reflect(rd, sn)*.5)*2.5);
        
            // Coloring the surface with the Voronesque function that is used to bump the surface. See 
            // "bump mapping" above.
            vec3 objCol = pow(min(vec3(1.5, 1, 1)*(c*.97 + .03), 1.), vec3(1, 3, 16)); // Cheap red palette.
            //vec3 objCol = vec3(c*c*.9, c, c*c*.4); // Cheap green palette.
            //vec3 objCol = vec3(pow(c, 1.6), pow(c, 1.7), c); // Purpley blue.
            //vec3 objCol = vec3(c); // Grey scale.

            // Using the values above to produce the final color.
            col = (objCol*(diff + ref*.35 + .25 + vec3(1, .9, .7)*spec) + (c + .35)*vec3(.25, .5, 1)*ref)*atten;
            
            // Rough gamma correction.
            return vec4(sqrt(clamp(col, 0., 1.)), 1);
        }
    )";

    sizeof(example_run);

    // Create an "empty screen" (blank) shader
    // Sets every pixel to magenta
    // ReloadCSProgram("return vec4(1.0, 0.0, 1.0, 1.0);", &_program_compute);
    ReloadCSProgram(example_run, &_program_compute);

    glCreateVertexArrays(1, &_vao_screen);
    glBindVertexArray(_vao_screen);
    glGenBuffers(1, &_vbo_screen);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo_screen);

    static const float vtx_uv[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_uv), vtx_uv, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    reloadScreenTexture(screen_size_x, screen_size_y);
}

RayMarchRenderer::RayMarchRendererDraw::~RayMarchRendererDraw()
{
    glDeleteProgram(_program_render);
    glDeleteProgram(_program_compute);
}

void RayMarchRenderer::RayMarchRendererDraw::reloadScreenTexture(GLsizei w, GLsizei h)
{
    glUseProgram(_program_compute);
    if(!glIsTexture(_target_texture))
    {
        glGenTextures(1, &_target_texture);
        glBindTexture(GL_TEXTURE_2D, _target_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        // Stay true to what the raymarcher produces
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, _target_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F); // Bind to location 0 (see compute.cs.template)
    glUseProgram(0);
}

void RayMarchRenderer::RayMarchRendererDraw::reloadShader(const std::string& code)
{
    ReloadCSProgram(code, &_program_compute);
}

void RayMarchRenderer::RayMarchRendererDraw::render(GLFWwindow* window)
{   
    glUseProgram(_program_compute);

    glDispatchCompute(screen_size_x, screen_size_y, 1);

    // Make sure raymarching is done before drawing
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glUseProgram(_program_render);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render
    glBindVertexArray(_vao_screen);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _target_texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}
