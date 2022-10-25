#pragma once
#include "raymarch_renderer.h"
#include "raster_renderer.h"

namespace Renderer
{
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

    void Init(GLFWwindow* window, const int sw, const int sh);
    void Render(GLFWwindow* window, NodeWindow* nodeWindow, AnalyticsWindow* analyticsWindow, OptionsWindow* optionsWindow);
    void CleanUp();
    void SetGlobalSceneMotif(const Vector3& motif);

    RasterRenderer::DrawList* GetRasterDrawList();

    struct ScreenRenderData
    {
        float screen_halfsize[2];
        float screen_size[2];
        bool viewport_changed;
        float mouse_delta[2];
        float mouse_scroll;
        int mouse_hold = 0;
        Camera::DirectionFlags cam_dir_f;
    };
}

