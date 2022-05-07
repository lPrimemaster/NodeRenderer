#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <unordered_map>
#include "../../glm/glm/glm.hpp"
#include "../windows/node_window.h"

namespace Renderer
{
    struct DrawInstance
    {
        DrawInstance();
        ~DrawInstance();

        GLuint _vao;
        GLuint _vbo;
        GLuint _ebo;
        GLsizei _idxcount;

        GLuint _imb;
        glm::mat4** _intanceModelMatrixPtr;

        unsigned int _instanceCount;
    };

    struct Camera
    {
        typedef int DirectionFlags;
        struct DirectionFlag
        {

            DirectionFlag(DirectionFlags flag)
            {
                _flags = flag;
            }
            union
            {
                struct
                {
                    unsigned int FORWARD  : 1;
                    unsigned int BACKWARD : 1;
                    unsigned int LEFT     : 1;
                    unsigned int RIGHT    : 1;
                };
                unsigned int _flags;
            };
        };

        enum DirectionFlag_
        {
            DirectionFlag_FORWARD  = 0x1,
            DirectionFlag_BACKWARD = 0x2,
            DirectionFlag_LEFT     = 0x4,
            DirectionFlag_RIGHT    = 0x8
        };

        Camera(float fov);

        static constexpr float aspectRatio = 16.0f/9.0f;

        void update(float frame_mouse_scroll, float frame_mouse_x, float frame_mouse_y, DirectionFlag dir, float dt);


        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;

    private:
        glm::vec3 position;
        glm::vec3 up;
        glm::vec3 right;
        glm::vec3 front;

        float yaw   = -90.0f;
        float pitch = 0.0f;
        float move_speed = 2.5f;
        float look_speed = 0.1f;
        float zoom = 45.0f; // NOTE: Not currently in use
    };

    struct DrawList
    {
        DrawList(GLFWwindow* window, const int sw, const int sh);
        ~DrawList();

        std::vector<DrawInstance*> instances;

        inline void addInstance(DrawInstance* instance)
        {
            instances.push_back(instance);
        }

        void render(NodeWindow* nodeWindow);

        GLuint _program;

        struct
        {
            GLuint projectionMatrix;
            GLuint viewMatrix;
            GLuint modelMatrix;
        } _uniforms;

        Camera* camera;
    };
}
