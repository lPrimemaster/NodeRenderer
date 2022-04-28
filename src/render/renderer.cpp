#include "renderer.h"
#include <fstream>
#include <string>
#include "../log/logger.h"

static int LoadShaderFromFile(GLuint shadertype, std::string file, GLuint* program)
{
    std::string content;
    std::ifstream f(file, std::ios::in);

    if(!f.is_open())
    {
        L_ERROR("Could not load file: %s", file.c_str());
        return 1;
    }

    std::string line;
    while(!f.eof())
    {
        std::getline(f, line);
        content.append(line + '\n');
    }

    f.close();

    GLuint cs = glCreateShader(shadertype);

    const char* const source = content.c_str();

    glShaderSource(cs, 1, &source, NULL);
    glCompileShader(cs);

    GLint result = GL_FALSE;
    int logLength;

    // Compile
    glGetShaderiv(cs, GL_COMPILE_STATUS, &result);
    glGetShaderiv(cs, GL_INFO_LOG_LENGTH, &logLength);
    char buffer[1024] = { '\0' };
    glGetShaderInfoLog(cs, logLength, NULL, buffer);
    L_DEBUG("%s", buffer);

    if(!result)
    {
        L_ERROR("Shader compilation failed.");
        return 1;
    }

    // Link
    if(program == nullptr || !glIsProgram(*program))
    {
        L_ERROR("Program is not valid.");
        return 1;
    }

    glAttachShader(*program, cs);
    glLinkProgram(*program);

    glGetProgramiv(*program, GL_LINK_STATUS, &result);
    glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &logLength);

    glGetProgramInfoLog(*program, logLength, NULL, buffer);
    L_DEBUG("%s", buffer);

    if(!result)
    {
        L_ERROR("Program linking failed.");
        return 1;
    }

    glDeleteShader(cs);

    return 0;
}

static GLuint CreateDefaultProgram()
{
    GLuint program = glCreateProgram();

    int err = LoadShaderFromFile(GL_VERTEX_SHADER, "src/shader/vertex.glsl", &program);
    err |= LoadShaderFromFile(GL_FRAGMENT_SHADER, "src/shader/fragment.glsl", &program);

    if(err)
    {
        L_ERROR("Failed to create program.");
        glDeleteProgram(program);
    }

    return program;
}

Renderer::DrawList::DrawList()
{
    _program = CreateDefaultProgram();
    addInstance(DrawInstance());
}

Renderer::DrawList::~DrawList()
{
    instances.clear();
    glDeleteProgram(_program);
}

void Renderer::DrawList::render()
{
    glUseProgram(_program);
    glBindVertexArray(instances[0]._vao);
    glDrawElements(GL_TRIANGLES, 2, GL_UNSIGNED_INT, nullptr);
    glUseProgram(0);
}