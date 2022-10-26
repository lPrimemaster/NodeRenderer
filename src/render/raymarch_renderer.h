#pragma once
#include "../../glm/glm/glm.hpp"
#include "../windows/node_window.h"
#include "../windows/analytics_window.h"
#include "../windows/options_window.h"
#include "../math/vector.h"
#include "../util/misc.inl"

namespace Renderer
{
    struct ScreenRenderData;
    struct Camera;
}

namespace RayMarchRenderer
{
    // The raymarcher might need some kind of bvh inside the shader for complex scenes set by the user
    // Good to know we already have a bvh builder (for the CPU) for rasterized meshes interpolations

    // The idea of this rendering mode is to allow the user advanced control in writing its own raymarch shader

    // In the future we could use some shader graph like approach to draw some fancy stuff on the screen
    // For now just get the opengl uniforms / attributes the user sets in the custom shader and automatically create render node inputs for them
    
    // The raymarcher runs in a compute shader so rescaling is easier since we are not directly limited by the framebuffer's size
    // This has a little overhead cost of swapping glsl shader contexts
    struct RayMarchRendererDraw
    {
        RayMarchRendererDraw(GLFWwindow* window, Renderer::ScreenRenderData* screenRenderData, Renderer::Camera* camera);
        ~RayMarchRendererDraw();

        GLuint _program_render;
        GLuint _program_compute;

        GLuint _vao_screen;
        GLuint _vbo_screen;

        unsigned int screen_size_x;
        unsigned int screen_size_y;

        Renderer::ScreenRenderData* _screen_render_data;
        Renderer::Camera* camera;

        void reloadScreenTexture(GLsizei w, GLsizei h);
        void reloadShader(const std::string& code);

        // Screen texture
        GLuint _target_texture;

        // Automatic uniforms

        void render(GLFWwindow* window);
    };
};
