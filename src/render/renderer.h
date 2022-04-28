#pragma once
#include <glad/glad.h>
#include <vector>
#include <unordered_map>

namespace Renderer
{
    struct DrawInstance
    {
        DrawInstance()
        {
            glGenVertexArrays(1, &_vao);
            glBindVertexArray(_vao);
            glGenBuffers(1, &_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, _vbo);

            static const float vertices[] = {
                -.5f, -.5f, -.5f,
                 .5f, -.5f, -.5f,
                -.5f,  .5f, -.5f,
                 .5f,  .5f, -.5f,

                -.5f, -.5f,  .5f,
                 .5f, -.5f,  .5f,
                -.5f,  .5f,  .5f,
                 .5f,  .5f,  .5f,
            };

            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glGenBuffers(1, &_ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);

            static const unsigned int indices[] = {
                0, 1, 2,
                2, 3, 1
            };

            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            _instanceCount = 1;
            glBindVertexArray(0);
        }

        ~DrawInstance()
        {
            glDeleteVertexArrays(1, &_vao);
            glDeleteBuffers(1, &_vbo);
            glDeleteBuffers(1, &_ebo);
        }

        GLuint _vao;
        GLuint _vbo;
        GLuint _ebo;
        unsigned int _instanceCount;
    };

    struct DrawList
    {
        DrawList();
        ~DrawList();

        std::vector<DrawInstance> instances;

        inline void addInstance(DrawInstance instance)
        {
            instances.push_back(instance);
        }

        void render();

        GLuint _program;
    };
}
