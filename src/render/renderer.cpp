#include "renderer.h"
#include <fstream>
#include <string>
#include "../log/logger.h"
#include "../../glm/glm/gtx/transform.hpp"
#include "nodes/render_node.h"
#include "../../imgui/imgui_impl_glfw.h"

#include "../util/objloader.h"

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
    io.AddMousePosEvent((float)xpos, (float)ypos);

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
    ImGuiIO& io = ImGui::GetIO();
    mouse_scroll = (float)yoffset;
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

    int err = LoadShaderFromFile(GL_VERTEX_SHADER, "shader/vertex.vs", &program);
    err |= LoadShaderFromFile(GL_FRAGMENT_SHADER, "shader/fragment.fs", &program);

    if(err)
    {
        L_ERROR("Failed to create program.");
        glDeleteProgram(program);
    }

    return program;
}

static GLuint CreateNormalPassProgram()
{
    GLuint program = glCreateProgram();

    int err = LoadShaderFromFile(GL_VERTEX_SHADER, "shader/normal_pass.vs", &program);
    err |= LoadShaderFromFile(GL_FRAGMENT_SHADER, "shader/normal_pass.fs", &program);

    if(err)
    {
        L_ERROR("Failed to create program.");
        glDeleteProgram(program);
    }

    return program;
}

static GLuint CreateSobelFilterProgram()
{
    GLuint program = glCreateProgram();

    int err = LoadShaderFromFile(GL_VERTEX_SHADER, "shader/sobel_filter.vs", &program);
    err |= LoadShaderFromFile(GL_FRAGMENT_SHADER, "shader/sobel_filter.fs", &program);

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

    static const float vtx_nrm[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f, 0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f, 0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f, 0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f, 0.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_nrm), vtx_nrm, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    _idxcount = 36;
    _instanceCount = 1;
    glGenBuffers(1, &_imb);
    glBindBuffer(GL_ARRAY_BUFFER, _imb);
    _intanceModelMatrixPtr = nullptr;
    glBufferData(GL_ARRAY_BUFFER, _instanceCount * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(1 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(2 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(3 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(5);

    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);

    glGenBuffers(1, &_icb);
    glBindBuffer(GL_ARRAY_BUFFER, _icb);
    _instanceColorsPtr = nullptr;
    glBufferData(GL_ARRAY_BUFFER, _instanceCount * sizeof(Vector4), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (void*)0);
    glEnableVertexAttribArray(6);

    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}

Renderer::DrawInstance::~DrawInstance()
{
    L_TRACE("~DrawInstance()");
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    // glDeleteBuffers(1, &_ebo);
    glDeleteBuffers(1, &_imb);
    glDeleteBuffers(1, &_icb);
}

Renderer::Camera::Camera(float fov)
{
    // TODO: Update camera projection with viewport change and add camera control options
    projectionMatrix = glm::perspective(fov, aspectRatio, 0.1f, 100.0f);

    position = glm::vec3(0, 0, 2);
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

    _program_nrmpass = CreateNormalPassProgram();
    _program_sobfilter = CreateSobelFilterProgram();
    
    addInstance(new DrawInstance());

    camera = new Camera(45.0f);

    glUseProgram(_program_nrmpass);
    _uniforms.projectionMatrix = glGetUniformLocation(_program_nrmpass, "projectionMatrix");
    _uniforms.viewMatrix = glGetUniformLocation(_program_nrmpass, "viewMatrix");
    _uniforms.modelMatrix = glGetUniformLocation(_program_nrmpass, "modelMatrix");
    
    glUniformMatrix4fv(_uniforms.projectionMatrix, 1, GL_FALSE, &camera->projectionMatrix[0][0]);

    glGenFramebuffers(1, &_rendertarget.framebuffer_id);
    glBindFramebuffer(GL_FRAMEBUFFER, _rendertarget.framebuffer_id);

    glGenTextures(3, _rendertarget.texid);
    glBindTexture(GL_TEXTURE_2D, _rendertarget.texid[0]);
    // TODO: Set texture size according to a given resolution
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sw, sh, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, _rendertarget.texid[1]);
    // TODO: Set texture size according to a given resolution
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sw, sh, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, _rendertarget.texid[2]);
    // TODO: Set texture size according to a given resolution
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, sw, sh, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    // NOTE: Color attachment is automatically present
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _rendertarget.texid[0], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, _rendertarget.texid[1], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _rendertarget.texid[2], 0);
    GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    glUseProgram(_program_sobfilter);
    glUniform1i(glGetUniformLocation(_program_sobfilter, "screenNormalTexture"), 0);
    glUniform1i(glGetUniformLocation(_program_sobfilter, "screenDiffuseTexture"), 1);
    glUniform1i(glGetUniformLocation(_program_sobfilter, "screenDepthTexture"), 2);

    glGenVertexArrays(1, &_vao_screen);
    glBindVertexArray(_vao_screen);
    glGenBuffers(1, &_vbo_screen);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo_screen);

    static const float vtx_uv[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_uv), vtx_uv, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

