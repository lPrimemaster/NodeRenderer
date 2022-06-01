#pragma once
#include "node.h"
#include "mesh_node.h"
#include "../../math/vector.h"
#include "../../../glm/glm/glm.hpp"
#include "../../../glm/glm/gtx/transform.hpp"
#include "../../../glm/glm/gtx/euler_angles.hpp"

struct RenderNodeData
{
    // Instance and mesh rendering data
    unsigned int   _instanceCount = 0;
    MeshNodeData** _meshPtr = nullptr;
    glm::mat4**    _worldPositionPtr = nullptr;
    glm::mat4**    _worldRotationPtr = nullptr;
    Vector4**      _instanceColorsPtr = nullptr;
    glm::mat4**    _motifPositionPtr = nullptr;

    // Fog rendering data
    float   _fogMax = 50.0f;
    float   _fogMin = 10.0f;
    Vector3 _fogColor = { 0.7f, 0.7f, 0.7f };
    bool    _fogChanged = false;

    // Position related options
    bool         _repeatBlocks = false;
    Vector3      _motifSize = Vector3(2.0f, 2.0f, 2.0f);
    unsigned int _motif_span = 1;
    unsigned int _motifInstances[3] = {1, 1, 1};
    bool         _motifChanged = false;
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
                "worldRotation",
                "mesh",
                "colors"
            }
        );
        _output_count = 0;

        _renderData._worldPositionPtr = (glm::mat4**)malloc(sizeof(glm::mat4*));
        *(_renderData._worldPositionPtr) = nullptr;
        L_TRACE("_worldPositionPtr : 0x%X", _renderData._worldPositionPtr);

        _renderData._worldRotationPtr = (glm::mat4**)malloc(sizeof(glm::mat4*));
        *(_renderData._worldRotationPtr) = nullptr;
        L_TRACE("_worldRotationPtr : 0x%X", _renderData._worldRotationPtr);

        _renderData._instanceColorsPtr = (Vector4**)malloc(sizeof(Vector4*));
        *(_renderData._instanceColorsPtr) = nullptr;
        L_TRACE("_instanceColorsPtr : 0x%X", _renderData._instanceColorsPtr);

        _renderData._meshPtr = (MeshNodeData**)malloc(sizeof(MeshNodeData*));
        *(_renderData._meshPtr) = nullptr;
        L_TRACE("_meshPtr : 0x%X", _renderData._meshPtr);

        _renderData._motifPositionPtr = (glm::mat4**)malloc(sizeof(glm::mat4*));
        *(_renderData._motifPositionPtr) = nullptr;
        L_TRACE("_meshPtr : 0x%X", _renderData._motifPositionPtr);

        name = "Render Node #" + std::to_string(inc++);

        data.setValue(_renderData);
    }
    
    ~RenderNode()
    {
        if(_renderData._worldPositionPtr != nullptr)
        {
            free(_renderData._worldPositionPtr);
        }

        if(_renderData._worldRotationPtr != nullptr)
        {
            free(_renderData._worldRotationPtr);
        }

        if(_renderData._instanceColorsPtr != nullptr)
        {
            free(_renderData._instanceColorsPtr);
        }

        if(_renderData._meshPtr != nullptr)
        {
            free(_renderData._meshPtr);
        }

        if(_renderData._motifPositionPtr != nullptr)
        {
            free(_renderData._motifPositionPtr);
        }
        L_TRACE("~RenderNode()");
    }

    inline bool renderDataChanged() const
    {
        return _render_data_changed;
    }

    // TODO: This node needs a refactor
    inline virtual void render() override
    {
        ImGui::Text("This node converts data to be rendered.");
        ImGui::TextColored(ImVec4(0.8f, 0.7f, 0.0f, 1.0f), "Warn: Data types are not checked!");

        #define AND_BHEADER(i) allCollapsed &= !bheaders.all[i]
        #define HEADER_SZ 2
        struct
        {
            union
            {
                struct
                {
                    bool fog;
                    bool pos;
                };
                bool all[HEADER_SZ];
            };
        } static bheaders;
        static_assert(HEADER_SZ == sizeof(bheaders) / sizeof(bool));

        bool allCollapsed = true;
        for(int i = 0; i < HEADER_SZ; i++) AND_BHEADER(i);

        ImVec2 childSize = ImVec2(400.0f, (ImGui::GetTextLineHeightWithSpacing() + 15.0f) * HEADER_SZ);

        #undef AND_BHEADER
        #undef HEADER_SZ

        if(!allCollapsed) childSize = ImVec2(400.0f, 300.0f);

        ImGui::BeginChild("#details_window", childSize, true);

        // Fog settings header
        ImU32 frame_color = IM_COL32(179, 102, 0, 255);
        ImGui::PushStyleColor(ImGuiCol_Header, frame_color);
        bheaders.fog = ImGui::CollapsingHeader("Fog");
        ImGui::PopStyleColor();
        if(bheaders.fog)
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 border_start = ImGui::GetItemRectMin();
            ImGui::BeginChild("#fog_details");

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
            draw_list->AddRect(border_start, border_end, frame_color);
        }

        // Position settings header
        frame_color = IM_COL32(50, 205, 50, 255);
        ImGui::PushStyleColor(ImGuiCol_Header, frame_color);
        bheaders.pos = ImGui::CollapsingHeader("Position");
        ImGui::PopStyleColor();
        _motif_changed_internal = false;
        if(bheaders.pos)
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 border_start = ImGui::GetItemRectMin();
            ImGui::BeginChild("#pos_details");

            _motif_changed_internal = ImGui::Checkbox("Repeat Motif", &_renderData._repeatBlocks);

            if(_motif_changed_internal)
            {
                auto worldPositionLocal = inputs_named.find("worldPosition");
                if(worldPositionLocal != inputs_named.end())
                {
                    auto data = worldPositionLocal->second->data.getValue<std::vector<Vector3>>().data();
                    float maxx = positionMaxFromArray(data, 0);
                    float maxy = positionMaxFromArray(data, 1);
                    float maxz = positionMaxFromArray(data, 2);

                    float minx = positionMinFromArray(data, 0);
                    float miny = positionMinFromArray(data, 1);
                    float minz = positionMinFromArray(data, 2);

                    float spanx = std::abs(maxx - minx) * 2.0f;
                    float spany = std::abs(maxy - miny) * 2.0f;
                    float spanz = std::abs(maxz - minz) * 2.0f;

                    L_DEBUG("Data span (x, y, z): (%.1f, %.1f, %.1f)", spanx, spany, spanz);

                    if(spanx > _renderData._motifSize.x) _renderData._motifSize.x = spanx;
                    if(spany > _renderData._motifSize.y) _renderData._motifSize.y = spany;
                    if(spanz > _renderData._motifSize.z) _renderData._motifSize.z = spanz;
                }
            }

            if(_renderData._repeatBlocks)
            {
                if(ImGui::InputFloat3("Motif Size", _renderData._motifSize.data, "%.1f") || _motif_changed_internal || _renderData._fogChanged)
                {
                    _motif_changed_internal = true;
                    _renderData._motifInstances[0] = (unsigned int)std::ceil(_renderData._fogMax / _renderData._motifSize.x);
                    _renderData._motifInstances[1] = (unsigned int)std::ceil(_renderData._fogMax / _renderData._motifSize.y);
                    _renderData._motifInstances[2] = (unsigned int)std::ceil(_renderData._fogMax / _renderData._motifSize.z);

                    _renderData._motif_span = (_renderData._motifInstances[0] + 1) *
                                              (_renderData._motifInstances[1] + 1) *
                                              (_renderData._motifInstances[2] + 1) * 8;

                    glm::mat4* motifPosLocal = *(_renderData._motifPositionPtr);
                    if(motifPosLocal != nullptr)
                    {
                        delete[] motifPosLocal;
                    }
                    motifPosLocal = new glm::mat4[_renderData._motif_span * _renderData._instanceCount];

                    int span_x = (int)_renderData._motifInstances[0];
                    int span_y = (int)_renderData._motifInstances[1];
                    int span_z = (int)_renderData._motifInstances[2];


                    // TODO: This can be async if it starts to seem slow
                    for(int i = 0; i < _renderData._instanceCount; i++)
                    {
                        int o = i * _renderData._motif_span;
                        for(int z = -span_z; z < span_z + 1; z++)
                        {
                            for(int y = -span_y; y < span_y + 1; y++)
                            {
                                for(int x = -span_x; x < span_x + 1; x++)
                                {
                                    int idx = (x + span_x) + (y + span_y) * 2 * span_x + (z + span_z) * 4 * span_x * span_y;
                                    motifPosLocal[o + idx] = glm::translate(
                                        glm::vec3(
                                            _renderData._motifSize.x * x,
                                            _renderData._motifSize.y * y,
                                            _renderData._motifSize.z * z
                                        )
                                    );
                                }
                            }
                        }
                    }

                    *(_renderData._motifPositionPtr) = motifPosLocal;
                    L_DEBUG("New Motif Instances: (%u, %u, %u)", _renderData._motifInstances[0], _renderData._motifInstances[1], _renderData._motifInstances[2]);
                    L_DEBUG("Motif Instance Attr size: %d kb", _renderData._motif_span * _renderData._instanceCount * sizeof(glm::mat4) / 1024);
                    L_DEBUG("New Total Instance Count: %u", _renderData._motif_span * _renderData._instanceCount);
                    _renderData._motifChanged = true;
                }
                else
                {
                    _motif_changed_internal = false;
                    _renderData._motifChanged = false;
                }

                data.setValue(_renderData);
            }

            ImGui::EndChild();

            ImVec2 border_end = ImGui::GetItemRectMax();
            border_end.x += ImGui::GetStyle().WindowPadding.x / 2;
            draw_list->AddRect(border_start, border_end, frame_color);
        }

        // Recalculate instances
        if(_motif_changed_internal || _renderData._fogChanged)
        {
            _renderData._motifInstances[0] = (unsigned int)std::ceil(_renderData._fogMax / _renderData._motifSize.x);
            _renderData._motifInstances[1] = (unsigned int)std::ceil(_renderData._fogMax / _renderData._motifSize.y);
            _renderData._motifInstances[2] = (unsigned int)std::ceil(_renderData._fogMax / _renderData._motifSize.z);
            
            _renderData._motif_span = (_renderData._motifInstances[0] + 1) *
                                      (_renderData._motifInstances[1] + 1) *
                                      (_renderData._motifInstances[2] + 1) * 8;

            glm::mat4* motifPosLocal = *(_renderData._motifPositionPtr);
            if(motifPosLocal != nullptr)
            {
                delete[] motifPosLocal;
            }
            motifPosLocal = new glm::mat4[_renderData._motif_span * _renderData._instanceCount];

            int span_x = (int)_renderData._motifInstances[0];
            int span_y = (int)_renderData._motifInstances[1];
            int span_z = (int)_renderData._motifInstances[2];

            for(int i = 0; i < (int)_renderData._instanceCount; i++)
            {
                int o = i * _renderData._motif_span;
                for(int z = -span_z; z < span_z + 1; z++)
                {
                    for(int y = -span_y; y < span_y + 1; y++)
                    {
                        for(int x = -span_x; x < span_x + 1; x++)
                        {
                            int idx = (x + span_x) + (y + span_y) * 2 * span_x + (z + span_z) * 4 * span_x * span_y;
                            motifPosLocal[o + idx] = glm::translate(
                                glm::vec3(
                                    _renderData._motifSize.x * x,
                                    _renderData._motifSize.y * y,
                                    _renderData._motifSize.z * z
                                )
                            );
                        }
                    }
                }
            }

            *(_renderData._motifPositionPtr) = motifPosLocal;

            L_DEBUG("New Motif Instances: (%u, %u, %u)", _renderData._motifInstances[0], _renderData._motifInstances[1], _renderData._motifInstances[2]);
            L_DEBUG("Motif Instance Attr size: %d kb", _renderData._motif_span * _renderData._instanceCount * sizeof(glm::mat4) / 1024);
            L_DEBUG("New Total Instance Count: %u", _renderData._motif_span * _renderData._instanceCount);
            _renderData._motifChanged = true;
            data.setValue(_renderData);
        }
        else if(_renderData._motifChanged)
        {
            _renderData._motifChanged = false;
            data.setValue(_renderData);
        }

        ImGui::EndChild();
    }

    inline virtual void update() override
    {
        data.resetDataUpdate();
        _render_data_changed = false;

        disconnectInputIfNotOfType<unsigned int>("instanceCount");
        disconnectInputIfNotOfType<std::vector<Vector3>>("worldPosition");
        disconnectInputIfNotOfType<std::vector<Vector3>>("worldRotation");
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

                glm::mat4* worldRotationLocal = *(_renderData._worldRotationPtr);
                if(worldRotationLocal != nullptr)
                {
                    delete[] worldRotationLocal;
                }
                worldRotationLocal = new glm::mat4[_instanceCountLast];

                auto rotationLocal = inputs_named.find("worldRotation");
                if(rotationLocal != inputs_named.end())
                {
                    auto rots = rotationLocal->second->data.getValue<std::vector<Vector4>>();
                    worldRotationLocal[0] = glm::eulerAngleYXZ(rots[0].y, rots[0].x, rots[0].z);
                }
                else
                {
                    worldRotationLocal[0] = glm::mat4(1.0f);
                }

                _render_data_changed = true;
                *(_renderData._worldPositionPtr) = worldPosLocal;
                *(_renderData._instanceColorsPtr) = instanceColorLocal;
                *(_renderData._worldRotationPtr) = worldRotationLocal;
                data.setValue(_renderData);
            }
        }
        else
        {
            unsigned int instanceCount = instanceCountLocal->second->data.getValue<unsigned int>();
            auto worldPositionLocal = inputs_named.find("worldPosition");
            bool worldPositionOkay = true;

            auto worldRotationLocal = inputs_named.find("worldRotation");
            bool worldRotationOkay = true;

            auto colorLocal = inputs_named.find("colors");
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
                    data.setDataChanged();
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
                    data.setDataChanged();
                }
            }

            if(worldRotationLocal == inputs_named.end())
            {
                worldRotationOkay = false;
            }
            else
            {
                if(worldRotationLocal->second->data.dataChanged())
                {
                    auto worldRotations = worldRotationLocal->second->data.getValue<std::vector<Vector3>>();
                    glm::mat4* worldRotLocal = *(_renderData._worldRotationPtr);

                    for(unsigned int i = 0; i < _renderData._instanceCount; i++)
                    {
                        worldRotLocal[i] = glm::eulerAngleYXZ(worldRotations[i].y, worldRotations[i].x, worldRotations[i].z);
                    }

                    *(_renderData._worldRotationPtr) = worldRotLocal;
                    data.setDataChanged();
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

                glm::mat4* worldRotLocal = *(_renderData._worldRotationPtr);
                if(worldRotLocal != nullptr)
                {
                    delete[] worldRotLocal;
                }
                worldRotLocal = new glm::mat4[instanceCount];

                if(worldRotationOkay)
                {
                    auto worldRotations = worldRotationLocal->second->data.getValue<std::vector<Vector3>>();

                    for(unsigned int i = 0; i < _renderData._instanceCount; i++)
                    {
                        worldRotLocal[i] = glm::eulerAngleYXZ(worldRotations[i].y, worldRotations[i].x, worldRotations[i].z);
                    }
                }
                else
                {
                    for(unsigned int i = 0; i < _renderData._instanceCount; i++)
                    {
                        worldRotLocal[i] = glm::mat4(1.0f);
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
                *(_renderData._worldRotationPtr) = worldRotLocal;
                *(_renderData._instanceColorsPtr) = colors;

                data.setValue(_renderData);
            }
        }

        // if mesh data changed
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
    }

private:
    inline float positionMaxFromArray(const Vector3* data, int offset)
    {
        const float* fdata = data->data;
        constexpr int stride = 3;
        float max = fdata[offset];
        for(int i = offset + stride; i < _renderData._instanceCount * stride; i += stride)
        {
            if(fdata[i] > max) max = fdata[i];
        }
        return max;
    }

    inline float positionMinFromArray(const Vector3* data, int offset)
    {
        const float* fdata = data->data;
        constexpr int stride = 3;
        float min = fdata[offset];
        for(int i = offset + stride; i < _renderData._instanceCount * stride; i += stride)
        {
            if(fdata[i] < min) min = fdata[i];
        }
        return min;
    }

    unsigned int _instanceCountLast = 0;
    RenderNodeData _renderData;
    bool _render_data_changed = false;
    bool _fog_changed_last_frame = false;
    bool _motif_changed_internal = false;
};
