#include "renderer.h"
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include "../log/logger.h"
#include "../../glm/glm/gtx/transform.hpp"
#include "nodes/render_node.h"
#include "../../imgui/imgui_impl_glfw.h"

#include "../util/objloader.h"
#include "../util/perlin.inl"

static float screen_halfsize[2];
static float screen_size[2];
static bool viewport_changed;
static float mouse_delta[2];
static float mouse_scroll;
static bool holding_mouse_right;
static Renderer::Camera::DirectionFlags cam_dir_f;
constexpr static Vector3 infinityVec3 = Vector3(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
static Vector3 mcm_motif_size = infinityVec3;

static void _key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    if(!io.WantCaptureKeyboard)
    {
        if(action == GLFW_PRESS)
        {
            if(key == GLFW_KEY_W)          cam_dir_f |= Renderer::Camera::DirectionFlag_FORWARD;
            if(key == GLFW_KEY_A)          cam_dir_f |= Renderer::Camera::DirectionFlag_LEFT;
            if(key == GLFW_KEY_S)          cam_dir_f |= Renderer::Camera::DirectionFlag_BACKWARD;
            if(key == GLFW_KEY_D)          cam_dir_f |= Renderer::Camera::DirectionFlag_RIGHT;
            if(key == GLFW_KEY_SPACE)      cam_dir_f |= Renderer::Camera::DirectionFlag_UP;
            if(key == GLFW_KEY_LEFT_SHIFT) cam_dir_f |= Renderer::Camera::DirectionFlag_DOWN;
        }
        else if(action == GLFW_RELEASE)
        {
            if(key == GLFW_KEY_W)          cam_dir_f &= ~Renderer::Camera::DirectionFlag_FORWARD;
            if(key == GLFW_KEY_A)          cam_dir_f &= ~Renderer::Camera::DirectionFlag_LEFT;
            if(key == GLFW_KEY_S)          cam_dir_f &= ~Renderer::Camera::DirectionFlag_BACKWARD;
            if(key == GLFW_KEY_D)          cam_dir_f &= ~Renderer::Camera::DirectionFlag_RIGHT;
            if(key == GLFW_KEY_SPACE)      cam_dir_f &= ~Renderer::Camera::DirectionFlag_UP;
            if(key == GLFW_KEY_LEFT_SHIFT) cam_dir_f &= ~Renderer::Camera::DirectionFlag_DOWN;
        }
    }
}

// FIX: Check if we are in the screen region or not
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

    screen_size[0] = (float)width;
    screen_size[1] = (float)height;

    screen_halfsize[0] = width  / 2.0f;
    screen_halfsize[1] = height / 2.0f;

    viewport_changed = true;
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

