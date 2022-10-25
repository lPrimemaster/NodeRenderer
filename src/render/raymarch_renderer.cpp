#include "raymarch_renderer.h"

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
    const std::string file = "compute.cs.template";
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
    // Load the template once
    static std::string template_compute_shader = LoadCSTemplateFile();

    // Copy the user code into the template
    std::string shader_code = template_compute_shader.replace(template_compute_shader.find("$user_code"), sizeof("$user_code") - 1, user_code);

    if(glIsProgram(*program))
    {
        L_DEBUG("Reloading compute shader program.");
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
}

RayMarchRenderer::RayMarchRendererDraw::RayMarchRendererDraw()
{
    _program_compute = 0;
    _program_render = CreateDefaultProgram();

    // Create an "empty screen" (blank) shader
    // Sets every pixel to white
    ReloadCSProgram("return vec4(1.0, 1.0, 1.0, 1.0);", &_program_compute);

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
}

RayMarchRenderer::RayMarchRendererDraw::~RayMarchRendererDraw()
{
    glDeleteProgram(_program_render);
    glDeleteProgram(_program_compute);
}

void RayMarchRenderer::RayMarchRendererDraw::reloadScreenTexture(GLsizei w, GLsizei h)
{
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
