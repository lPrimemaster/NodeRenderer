#include "renderer.h"
#include <fstream>
#include <string>
#include "../log/logger.h"
#include "../../glm/glm/gtx/transform.hpp"
#include "nodes/render_node.h"
#include "../../imgui/imgui_impl_glfw.h"

static float screen_halfsize[2];
static float mouse_delta[2];
static float mouse_scroll;
static bool holding_mouse_right;
static Renderer::Camera::DirectionFlags cam_dir_f;

static void _key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    if(!io.WantCaptureKeyboard)
    {
        if(action == GLFW_PRESS)
        {
            if(key == GLFW_KEY_W) cam_dir_f |= Renderer::Camera::DirectionFlag_FORWARD;
            if(key == GLFW_KEY_A) cam_dir_f |= Renderer::Camera::DirectionFlag_LEFT;
            if(key == GLFW_KEY_S) cam_dir_f |= Renderer::Camera::DirectionFlag_BACKWARD;
            if(key == GLFW_KEY_D) cam_dir_f |= Renderer::Camera::DirectionFlag_RIGHT;
        }
        else if(action == GLFW_RELEASE)
        {
            if(key == GLFW_KEY_W) cam_dir_f &= ~Renderer::Camera::DirectionFlag_FORWARD;
            if(key == GLFW_KEY_A) cam_dir_f &= ~Renderer::Camera::DirectionFlag_LEFT;
            if(key == GLFW_KEY_S) cam_dir_f &= ~Renderer::Camera::DirectionFlag_BACKWARD;
            if(key == GLFW_KEY_D) cam_dir_f &= ~Renderer::Camera::DirectionFlag_RIGHT;
        }
    }
}

// TODO: Check if we are in the screen region or not
static void _mouse_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(xpos, ypos);

    if(holding_mouse_right)
    {
        mouse_delta[0] = screen_halfsize[0] - (float)xpos;
        mouse_delta[1] = screen_halfsize[1] - (float)ypos;
        glfwSetCursorPos(window, screen_halfsize[0], screen_halfsize[1]);
    }
    else
    {
        mouse_delta[0] = 0.0f;
        mouse_delta[1] = 0.0f;
    }
}

static void _mouse_scr_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    mouse_scroll = (float)yoffset;
}

static void _mouse_btn_callback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseButtonEvent(button, (action == GLFW_PRESS || action == GLFW_REPEAT));

    if (!io.WantCaptureMouse)
    {
        if(button == GLFW_MOUSE_BUTTON_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            holding_mouse_right = true;
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        {
            holding_mouse_right = false;
        }
    }
}

static void _framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    screen_halfsize[0] = width  / 2.0f;
    screen_halfsize[1] = height / 2.0f;
}

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

    if(!result)
    {
        L_ERROR("Program linking failed.");
        L_WARNING("%s", buffer);
        return 1;
    }

    glDeleteShader(cs);

    return 0;
}

static GLuint CreateDefaultProgram()
{
    GLuint program = glCreateProgram();

    int err = LoadShaderFromFile(GL_VERTEX_SHADER, "shader/vertex.glsl", &program);
    err |= LoadShaderFromFile(GL_FRAGMENT_SHADER, "shader/fragment.glsl", &program);

    if(err)
    {
        L_ERROR("Failed to create program.");
        glDeleteProgram(program);
    }

    return program;
}

