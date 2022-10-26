#include "renderer.h"
#include "../../glm/glm/gtx/transform.hpp"
#include "nodes/render_node.h"

static Renderer::Camera camera(45.0f);
static Renderer::ScreenRenderData screen_render_data;
constexpr static Vector3 infinityVec3 = Vector3(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
static Vector3 mcm_motif_size = infinityVec3;

static RasterRenderer::DrawList* raster_renderer = nullptr;
static RayMarchRenderer::RayMarchRendererDraw* raymarch_renderer = nullptr;

static void _key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    if(!io.WantCaptureKeyboard)
    {
        if(action == GLFW_PRESS)
        {
            if(key == GLFW_KEY_W)          screen_render_data.cam_dir_f |= Renderer::Camera::DirectionFlag_FORWARD;
            if(key == GLFW_KEY_A)          screen_render_data.cam_dir_f |= Renderer::Camera::DirectionFlag_LEFT;
            if(key == GLFW_KEY_S)          screen_render_data.cam_dir_f |= Renderer::Camera::DirectionFlag_BACKWARD;
            if(key == GLFW_KEY_D)          screen_render_data.cam_dir_f |= Renderer::Camera::DirectionFlag_RIGHT;
            if(key == GLFW_KEY_SPACE)      screen_render_data.cam_dir_f |= Renderer::Camera::DirectionFlag_UP;
            if(key == GLFW_KEY_LEFT_SHIFT) screen_render_data.cam_dir_f |= Renderer::Camera::DirectionFlag_DOWN;
        }
        else if(action == GLFW_RELEASE)
        {
            if(key == GLFW_KEY_W)          screen_render_data.cam_dir_f &= ~Renderer::Camera::DirectionFlag_FORWARD;
            if(key == GLFW_KEY_A)          screen_render_data.cam_dir_f &= ~Renderer::Camera::DirectionFlag_LEFT;
            if(key == GLFW_KEY_S)          screen_render_data.cam_dir_f &= ~Renderer::Camera::DirectionFlag_BACKWARD;
            if(key == GLFW_KEY_D)          screen_render_data.cam_dir_f &= ~Renderer::Camera::DirectionFlag_RIGHT;
            if(key == GLFW_KEY_SPACE)      screen_render_data.cam_dir_f &= ~Renderer::Camera::DirectionFlag_UP;
            if(key == GLFW_KEY_LEFT_SHIFT) screen_render_data.cam_dir_f &= ~Renderer::Camera::DirectionFlag_DOWN;
        }
    }
}

// FIXME: Check if we are in the screen region or not
static void _mouse_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    ImGuiIO& io = ImGui::GetIO();

    // GetMainViewport fix for multi viewports ImGui
    io.AddMousePosEvent((float)xpos + ImGui::GetMainViewport()->Pos.x, (float)ypos + ImGui::GetMainViewport()->Pos.y);
    
    // Not the best approach, but it is simple
    if(screen_render_data.mouse_hold > 1)
    {
        screen_render_data.mouse_delta[0] = 0.0f;
        screen_render_data.mouse_delta[1] = 0.0f;
        glfwSetCursorPos(window, screen_render_data.screen_halfsize[0], screen_render_data.screen_halfsize[1]);
        screen_render_data.mouse_hold--;
    }
    if(screen_render_data.mouse_hold == 1)
    {
        screen_render_data.mouse_delta[0] = screen_render_data.screen_halfsize[0] - (float)xpos;
        screen_render_data.mouse_delta[1] = screen_render_data.screen_halfsize[1] - (float)ypos;
        glfwSetCursorPos(window, screen_render_data.screen_halfsize[0], screen_render_data.screen_halfsize[1]);
    }
    else
    {
        screen_render_data.mouse_delta[0] = 0.0f;
        screen_render_data.mouse_delta[1] = 0.0f;
    }
}

static void _mouse_scr_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    screen_render_data.mouse_scroll = (float)yoffset;
    io.AddMouseWheelEvent(0, (float)yoffset);
}

