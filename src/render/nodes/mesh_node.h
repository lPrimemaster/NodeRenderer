#pragma once
#include "node.h"
#include "../../util/imgui_ext.inl"
#include "../../util/objloader.h"
#include <future>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct MeshNodeData
{
    float* vertex_data = nullptr;
    size_t data_size = 0;
};

struct MeshNode final : public PropertyNode
{
    inline MeshNode() : PropertyNode(MeshNodeData())
    {
        static int inc = 0;
        _input_count = 0;
        _output_count = 1;
        name = "Mesh Node #" + std::to_string(inc++);

        // glGenVertexArrays(1, &_preview_vao);
        // glBindVertexArray(_preview_vao);

        // glGenBuffers(1, &_preview_vbo);
        // glBindBuffer(GL_ARRAY_BUFFER, _preview_vbo);
        // glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

        // GLuint _preview_vao;
        // GLuint _preview_vbo;
        // GLuint _preview_prog;
        // GLuint _preview_fbo;
        // GLuint _preview_tex;
    }
    
    ~MeshNode()
    {
        if(vertices_data.vertex_data != nullptr)
        {
            delete[] vertices_data.vertex_data;
        }
    }

    inline virtual void render() override
    {
        data.resetDataUpdate();
        // TODO: Make this node a file drag and drop from windows as well

        if(valid_model)
        {
            //TODO: Display a model preview on the node
            ImGui::Text("Currently loaded: %s", std::filesystem::path(to_load).filename().string().c_str());


        }
        
        bool closepopup = false;
        static const std::vector<std::string> ext = { ".obj" };
        if(ImGuiExt::FileBrowser(&to_load, ext))
        {
            // Thread the file loading
            if(!loading)
            {
                f_fdata = std::async(Utils::LoadFloatVertexDataFromFile, to_load);
                loading = true;
            }
        }
        
        if(f_fdata.valid())
        {
            if(f_fdata.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                closepopup = true;
                fdata = f_fdata.get();
                loading = false;
            }
            else if(loading && !popupOpened)
            {
                popupOpened = true;
                L_TRACE("Loading popup open.");
                ImGui::OpenPopup("Please Wait");
            }

            if(!fdata.empty())
            {
                L_DEBUG("mesh_node: loaded obj file.");
                valid_model = true;

                vertices_data.data_size = fdata.size();
                if(vertices_data.vertex_data != nullptr)
                {
                    delete[] vertices_data.vertex_data;
                }
                vertices_data.vertex_data = new float[vertices_data.data_size];

                memcpy(vertices_data.vertex_data, fdata.data(), fdata.size() * sizeof(float));

                data.setValue(vertices_data);
                fdata.clear();
            }
        }

        if(ImGui::BeginPopupModal("Please Wait", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGuiExt::SpinnerText();
            ImGui::SameLine();
            ImGui::Text("Loading File...");
            if(closepopup)
            {
                popupOpened = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

private:
    std::string to_load;
    MeshNodeData vertices_data;
    std::future<std::vector<float>> f_fdata;
    std::vector<float> fdata;
    bool valid_model = false;
    bool loading = false;
    bool popupOpened = false;

    // NOTE: We are inside a gl context, so this should be fine
    GLuint _preview_vao;
    GLuint _preview_vbo;
    GLuint _preview_prog;
    GLuint _preview_fbo;
    GLuint _preview_tex;
};
