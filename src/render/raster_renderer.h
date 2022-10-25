#pragma once
#include <vector>
#include <unordered_map>
#include "../../glm/glm/glm.hpp"
#include "../windows/node_window.h"
#include "../windows/analytics_window.h"
#include "../windows/options_window.h"
#include "../math/vector.h"
#include "../util/misc.inl"

struct GLFWwindow;

namespace Renderer
{
    struct Camera;
    struct ScreenRenderData;
}

namespace RasterRenderer
{
    constexpr unsigned int MAX_MESH_MERGE = 2;

    struct DrawInstance
    {
        DrawInstance();
        ~DrawInstance();

        void updateMotifInstanceForVertexArray();

        GLuint  _vao;
        GLuint  _vbo[MAX_MESH_MERGE];
        GLuint  _ebo; // Unused
        GLsizei _idxcount;

        GLuint      _ipb;
        Vector4**   _intancePositionMatrixPtr;

        GLuint      _irb;
        glm::mat4** _intanceRotationMatrixPtr;

        GLuint      _icb;
        Vector4**   _instanceColorsPtr;

        GLuint      _mpb;
        Vector4**   _motifPositionMatrixPtr;

        unsigned int _instanceCount;
        unsigned int _motif_span;
    };

    struct DrawList
    {
        DrawList(GLFWwindow* window, const int sw, const int sh, Renderer::ScreenRenderData* screenRenderData, Renderer::Camera* camera);
        ~DrawList();

        void render(GLFWwindow* window, NodeWindow* nodeWindow, AnalyticsWindow* analyticsWindow, OptionsWindow* optionsWindow);

        void updateFramebufferTextures();
        void updateCameraPerspective();
        // void updateFogParticlesMotifSize();

        struct
        {
            GLuint projectionMatrix;
            GLuint viewMatrix;

            GLuint fog_projectionMatrix;
            GLuint fog_viewMatrix;
            GLuint fog_time;
            GLuint fog_motifSize;

            const GLuint all_meshParam = 99; // Special uniform value parameter

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

        Renderer::ScreenRenderData* _screen_render_data;
        Renderer::Camera* camera;

        std::vector<DrawInstance*> instances;
        inline void addInstance(DrawInstance* instance)
        {
            instances.push_back(instance);
        }
    };
}
