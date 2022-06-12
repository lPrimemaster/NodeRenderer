#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <unordered_map>
#include "../../glm/glm/glm.hpp"
#include "../windows/node_window.h"
#include "../windows/analytics_window.h"
#include "../windows/options_window.h"
#include "../math/vector.h"

namespace Renderer
{
    struct DrawInstance
    {
        DrawInstance();
        ~DrawInstance();

        void updateMotifInstanceForVertexArray();

        GLuint  _vao;
        GLuint  _vbo;
        GLuint  _ebo;
        GLsizei _idxcount;

        GLuint      _ipb;
        glm::mat4** _intancePositionMatrixPtr;

        GLuint      _irb;
        glm::mat4** _intanceRotationMatrixPtr;

        GLuint    _icb;
        Vector4** _instanceColorsPtr;

        GLuint      _mpb;
        glm::mat4** _motifPositionMatrixPtr;

        unsigned int _instanceCount;
        unsigned int _motif_span;
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
                    unsigned int UP       : 1;
                    unsigned int DOWN     : 1;
                };
                unsigned int _flags;
            };
        };

        enum DirectionFlag_
        {
            DirectionFlag_FORWARD  = 0x1,
            DirectionFlag_BACKWARD = 0x2,
            DirectionFlag_LEFT     = 0x4,
            DirectionFlag_RIGHT    = 0x8,
            DirectionFlag_UP       = 0x10,
            DirectionFlag_DOWN     = 0x20,
        };

        Camera(float fov);

        static constexpr float initialAspectRatio = 16.0f/9.0f;

        void update(float frame_mouse_scroll, float frame_mouse_x, float frame_mouse_y, DirectionFlag dir, float dt);

        inline Vector3 getPosition() const
        {
            return Vector3(position.x, position.y, position.z);
        }

        inline const glm::vec3& getPositionGLM() const
        {
            return position;
        }

        inline Vector3 getHeading() const
        {
            return Vector3(front.x, front.y, front.z);
        }


        inline void setPositionAndForwardVectorsAutomatic(const Vector3& p, const Vector3& f)
        {
            // This frame the position is auto
            is_automatic = true;

            position = glm::vec3(p.x, p.y, p.z);
            front    = glm::vec3(f.x, f.y, f.z);
        }

        inline bool isAutomatic() const
        {
            return is_automatic;
        }


        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;

    private:
        glm::vec3 position;
        glm::vec3 up;
        glm::vec3 right;
        glm::vec3 front;

        bool is_automatic = false;

        float yaw   = -90.0f;
        float pitch = 0.0f;
        float move_speed = 2.5f;
        float look_speed = 0.1f;
        float zoom = 45.0f; // NOTE: Not currently in use
        float fov;
    };

    struct DrawList
    {
        DrawList(GLFWwindow* window, const int sw, const int sh);
        ~DrawList();

        void render(GLFWwindow* window, NodeWindow* nodeWindow, AnalyticsWindow* analyticsWindow, OptionsWindow* optionsWindow);

        void updateFramebufferTextures();
        void updateFogParticlesMotifSize();

        struct
        {
            GLuint projectionMatrix;
            GLuint viewMatrix;

            GLuint fog_projectionMatrix;
            GLuint fog_viewMatrix;
            GLuint fog_time;
            GLuint fog_motifSize;

            GLuint sobel_time;
        } _uniforms;

        struct
        {
            GLuint framebuffer_id;
            GLuint texid[3];
        } _rendertarget;

        GLuint _program_nrmpass;
        GLuint _program_fogpart;
        GLuint _program_sobfilter;

        GLuint _vao_screen;
        GLuint _vbo_screen;

        GLuint _vao_fog;
        GLuint _vbo_fog;
        int _fog_density;
        GLuint _fog_perlin_texid;

        struct
        {
            GLuint framebuffer_id;
            GLuint texid;
        } _finalrender;

        Camera* camera;
        std::vector<DrawInstance*> instances;
        inline void addInstance(DrawInstance* instance)
        {
            instances.push_back(instance);
        }
    };
}