Renderer::DrawList::~DrawList()
{
    L_TRACE("~DrawList()");
    for(auto i : instances)
    {
        delete i;
    }

    delete camera;

    instances.clear();
    glDeleteProgram(_program_nrmpass);
    glDeleteProgram(_program_sobfilter);

    glDeleteVertexArrays(1, &_vao_screen);
    glDeleteBuffers(1, &_vbo_screen);

    glDeleteTextures(3, _rendertarget.texid);
    glDeleteFramebuffers(1, &_rendertarget.framebuffer_id);
}

void Renderer::DrawList::render(GLFWwindow* window, NodeWindow* nodeWindow)
{ 
    static float last_time = (float)glfwGetTime();
    RenderNode* outNode = dynamic_cast<RenderNode*>(nodeWindow->getRenderOutputNode());

    if(outNode != nullptr)
    {
        RenderNodeData nodeData = outNode->data.getValue<RenderNodeData>();

        if(nodeWindow->isRenderOutputNodeChanged())
        {
            // RenderNodeData nodeData = outNode->data.getValue<RenderNodeData>();
            instances[0]->_instanceCount = nodeData._instanceCount;
            instances[0]->_intanceModelMatrixPtr = nodeData._worldPositionPtr;
            instances[0]->_instanceColorsPtr = nodeData._instanceColorsPtr;
        }

        if(outNode->renderDataChanged())
        {
            // TODO: Do something with the new data / init it in the graphics memory
            // RenderNodeData nodeData = outNode->data.getValue<RenderNodeData>();
            instances[0]->_instanceCount = nodeData._instanceCount;

            // TODO: This is dumb, create a function and update it only when it changes
            MeshNodeData* mesh = *(nodeData._meshPtr);
            if(mesh != nullptr)
            {
                glBindBuffer(GL_ARRAY_BUFFER, instances[0]->_vbo);
                glBufferData(GL_ARRAY_BUFFER, mesh->data_size * sizeof(float), mesh->vertex_data, GL_STATIC_DRAW);
                instances[0]->_idxcount = (GLsizei)mesh->data_size / 6;
            }
        }

        glm::mat4* pos = *(nodeData._worldPositionPtr);
        if(pos != nullptr)
        {
            glBindBuffer(GL_ARRAY_BUFFER, instances[0]->_imb);
            glBufferData(GL_ARRAY_BUFFER, instances[0]->_instanceCount * sizeof(glm::mat4), pos, GL_DYNAMIC_DRAW);
        }

        Vector4* col = *(nodeData._instanceColorsPtr);
        if(col != nullptr)
        {
            glBindBuffer(GL_ARRAY_BUFFER, instances[0]->_icb);
            glBufferData(GL_ARRAY_BUFFER, instances[0]->_instanceCount * sizeof(Vector4), col, GL_DYNAMIC_DRAW);
        }

        if(nodeData._fogChanged)
        {
            // NOTE: Using glGetUniformLocation should be ok: this won't change that often and I'm lazy
            glUseProgram(_program_sobfilter);
            glUniform1f(glGetUniformLocation(_program_sobfilter, "fogMax"), nodeData._fogMax);
            glUniform1f(glGetUniformLocation(_program_sobfilter, "fogMin"), nodeData._fogMin);
            // fogcolor is background basically (no volumetric fog)
            glClearColor(nodeData._fogColor.x, nodeData._fogColor.y, nodeData._fogColor.z, 1.0f);
        }
    }

    // Update camera
    float now = (float)glfwGetTime();
    camera->update(mouse_scroll, mouse_delta[0], mouse_delta[1], cam_dir_f, now - last_time);
    last_time = now;

    // Render using normals to create an image to the sobel filter for edge detection
    glUseProgram(_program_nrmpass);
    glUniformMatrix4fv(_uniforms.modelMatrix, 1, GL_FALSE, &camera->modelMatrix[0][0]);
    glUniformMatrix4fv(_uniforms.viewMatrix, 1, GL_FALSE, &camera->viewMatrix[0][0]);
    glBindFramebuffer(GL_FRAMEBUFFER, _rendertarget.framebuffer_id);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // TODO: Add glViewport() here to the size of the framebuffer's texture
    glBindVertexArray(instances[0]->_vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, instances[0]->_idxcount, instances[0]->_instanceCount);

    // Render the filter calculated outlines into the screen alongside the diffuse data
    glUseProgram(_program_sobfilter);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(_vao_screen);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _rendertarget.texid[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _rendertarget.texid[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _rendertarget.texid[2]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glBindVertexArray(0);
    glUseProgram(0);
}