static void _mouse_btn_callback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseButtonEvent(button, (action == GLFW_PRESS || action == GLFW_REPEAT));

    if (!io.WantCaptureMouse)
    {
        if(button == GLFW_MOUSE_BUTTON_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            screen_render_data.mouse_hold = 5;
            glfwSetCursorPos(window, screen_render_data.screen_halfsize[0], screen_render_data.screen_halfsize[1]);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        {
            screen_render_data.mouse_hold = 0;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

static void _framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    screen_render_data.screen_size[0] = (float)width;
    screen_render_data.screen_size[1] = (float)height;

    screen_render_data.screen_halfsize[0] = width  / 2.0f;
    screen_render_data.screen_halfsize[1] = height / 2.0f;

    screen_render_data.viewport_changed = true;
}

void Renderer::Init(GLFWwindow* window, const int sw, const int sh)
{
    glfwSetKeyCallback(window, _key_callback);
    glfwSetCursorPosCallback(window, _mouse_pos_callback);
    glfwSetScrollCallback(window, _mouse_scr_callback);
    glfwSetMouseButtonCallback(window, _mouse_btn_callback);
    glfwSetFramebufferSizeCallback(window, _framebuffer_size_callback);

    screen_render_data.screen_size[0] = sw;
    screen_render_data.screen_size[1] = sh;

    screen_render_data.screen_halfsize[0] = sw / 2.0f;
    screen_render_data.screen_halfsize[1] = sh / 2.0f;

    raster_renderer = new RasterRenderer::DrawList(window, &screen_render_data, &camera);

    raymarch_renderer = new RayMarchRenderer::RayMarchRendererDraw(window, &screen_render_data, &camera);
}

void Renderer::CleanUp()
{
    if(raster_renderer) delete raster_renderer;
    if(raymarch_renderer) delete raymarch_renderer;
}

void Renderer::SetGlobalSceneMotif(const Vector3& motif)
{
    mcm_motif_size = motif;
}

RasterRenderer::DrawList* Renderer::GetRasterDrawList()
{
    return raster_renderer;
}

void Renderer::Render(GLFWwindow* window, NodeWindow* nodeWindow, AnalyticsWindow* analyticsWindow, OptionsWindow* optionsWindow)
{
    static float last_time = (float)glfwGetTime();

    if(screen_render_data.viewport_changed)
    {
        raster_renderer->updateFramebufferTextures();
        raster_renderer->updateCameraPerspective();
        raymarch_renderer->reloadScreenTexture(screen_render_data.screen_size[0], screen_render_data.screen_size[1]);

        // Resize the node window
        nodeWindow->setWindowSize(ImVec2(screen_render_data.screen_size[0] - optionsWindow->getBarWidth(), screen_render_data.screen_size[1] / 3));
        nodeWindow->setWindowPos(ImVec2(optionsWindow->getBarWidth(), 0));

        analyticsWindow->setWindowSize(ImVec2(AnalyticsWindow::WIDTH, screen_render_data.screen_size[1] / 3.0f));

        if(analyticsWindow->isCollapsed())
        {
            analyticsWindow->setWindowPos(ImVec2(optionsWindow->getBarWidth(), screen_render_data.screen_size[1] - 19.0f));
            analyticsWindow->setCollapsedPosY(screen_render_data.screen_size[1] - 19.0f);
        }
        else
        {
            analyticsWindow->setWindowPos(ImVec2(optionsWindow->getBarWidth(), screen_render_data.screen_size[1] - analyticsWindow->getWindowSize().y));
            analyticsWindow->setCollapsedPosY(screen_render_data.screen_size[1] - 19.0f);
        }
    }

    // Update camera
    float now = (float)glfwGetTime();
    camera.update(screen_render_data.mouse_scroll, screen_render_data.mouse_delta[0], screen_render_data.mouse_delta[1], screen_render_data.cam_dir_f, now - last_time);
    last_time = now;


    RenderNode* outNode = dynamic_cast<RenderNode*>(nodeWindow->getRenderOutputNode());

    if(outNode != nullptr)
    {
        RenderNodeData nodeData = outNode->outputs[0]->getValue<RenderNodeData>();

        if(nodeData._reloadShader)
        {
            L_DEBUG("Reloading compute shader.");
            raymarch_renderer->reloadShader(nodeData._glslCode);
            outNode->outputs[0]->getValuePtr<RenderNodeData>()->_reloadShader = false;
        }

        // Pass execution to the preferred rendering mode
        switch (nodeData._renderMode)
        {
        case RenderNodeData::RenderMode::RASTER:
            raster_renderer->render(window, nodeWindow, analyticsWindow, optionsWindow);
            break;
        case RenderNodeData::RenderMode::RAYMARCH:
            raymarch_renderer->render(window);
            break;
        default:
            __assume(0);
        }
    }
    else
    {
        raster_renderer->render(window, nodeWindow, analyticsWindow, optionsWindow);
    }

    screen_render_data.viewport_changed = false;
}

Renderer::Camera::Camera(float fov) : fov(fov)
{
    projectionMatrix = glm::perspective(fov, initialAspectRatio, 0.1f, 100.0f);

    position = glm::vec3(0, 0, 2);
    front = glm::vec3(0, 0, -1);
    up = glm::vec3(0, 1, 0);
    right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
    viewMatrix = glm::lookAt(position, front, up);

    modelMatrix = glm::mat4(1.0);
}

void Renderer::Camera::update(float frame_mouse_scroll, float frame_mouse_x, float frame_mouse_y, DirectionFlag dir, float dt)
{
    if(screen_render_data.viewport_changed && screen_render_data.screen_size[0] > 0.1)
    {
        projectionMatrix = glm::perspective(fov, screen_render_data.screen_size[0] / screen_render_data.screen_size[1], 0.1f, 100.0f);
    }

    // Disable manual controls in automatic mode
    if(!is_automatic)
    {
        // Update position
        float v = move_speed * dt;
        if(dir.FORWARD)
        {
            position += front * v;
        }
        if(dir.BACKWARD)
        {
            position -= front * v;
        }
        if(dir.LEFT)
        {
            position -= right * v;
        }
        if(dir.RIGHT)
        {
            position += right * v;
        }
        if(dir.UP)
        {
            position += up * v;
        }
        if(dir.DOWN)
        {
            position -= up * v;
        }

        // Update look
        frame_mouse_x *= look_speed;
        frame_mouse_y *= look_speed;

        yaw   -= frame_mouse_x;
        pitch += frame_mouse_y;

        if(pitch > 89.0f) pitch = 89.0f;
        if(pitch < -89.0f) pitch = -89.0f;

        front = glm::normalize(
            glm::vec3(
                cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
                sin(glm::radians(pitch)),
                sin(glm::radians(yaw)) * cos(glm::radians(pitch))
            )
        );
    }

    right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
    up    = glm::normalize(glm::cross(right, front));

    // Update zoom
    zoom -= frame_mouse_scroll;
    if(zoom < 1.0f)  zoom = 1.0f;
    if(zoom > 45.0f) zoom = 45.0f;

    position.x = fmodf(position.x, mcm_motif_size.x);
    position.y = fmodf(position.y, mcm_motif_size.y);
    position.z = fmodf(position.z, mcm_motif_size.z);

    viewMatrix = glm::lookAt(position, position + front, up);
    is_automatic = false; // Next frame might be manual
}