Renderer::DrawInstance::DrawInstance()
{
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    static const float vertices[] = {
        -1, -1, -1,
         1, -1, -1,
         1,  1, -1,
        -1,  1, -1,
        -1, -1,  1,
         1, -1,  1,
         1,  1,  1,
        -1,  1,  1
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);

    static const unsigned int indices[] = {
        0, 1, 3, 3, 1, 2,
        1, 5, 2, 2, 5, 6,
        5, 4, 6, 6, 4, 7,
        4, 0, 7, 7, 0, 3,
        3, 2, 7, 7, 2, 6,
        4, 5, 0, 0, 5, 1
    };

    _idxcount = sizeof(indices) / sizeof(indices[0]);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    _instanceCount = 100;
    glGenBuffers(1, &_imb);
    glBindBuffer(GL_ARRAY_BUFFER, _imb);
    _intanceModelMatrixPtr = nullptr;
    glBufferData(GL_ARRAY_BUFFER, _instanceCount * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(1 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(2 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(3 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(4);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
}

Renderer::DrawInstance::~DrawInstance()
{
    L_TRACE("~DrawInstance()");
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
}

Renderer::Camera::Camera(float fov)
{
    // TODO: Update camera projection with viewport change and add camera control options
    projectionMatrix = glm::perspective(fov, aspectRatio, 0.1f, 1000.0f);

    position = glm::vec3(0, 0, 0);
    front = glm::vec3(0, 0, -1);
    up = glm::vec3(0, 1, 0);
    right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
    viewMatrix = glm::lookAt(position, front, up);

    modelMatrix = glm::mat4(1.0);
}

void Renderer::Camera::update(float frame_mouse_scroll, float frame_mouse_x, float frame_mouse_y, DirectionFlag dir, float dt)
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
    right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
    up    = glm::normalize(glm::cross(right, front));

    // Update zoom
    zoom -= frame_mouse_scroll;
    if(zoom < 1.0f)  zoom = 1.0f;
    if(zoom > 45.0f) zoom = 45.0f;

    viewMatrix = glm::lookAt(position, position + front, up);
}

Renderer::DrawList::DrawList(GLFWwindow* window, const int sw, const int sh)
{
    glfwSetKeyCallback(window, _key_callback);
    glfwSetCursorPosCallback(window, _mouse_pos_callback);
    glfwSetScrollCallback(window, _mouse_scr_callback);
    glfwSetMouseButtonCallback(window, _mouse_btn_callback);
    glfwSetFramebufferSizeCallback(window, _framebuffer_size_callback);

    screen_halfsize[0] = sw / 2.0f;
    screen_halfsize[1] = sh / 2.0f;

    _program = CreateDefaultProgram();
    addInstance(new DrawInstance());

    camera = new Camera(45.0f);

    glUseProgram(_program);
    _uniforms.projectionMatrix = glGetUniformLocation(_program, "projectionMatrix");
    _uniforms.viewMatrix = glGetUniformLocation(_program, "viewMatrix");
    _uniforms.modelMatrix = glGetUniformLocation(_program, "modelMatrix");
    
    glUniformMatrix4fv(_uniforms.projectionMatrix, 1, GL_FALSE, &camera->projectionMatrix[0][0]);
}

Renderer::DrawList::~DrawList()
{
    for(auto i : instances)
    {
        delete i;
    }

    delete camera;

    instances.clear();
    glDeleteProgram(_program);
}

void Renderer::DrawList::render(NodeWindow* nodeWindow)
{
    static float last_time = (float)glfwGetTime();
    RenderNode* outNode = dynamic_cast<RenderNode*>(nodeWindow->getRenderOutputNode());

    if(outNode != nullptr)
    {
        RenderNodeData nodeData = outNode->data.getValue<RenderNodeData>();

        if(nodeWindow->isRenderOutputNodeChanged())
        {
            RenderNodeData nodeData = outNode->data.getValue<RenderNodeData>();
            instances[0]->_instanceCount = nodeData._instanceCount;
            instances[0]->_intanceModelMatrixPtr = nodeData._worldPositionPtr;
        }

        if(outNode->renderDataChanged())
        {
            // TODO: Do something with the new data / init it in the graphics memory
            RenderNodeData nodeData = outNode->data.getValue<RenderNodeData>();
            instances[0]->_instanceCount = nodeData._instanceCount;
        }

        glm::mat4* pos = *(nodeData._worldPositionPtr);
        if(pos != nullptr)
        {
            glBindBuffer(GL_ARRAY_BUFFER, instances[0]->_imb);
            glBufferData(GL_ARRAY_BUFFER, instances[0]->_instanceCount * sizeof(glm::mat4), pos, GL_DYNAMIC_DRAW);
        }
    }

    // Update camera
    float now = (float)glfwGetTime();
    camera->update(mouse_scroll, mouse_delta[0], mouse_delta[1], cam_dir_f, now - last_time);
    last_time = now;

    glUseProgram(_program);
    // static float angle = 0.1f;
    // camera->modelMatrix = glm::rotate(camera->modelMatrix, angle, glm::vec3(-1, 1, 1));

    glUniformMatrix4fv(_uniforms.modelMatrix, 1, GL_FALSE, &camera->modelMatrix[0][0]);
    glUniformMatrix4fv(_uniforms.viewMatrix, 1, GL_FALSE, &camera->viewMatrix[0][0]);

    glBindVertexArray(instances[0]->_vao);
    glDrawElementsInstanced(GL_TRIANGLES, instances[0]->_idxcount, GL_UNSIGNED_INT, nullptr, instances[0]->_instanceCount);
    glBindVertexArray(0);
    glUseProgram(0);
}