static GLuint CreateFogParticleProgram()
{
    GLuint program = glCreateProgram();

    int err = LoadShaderFromFile(GL_VERTEX_SHADER, "shader/fogpart.vs", &program);
    err |= LoadShaderFromFile(GL_FRAGMENT_SHADER, "shader/fogpart.fs", &program);

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
    _motif_span = 1;
    glGenBuffers(1, &_ipb);
    glBindBuffer(GL_ARRAY_BUFFER, _ipb);
    _intancePositionMatrixPtr = nullptr;
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

    glGenBuffers(1, &_irb);
    glBindBuffer(GL_ARRAY_BUFFER, _irb);
    _intanceRotationMatrixPtr = nullptr;
    glBufferData(GL_ARRAY_BUFFER, _instanceCount * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(1 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(2 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(3 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(10);

    glVertexAttribDivisor(7, 1);
    glVertexAttribDivisor(8, 1);
    glVertexAttribDivisor(9, 1);
    glVertexAttribDivisor(10, 1);

    glGenBuffers(1, &_mpb);
    glBindBuffer(GL_ARRAY_BUFFER, _mpb);
    _motifPositionMatrixPtr = nullptr;
    glBufferData(GL_ARRAY_BUFFER, _motif_span * _instanceCount * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(11);
    glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(1 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(12);
    glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(2 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(13);
    glVertexAttribPointer(14, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(3 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(14);

    glVertexAttribDivisor(11, 1);
    glVertexAttribDivisor(12, 1);
    glVertexAttribDivisor(13, 1);
    glVertexAttribDivisor(14, 1);

    glBindVertexArray(0);
}

void Renderer::DrawInstance::updateMotifInstanceForVertexArray()
{
    glBindVertexArray(_vao);

    // position
    glVertexAttribDivisor(2, _motif_span);
    glVertexAttribDivisor(3, _motif_span);
    glVertexAttribDivisor(4, _motif_span);
    glVertexAttribDivisor(5, _motif_span);

    // color
    glVertexAttribDivisor(6, _motif_span);

    // rotation
    glVertexAttribDivisor(7, _motif_span);
    glVertexAttribDivisor(8, _motif_span);
    glVertexAttribDivisor(9, _motif_span);
    glVertexAttribDivisor(10, _motif_span);

    // motif
    glVertexAttribDivisor(11, 1);
    glVertexAttribDivisor(12, 1);
    glVertexAttribDivisor(13, 1);
    glVertexAttribDivisor(14, 1);
}

Renderer::DrawInstance::~DrawInstance()
{
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ipb);
    glDeleteBuffers(1, &_icb);
    glDeleteBuffers(1, &_irb);
    glDeleteBuffers(1, &_mpb);

    L_TRACE("~DrawInstance()");
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
    if(viewport_changed)
    {
        projectionMatrix = glm::perspective(fov, screen_size[0] / screen_size[1], 0.1f, 100.0f);
    }

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
}

static void GeneratePerlin2DTex(GLsizei w, GLsizei h, unsigned char* buffer)
{
    auto idx = [w](int x, int y) -> int { return 3 * (x + y * w); };

    if(buffer != nullptr)
    {
        std::srand(std::time(nullptr)); // Random seed
        unsigned int seed = std::rand() / ((RAND_MAX + 1u) / 2048);
        for(int x = 0; x < w; x++)
        {
            for(int y = 0; y < h; y++)
            {
                for(int i = 0; i < 3; i++)
                {
                    float noise = Utils::perlin(seed + (float)x / 16.0f, seed + (float)y / 16.0f, seed + (float)i);
                    buffer[idx(x, y) + i] = (unsigned char)((noise * 0.5f + 0.5f) * 255.0f);
                }
            }
        }
    }
    else
    {
        L_ERROR("GeneratePerlin2DTex(): buffer is nullptr.");
    }
}

Renderer::DrawList::DrawList(GLFWwindow* window, const int sw, const int sh)
{
    glfwSetKeyCallback(window, _key_callback);
    glfwSetCursorPosCallback(window, _mouse_pos_callback);
    glfwSetScrollCallback(window, _mouse_scr_callback);
    glfwSetMouseButtonCallback(window, _mouse_btn_callback);
    glfwSetFramebufferSizeCallback(window, _framebuffer_size_callback);

    screen_size[0] = sw;
    screen_size[1] = sh;

    screen_halfsize[0] = sw / 2.0f;
    screen_halfsize[1] = sh / 2.0f;

    _program_nrmpass = CreateNormalPassProgram();
    _program_sobfilter = CreateSobelFilterProgram();
    _program_fogpart = CreateFogParticleProgram();
    
    addInstance(new DrawInstance());

    camera = new Camera(45.0f);

    glUseProgram(_program_nrmpass);
    _uniforms.projectionMatrix = glGetUniformLocation(_program_nrmpass, "projectionMatrix");
    _uniforms.viewMatrix = glGetUniformLocation(_program_nrmpass, "viewMatrix");
    
    glUniformMatrix4fv(_uniforms.projectionMatrix, 1, GL_FALSE, &camera->projectionMatrix[0][0]);

    glGenFramebuffers(1, &_rendertarget.framebuffer_id);
    glBindFramebuffer(GL_FRAMEBUFFER, _rendertarget.framebuffer_id);

    glGenTextures(3, _rendertarget.texid);
    glBindTexture(GL_TEXTURE_2D, _rendertarget.texid[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sw, sh, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, _rendertarget.texid[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sw, sh, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, _rendertarget.texid[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, sw, sh, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    // NOTE: Depth attachment is automatically present
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _rendertarget.texid[0], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, _rendertarget.texid[1], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _rendertarget.texid[2], 0);
    GLenum drawBuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(3, drawBuffers);

    glUseProgram(_program_sobfilter);
    glUniform1i(glGetUniformLocation(_program_sobfilter, "screenNormalTexture"), 0);
    glUniform1i(glGetUniformLocation(_program_sobfilter, "screenDiffuseTexture"), 1);
    glUniform1i(glGetUniformLocation(_program_sobfilter, "screenDepthTexture"), 2);
    _uniforms.sobel_time = glGetUniformLocation(_program_sobfilter, "time");

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


    glGenFramebuffers(1, &_finalrender.framebuffer_id);
    glBindFramebuffer(GL_FRAMEBUFFER, _finalrender.framebuffer_id);
    glGenTextures(1, &_finalrender.texid);
    glBindTexture(GL_TEXTURE_2D, _finalrender.texid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sw, sh, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    unsigned char* buffer = new unsigned char[3 * 1024 * 1024];

    GeneratePerlin2DTex(1024, 1024, buffer);
    glGenTextures(1, &_fog_perlin_texid);
    glBindTexture(GL_TEXTURE_2D, _fog_perlin_texid);
    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // For 1d texture only (GL_RED)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    delete[] buffer;

    glGenVertexArrays(1, &_vao_fog);
    glBindVertexArray(_vao_fog);
    glGenBuffers(1, &_vbo_fog);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo_fog);

    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    _fog_density = 1000;
    std::vector<float> fog_points;
    fog_points.reserve(3 * _fog_density);

    std::srand(std::time(nullptr));
    // Random AABB around view fustrum
    for(int i = 0; i < 3 * _fog_density; i++)
    {
        float rx = ((float)std::rand() / RAND_MAX) * 2.0f - 1.0f;
        float ry = ((float)std::rand() / RAND_MAX) * 2.0f - 1.0f;
        float rz = ((float)std::rand() / RAND_MAX) * 2.0f - 1.0f;
        fog_points.push_back(rx);
        fog_points.push_back(ry);
        fog_points.push_back(rz);
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * _fog_density, fog_points.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(_program_fogpart);
    _uniforms.fog_projectionMatrix = glGetUniformLocation(_program_fogpart, "projectionMatrix");
    _uniforms.fog_viewMatrix = glGetUniformLocation(_program_fogpart, "viewMatrix");
    _uniforms.fog_time = glGetUniformLocation(_program_fogpart, "time");
    _uniforms.fog_motifSize = glGetUniformLocation(_program_fogpart, "ymotifsize");

    glUniformMatrix4fv(_uniforms.fog_projectionMatrix, 1, GL_FALSE, &camera->projectionMatrix[0][0]);
    glUniform1f(_uniforms.fog_motifSize, 0.0f);
}

void Renderer::DrawList::updateFogParticlesMotifSize()
{
    glBindBuffer(GL_ARRAY_BUFFER, _vbo_fog);
    

    float fog_range_x = mcm_motif_size.x * 10.0f;
    float fog_range_y = mcm_motif_size.y * 10.0f;
    float fog_range_z = mcm_motif_size.z * 10.0f;

    _fog_density = 10 * fog_range_x * fog_range_y * fog_range_z;
    std::vector<float> fog_points;
    fog_points.reserve(3 * _fog_density);

    // Random AABB around view fustrum
    for(int i = 0; i < 3 * _fog_density; i++)
    {
        float rx = (((float)std::rand() / RAND_MAX) * 2.0f - 1.0f) * fog_range_x;
        float ry = (((float)std::rand() / RAND_MAX) * 2.0f - 1.0f) * fog_range_y - fog_range_y / 2.0f;
        float rz = (((float)std::rand() / RAND_MAX) * 2.0f - 1.0f) * fog_range_z;
        fog_points.push_back(rx);
        fog_points.push_back(ry);
        fog_points.push_back(rz);
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * _fog_density, fog_points.data(), GL_STATIC_DRAW);

    glUseProgram(_program_fogpart);
    glUniform1f(_uniforms.fog_motifSize, fog_range_y);
}

Renderer::DrawList::~DrawList()
{
    for(auto i : instances)
    {
        delete i;
    }

    delete camera;

    instances.clear();
    glDeleteProgram(_program_nrmpass);
    glDeleteProgram(_program_sobfilter);
    glDeleteProgram(_program_fogpart);

    glDeleteVertexArrays(1, &_vao_screen);
    glDeleteBuffers(1, &_vbo_screen);

    glDeleteVertexArrays(1, &_vao_fog);
    glDeleteBuffers(1, &_vbo_fog);

    glDeleteTextures(3, _rendertarget.texid);
    glDeleteFramebuffers(1, &_rendertarget.framebuffer_id);

    glDeleteTextures(1, &_finalrender.texid);
    glDeleteFramebuffers(1, &_finalrender.framebuffer_id);

    glDeleteTextures(1, &_fog_perlin_texid);

    L_TRACE("~DrawList()");
}

void Renderer::DrawList::updateFramebufferTextures()
{
    glBindTexture(GL_TEXTURE_2D, _rendertarget.texid[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)screen_size[0], (GLsizei)screen_size[1], 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glBindTexture(GL_TEXTURE_2D, _rendertarget.texid[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)screen_size[0], (GLsizei)screen_size[1], 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glBindTexture(GL_TEXTURE_2D, _rendertarget.texid[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, (GLsizei)screen_size[0], (GLsizei)screen_size[1], 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);

    glBindTexture(GL_TEXTURE_2D, _finalrender.texid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)screen_size[0], (GLsizei)screen_size[1], 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
}

void Renderer::DrawList::render(GLFWwindow* window, NodeWindow* nodeWindow)
{
    if(viewport_changed)
    {
        updateFramebufferTextures();

        glUseProgram(_program_nrmpass);
        glUniformMatrix4fv(_uniforms.projectionMatrix, 1, GL_FALSE, &camera->projectionMatrix[0][0]);

        glUseProgram(_program_fogpart);
        glUniformMatrix4fv(_uniforms.fog_projectionMatrix, 1, GL_FALSE, &camera->projectionMatrix[0][0]);

        // Resize the node window
        nodeWindow->setWindowSize(ImVec2(screen_size[0], screen_size[1] / 3));
        nodeWindow->setWindowPos(ImVec2(0, 0));
    }

    static float last_time = (float)glfwGetTime();
    RenderNode* outNode = dynamic_cast<RenderNode*>(nodeWindow->getRenderOutputNode());

    if(outNode != nullptr)
    {
        RenderNodeData nodeData = outNode->data.getValue<RenderNodeData>();

        if(nodeWindow->isRenderOutputNodeChanged())
        {
            // RenderNodeData nodeData = outNode->data.getValue<RenderNodeData>();
            instances[0]->_instanceCount = nodeData._instanceCount;
            instances[0]->_intancePositionMatrixPtr = nodeData._worldPositionPtr;
            instances[0]->_intanceRotationMatrixPtr = nodeData._worldRotationPtr;
            instances[0]->_instanceColorsPtr = nodeData._instanceColorsPtr;
        }

        if(outNode->renderDataChanged())
        {
            instances[0]->_instanceCount = nodeData._instanceCount;


            MeshNodeData* mesh = *(nodeData._meshPtr);
            if(mesh != nullptr)
            {
                glBindBuffer(GL_ARRAY_BUFFER, instances[0]->_vbo);
                glBufferData(GL_ARRAY_BUFFER, mesh->data_size * sizeof(float), mesh->vertex_data, GL_STATIC_DRAW);
                instances[0]->_idxcount = (GLsizei)mesh->data_size / 6;
            }
        }

        if(outNode->data.dataChanged())
        {
            if(nodeData._fogChanged)
            {
                // NOTE: Using glGetUniformLocation should be ok: this won't change that often and I'm lazy
                glUseProgram(_program_sobfilter);
                glUniform1f(glGetUniformLocation(_program_sobfilter, "fogMax"), nodeData._fogMax);
                glUniform1f(glGetUniformLocation(_program_sobfilter, "fogMin"), nodeData._fogMin);
                // fogcolor is background basically (no volumetric fog)
                glClearColor(nodeData._fogColor.x, nodeData._fogColor.y, nodeData._fogColor.z, 1.0f);
            }

            glUseProgram(_program_sobfilter);
            glUniform1f(glGetUniformLocation(_program_sobfilter, "fogMax"), nodeData._fogMax);

            if(nodeData._repeatBlocks && nodeData._motifChanged)
            {
                mcm_motif_size = nodeData._motifSize; // NOTE : This only works for a single active render node like this
                instances[0]->_motif_span = nodeData._motif_span;

                glm::mat4* mpos = *(nodeData._motifPositionPtr);
                if(mpos != nullptr)
                {
                    glBindBuffer(GL_ARRAY_BUFFER, instances[0]->_mpb);
                    glBufferData(GL_ARRAY_BUFFER, instances[0]->_motif_span * instances[0]->_instanceCount * sizeof(glm::mat4), mpos, GL_DYNAMIC_DRAW);
                    instances[0]->updateMotifInstanceForVertexArray();
                }
                // FIXME : Fog Updater SLOOOOW
                // updateFogParticlesMotifSize();
            }
            else if(!nodeData._repeatBlocks)
            {
                instances[0]->_motif_span = 1;
                glBindBuffer(GL_ARRAY_BUFFER, instances[0]->_mpb);
                glm::mat4 identity = glm::mat4(1.0f);
                glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4), &identity, GL_DYNAMIC_DRAW);
                instances[0]->updateMotifInstanceForVertexArray();
                glBindVertexArray(instances[0]->_vao);
                glVertexAttribDivisor(11, instances[0]->_instanceCount);
                glVertexAttribDivisor(12, instances[0]->_instanceCount);
                glVertexAttribDivisor(13, instances[0]->_instanceCount);
                glVertexAttribDivisor(14, instances[0]->_instanceCount);
                mcm_motif_size = infinityVec3;

                // FIXME : Take care of this please
                // updateFogParticlesMotifSize();
            }
        }

        glm::mat4* pos = *(nodeData._worldPositionPtr);
        if(pos != nullptr)
        {
            glBindBuffer(GL_ARRAY_BUFFER, instances[0]->_ipb);
            glBufferData(GL_ARRAY_BUFFER, instances[0]->_instanceCount * sizeof(glm::mat4), pos, GL_DYNAMIC_DRAW);
        }

        glm::mat4* rot = *(nodeData._worldRotationPtr);
        if(rot != nullptr)
        {
            glBindBuffer(GL_ARRAY_BUFFER, instances[0]->_irb);
            glBufferData(GL_ARRAY_BUFFER, instances[0]->_instanceCount * sizeof(glm::mat4), rot, GL_DYNAMIC_DRAW);
        }

        Vector4* col = *(nodeData._instanceColorsPtr);
        if(col != nullptr)
        {
            glBindBuffer(GL_ARRAY_BUFFER, instances[0]->_icb);
            glBufferData(GL_ARRAY_BUFFER, instances[0]->_instanceCount * sizeof(Vector4), col, GL_DYNAMIC_DRAW);
        }
    }

    // Update camera
    float now = (float)glfwGetTime();
    camera->update(mouse_scroll, mouse_delta[0], mouse_delta[1], cam_dir_f, now - last_time);
    last_time = now;

    // Render using normals to create an image to the sobel filter for edge detection
    glUseProgram(_program_nrmpass);
    glUniformMatrix4fv(_uniforms.viewMatrix, 1, GL_FALSE, &camera->viewMatrix[0][0]);
    glBindFramebuffer(GL_FRAMEBUFFER, _rendertarget.framebuffer_id);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(instances[0]->_vao);

    glDrawArraysInstanced(GL_TRIANGLES, 0, instances[0]->_idxcount, instances[0]->_instanceCount * instances[0]->_motif_span);

    // FIXME : Fog/dust particles rendering by putting them at the origin with camera scroll
    // Render fog and particles to the final texture for displaying
    float time = (float)NodeWindow::GetApptimeMs() / 1000.0f;
    // glUseProgram(_program_fogpart);
    // glUniform1f(_uniforms.fog_time, time);
    // glUniformMatrix4fv(_uniforms.fog_viewMatrix, 1, GL_FALSE, &camera->viewMatrix[0][0]);
    // glBindVertexArray(_vao_fog);
    // // Don't write to the normal color buffer
    // // glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    // // Don't write to the depth buffer
    // // glDepthMask(GL_FALSE);
    // glDrawArrays(GL_POINTS, 0, _fog_density);
    // // glDepthMask(GL_TRUE);
    // // glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // Render the filter calculated outlines into the screen alongside the diffuse data
    glUseProgram(_program_sobfilter);
    glUniform1f(_uniforms.sobel_time, time);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // TODO : Render to texture
    // glBindFramebuffer(GL_FRAMEBUFFER, _finalrender.framebuffer_id);
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

    viewport_changed = false;
}
