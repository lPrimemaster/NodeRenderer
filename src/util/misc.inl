#pragma once
#include <unordered_map>
#include <chrono>
#include <fstream>
#include <glad/glad.h>

namespace Utils
{
    inline int LoadShaderFromString(GLuint shadertype, std::string content, GLuint* program)
    {
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

        if(!result)
        {
            L_ERROR("Shader compilation failed.");
            L_WARNING("%s", buffer);
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

        int nrAttributes = -1;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
        L_TRACE("GL supported max vtx attributes: %d", nrAttributes);

        if(!result)
        {
            L_ERROR("Program linking failed.");
            L_WARNING("%s", buffer);
            return 1;
        }

        glDeleteShader(cs);

        return 0;
    }

    inline int LoadShaderFromFile(GLuint shadertype, std::string file, GLuint* program)
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

        return LoadShaderFromString(shadertype, content, program);
    }

    inline bool TriggerConditionAfterXSeconds(const bool* condition, float fseconds)
    {
        // printf("%x\n", condition);
        static std::unordered_map<const bool*, std::chrono::steady_clock::time_point> retvalmap;
        auto it = retvalmap.find(condition);
        if(it != retvalmap.end())
        {
            // L_DEBUG("Cond %u", *condition);
            if(
                *condition && (
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - it->second
                    ).count() > (unsigned int)(fseconds * 1000)
                )
            ) { return true; }

            if(!(*condition))
            {
                L_DEBUG("Erase");
                retvalmap.erase(condition);
            }

            return false;
        }

        if(*condition)
        {
            // Time it to flip retvalmap
            L_DEBUG("Create");
            retvalmap[condition] = std::chrono::steady_clock::now();
            return false;
        }

        return false;
    }
}