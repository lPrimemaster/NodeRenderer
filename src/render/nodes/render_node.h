#pragma once
#include "node.h"
#include "mesh_node.h"
#include "mesh_interp_node.h"
#include "../node_outputs.h"
#include "../../../glm/glm/gtx/transform.hpp"
#include "../../../glm/glm/gtx/euler_angles.hpp"
#include <functional>
#include <algorithm>

struct RenderNode final : public PropertyNode
{
    inline RenderNode() : PropertyNode(Type::RENDER, 5, { "instanceCount", "worldPosition", "worldRotation", "mesh", "colors" }, 1, {})
    {
        static int inc = 0;

        _renderData._worldPositionPtr = (Vector4**)malloc(sizeof(Vector4*));
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
        L_TRACE("_motifPositionPtr : 0x%X", _renderData._motifPositionPtr);

        name = "Render Node #" + std::to_string(inc++);
        priority = PropertyNode::Priority::RENDER;

        // Raster
        inputs_description["instanceCount"] = 
            "The number of instances in the current motif. "
            "Must be equal to the size of the \"worldPosition\", \"worldRotation\" and \"colors\" lists."
        ;

        inputs_description["worldPosition"] = 
            "The positions for the current motif. "
            "If linked, must be equal to the size of the \"worldRotation\" and \"colors\" lists."
        ;

        inputs_description["worldRotation"] = 
            "The rotations for the current motif. "
            "If linked, must be equal to the size of the \"worldPosition\" and \"colors\" lists."
        ;

        inputs_description["colors"] = 
            "The colors for the current motif. "
            "If linked, must be equal to the size of the \"worldPosition\" and \"worldRotation\" lists."
        ;

        inputs_description["mesh"] = 
            "The mesh the current motif uses to render objects. "
            "If linked, must be a obj mesh file or a list of meshes (for interpolation scenarios)."
        ;


        // Raymarcher
        inputs_description["shader"] = 
            "The compute shader that renders the screen. "
            "The user code must specify a color for each screen pixel in this mode."
        ;

        outputs[0]->setValue(_renderData);
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

    inline void render_raster()
    {
        _motif_changed_internal = false;
        ImGui::BeginGroup();

        // Header flags
        static bool fog_header = false;
        static bool pos_header = false;

        // Fog settings header
        renderSubMenu("Fog Settings", ImVec4(179, 102, 0, 255), &fog_header, [&]() -> void {
            pos_header = false;
            bool fogmax = ImGui::InputFloat("Fog Maximum", &_renderData._fogMax, 1.0f, 0.0f, "%.1f");
            bool fogmin = ImGui::InputFloat("Fog Minimum", &_renderData._fogMin, 1.0f, 0.0f, "%.1f");
            bool fogcol = ImGui::ColorPicker3("Fog Color", _renderData._fogColor.data);
            _renderData._fogChanged = fogmax || fogmin || fogcol;

            if((fogmax || fogmin) && _renderData._repeatBlocks)
            {
                _motif_changed_internal = true;
            }

            if(_renderData._fogChanged)
            {
                _fog_changed_last_frame = true;
                outputs[0]->setValue(_renderData);
            }
            else if(_fog_changed_last_frame)
            {
                _fog_changed_last_frame = false;
                outputs[0]->setValue(_renderData);
            }
        });

        // Position settings header
        renderSubMenu("Position Settings", ImVec4(50, 205, 50, 255), &pos_header, [&]() -> void {
            fog_header = false;
            _motif_changed_internal = ImGui::Checkbox("Repeat Motif", &_renderData._repeatBlocks);
            if(_motif_changed_internal)
            {
                auto worldPositionLocal = inputs_named.find("worldPosition");
                if(worldPositionLocal != inputs_named.end())
                {
                    auto data = worldPositionLocal->second->getValue<std::vector<Vector3>>().data();
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
                if(ImGui::InputFloat3("Motif Size", _renderData._motifSize.data, "%.1f"))
                {
                    _motif_changed_internal = true;
                }
                else
                {
                    _motif_changed_internal = false;
                    _renderData._motifChanged = false;
                }

                outputs[0]->setValue(_renderData);
            }
        });

        ImGui::EndGroup();

        // Recalculate instances
        if(_motif_changed_internal || (_first_load && _renderData._repeatBlocks))
        {
            _first_load = false;
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
            outputs[0]->setValue(_renderData);
            _renderData._fogChanged = false;
        }
        else if(_renderData._motifChanged)
        {
            _renderData._motifChanged = false;
            outputs[0]->setValue(_renderData);
        }
    }

    inline void render_raymarch()
    {
        // TODO
    }

    // TODO: This node needs a refactor
    inline virtual void render() override
    {
        ImGui::Text("This node converts data to be rendered.");
        ImGui::TextColored(ImVec4(0.8f, 0.7f, 0.0f, 1.0f), "Warn: Data types are not checked!");

        static const char* const render_modes[] = { "Raster", "Raymarch" };

        if(ImGui::Combo("Render Mode", &internal_render_mode, render_modes, IM_ARRAYSIZE(render_modes)))
        {
            internal_render_mode_changed = true;
            _renderData._renderMode = static_cast<RenderNodeData::RenderMode>(internal_render_mode);
            outputs[0]->setValue(_renderData);
        }

        switch (internal_render_mode)
        {
        case 0: // Raster
            render_raster();
            break;
        
        case 1: // Raymarcher
            render_raymarch();
            break;
        }
    }

    inline void change_render_type()
    {
        internal_render_mode_changed = false;

        // Set all possible inputs to no connection and change them
        switch (internal_render_mode)
        {
        case 0: // Raster
        {
            disconnectInputIfNotOfType<EmptyType>("shader");

            setInputsOrdered({
                "instanceCount",
                "worldPosition",
                "worldRotation",
                "mesh",
                "colors"
            });
        }
        break;
        
        case 1: // Raymarcher
        {
            disconnectInputIfNotOfType<EmptyType>("instanceCount");
            disconnectInputIfNotOfType<EmptyType>("worldPosition");
            disconnectInputIfNotOfType<EmptyType>("worldRotation");
            disconnectInputIfNotOfType<EmptyType>("mesh");
            disconnectInputIfNotOfType<EmptyType>("colors");

            setInputsOrdered({
                "shader"
            });
        }
        break;
        }
    }

    inline void update_raster()
    {
        outputs[0]->resetDataUpdate();
        _render_data_changed = false;

        disconnectInputIfNotOfType<unsigned int>("instanceCount");
        disconnectInputIfNotOfType<std::vector<Vector3>>("worldPosition");
        disconnectInputIfNotOfType<std::vector<Vector3>>("worldRotation");
        disconnectInputIfNotOfType<MeshNodeData, MeshInterpListData>("mesh");
        disconnectInputIfNotOfType<std::vector<Vector4>>("colors");

        // Instance Count Handling
        auto instanceCountLocal = inputs_named.find("instanceCount");
        if(instanceCountLocal == inputs_named.end())
        {
            _instanceCountLast = 1;
            if(_instanceCountLast != _renderData._instanceCount)
            {
                _renderData._instanceCount = _instanceCountLast;

                Vector4* worldPosLocal = *(_renderData._worldPositionPtr);
                if(worldPosLocal != nullptr)
                {
                    delete[] worldPosLocal;
                }
                worldPosLocal = new Vector4[_instanceCountLast];

                auto worldPositionLocal = inputs_named.find("worldPosition");
                if(worldPositionLocal != inputs_named.end())
                {
                    auto worldPositions = worldPositionLocal->second->getValue<std::vector<Vector3>>();
                    worldPosLocal[0] = Vector4(
                        worldPositions[0].x, 
                        worldPositions[0].y,
                        worldPositions[0].z,
                        0.0f
                    );
                }
                else
                {
                    worldPosLocal[0] = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
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
                    auto colors = colorLocal->second->getValue<std::vector<Vector4>>();
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
                    auto rots = rotationLocal->second->getValue<std::vector<Vector4>>();
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
                outputs[0]->setValue(_renderData);
            }
        }
        else
        {
            unsigned int instanceCount = instanceCountLocal->second->getValue<unsigned int>();
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
                if(colorLocal->second->dataChanged())
                {
                    auto colors = colorLocal->second->getValue<std::vector<Vector4>>();
                    Vector4* color = *(_renderData._instanceColorsPtr);

                    if(_renderData._instanceCount > 0)
                        memcpy(color, colors.data(), colors.size() * sizeof(Vector4));

                    *(_renderData._instanceColorsPtr) = color;
                    outputs[0]->setDataChanged();
                }
            }

            if(worldPositionLocal == inputs_named.end())
            {
                worldPositionOkay = false;
            }
            else
            {
                if(worldPositionLocal->second->dataChanged())
                {
                    auto worldPositions = worldPositionLocal->second->getValue<std::vector<Vector3>>();
                    Vector4* worldPosLocal = *(_renderData._worldPositionPtr);

                    for(unsigned int i = 0; i < _renderData._instanceCount; i++)
                    {
                        worldPosLocal[i] = Vector4(
                            worldPositions[i].x, 
                            worldPositions[i].y,
                            worldPositions[i].z,
                            0.0f
                        );
                    }

                    *(_renderData._worldPositionPtr) = worldPosLocal;
                    outputs[0]->setDataChanged();
                }
            }

            if(worldRotationLocal == inputs_named.end())
            {
                worldRotationOkay = false;
            }
            else
            {
                if(worldRotationLocal->second->dataChanged())
                {
                    auto worldRotations = worldRotationLocal->second->getValue<std::vector<Vector3>>();
                    glm::mat4* worldRotLocal = *(_renderData._worldRotationPtr);

                    for(unsigned int i = 0; i < _renderData._instanceCount; i++)
                    {
                        worldRotLocal[i] = glm::eulerAngleYXZ(worldRotations[i].y, worldRotations[i].x, worldRotations[i].z);
                    }

                    *(_renderData._worldRotationPtr) = worldRotLocal;
                    outputs[0]->setDataChanged();
                }
            }

            if(_instanceCountLast != instanceCount)
            {
                _renderData._instanceCount = instanceCount;
                
                Vector4* worldPosLocal = *(_renderData._worldPositionPtr);
                if(worldPosLocal != nullptr)
                {
                    delete[] worldPosLocal;
                }
                worldPosLocal = new Vector4[instanceCount];

                if(worldPositionOkay)
                {
                    auto worldPositions = worldPositionLocal->second->getValue<std::vector<Vector3>>();

                    for(unsigned int i = 0; i < _renderData._instanceCount; i++)
                    {
                        worldPosLocal[i] = Vector4(
                            worldPositions[i].x, 
                            worldPositions[i].y,
                            worldPositions[i].z,
                            0.0f
                        );
                    }
                }
                else
                {
                    for(unsigned int i = 0; i < _renderData._instanceCount; i++)
                    {
                        worldPosLocal[i] = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
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
                    auto worldRotations = worldRotationLocal->second->getValue<std::vector<Vector3>>();

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
                    auto color = colorLocal->second->getValue<std::vector<Vector4>>();
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

                outputs[0]->setValue(_renderData);
            }
        }

        // if mesh data changed
        auto meshLocal = inputs_named.find("mesh");
        if(meshLocal != inputs_named.end())
        {
            if(meshLocal->second->dataChanged() || *(_renderData._meshPtr) == nullptr)
            {
                // We have a new mesh for displaying
                // Handle it

                if(meshLocal->second->isOfType<MeshNodeData>())
                {
                    auto newMeshPtr = meshLocal->second->getValuePtr<MeshNodeData>();
                    if(newMeshPtr->data_size > 0)
                    {
                        _render_data_changed = true;
                        *(_renderData._meshPtr) = newMeshPtr;
                        _renderData._meshCount = 1;
                        _renderData._meshParam = 0.0f;
                        outputs[0]->setValue(_renderData);
                    }
                }
                else
                {
                    auto newMeshListPtr = meshLocal->second->getValuePtr<MeshInterpListData>();
                    if(newMeshListPtr->meshes.size() > 0)
                    {
                        if(newMeshListPtr->changeParamOnly)
                        {
                            _renderData._meshParam = newMeshListPtr->t;
                            outputs[0]->setValue(_renderData);
                        }
                        else
                        {
                            _render_data_changed = true;
                            *(_renderData._meshPtr) = newMeshListPtr->meshes.data();
                            _renderData._meshCount = newMeshListPtr->meshes.size();
                            _renderData._meshParam = newMeshListPtr->t;
                            outputs[0]->setValue(_renderData);
                        }
                    }
                }
            }
        }
    }

    inline void update_raymarch()
    {
        outputs[0]->resetDataUpdate();
        _render_data_changed = false;

        disconnectInputIfNotOfType<ShaderNodeData>("shader");

        auto shaderNode = inputs_named.find("shader");
        if(shaderNode != inputs_named.end())
        {
            if(shaderNode->second->dataChanged())
            {
                // glsl needs to reload the shader
                _renderData._reloadShader = true;
                _renderData._glslCode = shaderNode->second->getValue<ShaderNodeData>().generated_code;
                outputs[0]->setValue(_renderData);
                // TODO: If compilation fails, draw the magenta shader (and display the user the errors)
            }
        }
    }

    inline virtual void update() override
    {
        if(internal_render_mode_changed)
        {
            change_render_type();
        }

        switch (internal_render_mode)
        {
        case 0: // Raster
            update_raster();
            break;
        
        case 1: // Raymarcher
            update_raymarch();
            break;
        }
    }

    inline virtual ByteBuffer serialize() const override
    {
        ByteBuffer buffer = PropertyNode::serialize();

        buffer.add(_renderData._renderMode);

        buffer.add(_renderData._fogMax);
        buffer.add(_renderData._fogMin);
        buffer.add(_renderData._fogColor.x);
        buffer.add(_renderData._fogColor.y);
        buffer.add(_renderData._fogColor.z);

        buffer.add(_renderData._repeatBlocks);
        buffer.add(_renderData._motifSize.x);
        buffer.add(_renderData._motifSize.y);
        buffer.add(_renderData._motifSize.z);

        return buffer;
    }

    inline virtual void deserialize(ByteBuffer& buffer) override
    {
        PropertyNode::deserialize(buffer);

        buffer.get(&_renderData._renderMode);
        internal_render_mode = static_cast<int>(_renderData._renderMode);

        buffer.get(&_renderData._fogMax);
        buffer.get(&_renderData._fogMin);
        buffer.get(&_renderData._fogColor.x);
        buffer.get(&_renderData._fogColor.y);
        buffer.get(&_renderData._fogColor.z);

        buffer.get(&_renderData._repeatBlocks);
        buffer.get(&_renderData._motifSize.x);
        buffer.get(&_renderData._motifSize.y);
        buffer.get(&_renderData._motifSize.z);

        _renderData._fogChanged = true;

        // Resetup this node
        _render_data_changed = true;
        _first_load = true;
        outputs[0]->setValue(_renderData);
    }

private:
    inline void renderSubMenu(const char* name, ImVec4 color, bool* flag, std::function<void()> func)
    {
        static std::unordered_map<std::string, int> id;
        static int sid = 0;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImVec2 cursor_start = ImGui::GetCursorScreenPos();
        const float text_spacing = ImGui::GetTextLineHeightWithSpacing();
        const float y_pad = (text_spacing - ImGui::GetTextLineHeight()) / 2.0f;
        const float x_pad = 15;
        const ImVec2 tree_size = ImVec2(300, text_spacing);
        const ImVec2 text_pad  = ImVec2(x_pad, y_pad);
        
        if(id.find(name) == id.end()) id[name] = sid++;
        
        (*flag) ^= ImGui::InvisibleButton((std::string("##tree_") + std::to_string(id[name])).c_str(), tree_size);
        draw_list->AddRectFilled(cursor_start, cursor_start + tree_size, ImGui::IsItemHovered() ? 
            IM_COL32(color.x, color.y, color.z, color.w) : 
            IM_COL32(color.x - 10 > 0 ? color.x - 10 : 0,
                     color.y - 10 > 0 ? color.y - 10 : 0,
                     color.z - 10 > 0 ? color.z - 10 : 0,
                     color.w
            )
        );
        draw_list->AddText(cursor_start + text_pad, IM_COL32_WHITE, name);

        ImVec2 tri_c = cursor_start + ImVec2(x_pad - 7.5f, text_spacing / 2);

        ImVec2 tri_0_rc = ImVec2(2.5f, 0.0f);
        ImVec2 tri_1_rc = ImVec2(-2.5f, -3);
        ImVec2 tri_2_rc = ImVec2(-2.5f,  3);

        auto rotate2D_90 = [](ImVec2& vec) -> void {
            float tx = vec.x;
            vec.x = -vec.y;
            vec.y = tx;
        };

        if(*flag)
        {
            // Open tree, rotate the triangle 90deg clockwise
            rotate2D_90(tri_0_rc);
            rotate2D_90(tri_1_rc);
            rotate2D_90(tri_2_rc);

            ImVec2 border_start = ImGui::GetItemRectMin();
            ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(5, 5));

            ImGui::BeginGroup();
            func();
            ImGui::EndGroup();

            ImVec2 border_end = ImVec2(cursor_start.x + tree_size.x, ImGui::GetItemRectMax().y + y_pad * 2);
            draw_list->AddRect(border_start, border_end, IM_COL32(color.x, color.y, color.z, color.w));
        }
        
        draw_list->AddTriangleFilled(
            tri_c + tri_0_rc,
            tri_c + tri_1_rc,
            tri_c + tri_2_rc,
            IM_COL32_WHITE
        );
    }

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

    int internal_render_mode = 0;
    bool internal_render_mode_changed = false;
    unsigned int _instanceCountLast = 0;
    RenderNodeData _renderData;
    bool _render_data_changed = false;
    bool _fog_changed_last_frame = false;
    bool _motif_changed_internal = false;
    bool _first_load = false;
};
