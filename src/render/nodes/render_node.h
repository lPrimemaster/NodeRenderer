#pragma once
#include "node.h"
#include "mesh_node.h"
#include "../../math/vector.h"
#include "../../../glm/glm/glm.hpp"
#include "../../../glm/glm/gtx/transform.hpp"

struct RenderNodeData
{
    // Instance and mesh rendering data
    unsigned int   _instanceCount = 0;
    MeshNodeData** _meshPtr = nullptr;
    glm::mat4**    _worldPositionPtr = nullptr;
    Vector4**      _instanceColorsPtr = nullptr;

    // Fog rendering data
    float   _fogMax = 50.0;
    float   _fogMin = 10.0;
    Vector3 _fogColor = { 0.7f, 0.7f, 0.7f };
    bool    _fogChanged = false;
};

struct RenderNode final : public PropertyNode
{
    inline RenderNode() : PropertyNode(RenderNodeData())
    {
        static int inc = 0;
        setInputsOrdered(
            {
                "instanceCount",
                "worldPosition",
                "mesh",
                "colors"
            }
        );
        _output_count = 0;

        _renderData._worldPositionPtr = (glm::mat4**)malloc(sizeof(glm::mat4*));
        *(_renderData._worldPositionPtr) = nullptr;
        L_TRACE("_worldPositionPtr : 0x%X", _renderData._worldPositionPtr);

        _renderData._instanceColorsPtr = (Vector4**)malloc(sizeof(Vector4*));
        *(_renderData._instanceColorsPtr) = nullptr;
        L_TRACE("_instanceColorsPtr : 0x%X", _renderData._instanceColorsPtr);

        _renderData._meshPtr = (MeshNodeData**)malloc(sizeof(MeshNodeData*));
        *(_renderData._meshPtr) = nullptr;
        L_TRACE("_meshPtr : 0x%X", _renderData._meshPtr);

        name = "Render Node #" + std::to_string(inc++);

        data.setValue(_renderData);
    }
    
    ~RenderNode()
    {
        L_TRACE("~RenderNode()");
        if(_renderData._worldPositionPtr != nullptr)
        {
            free(_renderData._worldPositionPtr);
        }

        if(_renderData._instanceColorsPtr != nullptr)
        {
            free(_renderData._instanceColorsPtr);
        }

        if(_renderData._meshPtr != nullptr)
        {
            free(_renderData._meshPtr);
        }
    }

    inline bool renderDataChanged() const
    {
        return _render_data_changed;
    }

    inline virtual void render() override
    {
        data.resetDataUpdate();
        _render_data_changed = false;

        disconnectInputIfNotOfType<unsigned int>("instanceCount");
        disconnectInputIfNotOfType<std::vector<Vector3>>("worldPosition");
        disconnectInputIfNotOfType<MeshNodeData>("mesh");
        disconnectInputIfNotOfType<std::vector<Vector4>>("colors");

        // Instance Count Handling
        auto instanceCountLocal = inputs_named.find("instanceCount");
        if(instanceCountLocal == inputs_named.end())
        {
            _instanceCountLast = 1;
            if(_instanceCountLast != _renderData._instanceCount)
            {
                _renderData._instanceCount = _instanceCountLast;

                glm::mat4* worldPosLocal = *(_renderData._worldPositionPtr);
                if(worldPosLocal != nullptr)
                {
                    delete[] worldPosLocal;
                }
                worldPosLocal = new glm::mat4[_instanceCountLast];

                auto worldPositionLocal = inputs_named.find("worldPosition");
                if(worldPositionLocal != inputs_named.end())
                {
                    auto worldPositions = worldPositionLocal->second->data.getValue<std::vector<Vector3>>();
                    worldPosLocal[0] = glm::translate(
                                                glm::vec3(
                                                    worldPositions[0].x, 
                                                    worldPositions[0].y,
                                                    worldPositions[0].z
                                                )
                                            );
                }
                else
                {
                    worldPosLocal[0] = glm::mat4(1.0f);
                }

                Vector4* instanceColorLocal = *(_renderData._instanceColorsPtr);
                if(instanceColorLocal != nullptr)
                {
                    delete[] instanceColorLocal;
                }
                instanceColorLocal = new Vector4[_instanceCountLast];

                auto colorLocal = inputs_named.find("colors");
                if(colorLocal != inputs_named.end())
                {
                    auto colors = colorLocal->second->data.getValue<std::vector<Vector4>>();
                    instanceColorLocal[0] = colors[0];
                }
                else
                {
                    instanceColorLocal[0] = Vector4(1, 1, 1, 1);
                }

                _render_data_changed = true;
                *(_renderData._worldPositionPtr) = worldPosLocal;
                *(_renderData._instanceColorsPtr) = instanceColorLocal;
                data.setValue(_renderData);
            }
        }
        else
        {
            unsigned int instanceCount = instanceCountLocal->second->data.getValue<unsigned int>();
            auto worldPositionLocal = inputs_named.find("worldPosition");
            auto colorLocal = inputs_named.find("colors");
            bool worldPositionOkay = true;
            bool colorOkay = true;

            if(colorLocal == inputs_named.end())
            {
                colorOkay = false;
            }
            else
            {
                if(colorLocal->second->data.dataChanged())
                {
                    auto colors = colorLocal->second->data.getValue<std::vector<Vector4>>();
                    Vector4* color = *(_renderData._instanceColorsPtr);

                    memcpy(color, colors.data(), colors.size() * sizeof(Vector4));

                    *(_renderData._instanceColorsPtr) = color;
                }
            }

            if(worldPositionLocal == inputs_named.end())
            {
                worldPositionOkay = false;
            }
            else
            {
                if(worldPositionLocal->second->data.dataChanged())
                {
                    auto worldPositions = worldPositionLocal->second->data.getValue<std::vector<Vector3>>();
                    glm::mat4* worldPosLocal = *(_renderData._worldPositionPtr);

                    for(unsigned int i = 0; i < _renderData._instanceCount; i++)
                    {
                        worldPosLocal[i] = glm::translate(
                                                            glm::vec3(
                                                                worldPositions[i].x, 
                                                                worldPositions[i].y,
                                                                worldPositions[i].z
                                                            )
                                                        );
                    }

                    *(_renderData._worldPositionPtr) = worldPosLocal;
                }
            }

            if(_instanceCountLast != instanceCount)
            {
                _renderData._instanceCount = instanceCount;
                
                glm::mat4* worldPosLocal = *(_renderData._worldPositionPtr);
                if(worldPosLocal != nullptr)
                {
                    delete[] worldPosLocal;
                }
                worldPosLocal = new glm::mat4[instanceCount];

                if(worldPositionOkay)
                {
                    auto worldPositions = worldPositionLocal->second->data.getValue<std::vector<Vector3>>();

                    for(unsigned int i = 0; i < _renderData._instanceCount; i++)
                    {
                        worldPosLocal[i] = glm::translate(
                                                            glm::vec3(
                                                                worldPositions[i].x, 
                                                                worldPositions[i].y,
                                                                worldPositions[i].z
                                                            )
                                                        );
                    }
                }
                else
                {
                    for(unsigned int i = 0; i < _renderData._instanceCount; i++)
                    {
                        worldPosLocal[i] = glm::mat4(1.0f);
                    }
                }

                Vector4* colors = *(_renderData._instanceColorsPtr);
                if(colors != nullptr)
                {
                    delete[] colors;
                }
                colors = new Vector4[instanceCount];

                if(colorOkay)
                {
                    auto color = colorLocal->second->data.getValue<std::vector<Vector4>>();
                    memcpy(colors, color.data(), instanceCount * sizeof(Vector4));
                }
                else
                {
                    for(unsigned int i = 0; i < _renderData._instanceCount; i++)
                    {
                        colors[i] = Vector4(1, 1, 1, 1);
                    }
                }

                _instanceCountLast = instanceCount;
                _render_data_changed = true;
                *(_renderData._worldPositionPtr) = worldPosLocal;
                *(_renderData._instanceColorsPtr) = colors;
                data.setValue(_renderData);
            }
        }

        // if mesh data changed
        // *(_renderData._meshPtr)
        auto meshLocal = inputs_named.find("mesh");
        if(meshLocal != inputs_named.end())
        {
            if(meshLocal->second->data.dataChanged() || *(_renderData._meshPtr) == nullptr)
            {
                // We have a new mesh for displaying
                // Handle it
                auto newMeshPtr = meshLocal->second->data.getValuePtr<MeshNodeData>();
                if(newMeshPtr->data_size > 0)
                {
                    _render_data_changed = true;
                    *(_renderData._meshPtr) = newMeshPtr;
                    data.setValue(_renderData);
                }
            }
        }

        ImGui::Text("This node converts data to be rendered.");
        ImGui::TextColored(ImVec4(0.8f, 0.7f, 0.0f, 1.0f), "Warn: Data types are not checked!");

        ImGui::BeginChild("#details_window", ImVec2(400.0f, 300.0f), true);

        // Fog settings header
        ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(179, 102, 0, 255));
        bool fog_header = ImGui::CollapsingHeader("Fog");
        ImGui::PopStyleColor();
        if(fog_header)
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 border_start = ImGui::GetItemRectMin();
            ImGui::BeginChild("#fog_details", ImVec2(0, 0), false);

            _renderData._fogChanged  = ImGui::InputFloat("Fog Maximum", &_renderData._fogMax, 1.0f, 0.0f, "%.1f");
            _renderData._fogChanged |= ImGui::InputFloat("Fog Minimum", &_renderData._fogMin, 1.0f, 0.0f, "%.1f");
            _renderData._fogChanged |= ImGui::ColorPicker3("Fog Color", _renderData._fogColor.data);

            if(_renderData._fogChanged)
            {
                _fog_changed_last_frame = true;
                data.setValue(_renderData);
            }
            else if(_fog_changed_last_frame)
            {
                _fog_changed_last_frame = false;
                data.setValue(_renderData);
            }

            ImGui::EndChild();

            ImVec2 border_end = ImGui::GetItemRectMax();
            border_end.x += ImGui::GetStyle().WindowPadding.x / 2;
            draw_list->AddRect(border_start, border_end, IM_COL32(179, 102, 0, 255));
        }

        ImGui::EndChild();
    }
private:
    unsigned int _instanceCountLast = 0;
    RenderNodeData _renderData;
    bool _render_data_changed = false;
    bool _fog_changed_last_frame = false;
};
