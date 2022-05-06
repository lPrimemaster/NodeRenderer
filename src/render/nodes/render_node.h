#pragma once
#include "node.h"
#include "mesh_node.h"
#include "../../math/vector.h"
#include "../../../glm/glm/glm.hpp"
#include "../../../glm/glm/gtx/transform.hpp"

struct RenderNodeData
{
    unsigned int _instanceCount = 0;
    glm::mat4**  _worldPositionPtr = nullptr;
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
                "mesh"
            }
        );
        _output_count = 0;

        _renderData._worldPositionPtr = (glm::mat4**)malloc(sizeof(glm::mat4*));
        *(_renderData._worldPositionPtr) = nullptr;
        L_TRACE("_worldPositionPtr : 0x%X", _renderData._worldPositionPtr);

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

        // Instance Count Handling
        auto instanceCountLocal = inputs_named.find("instanceCount");
        if(instanceCountLocal == inputs_named.end())
        {
            _instanceCountLast = 1;
            if(_instanceCountLast != _renderData._instanceCount)
            {
                glm::mat4* worldPosLocal = *(_renderData._worldPositionPtr);
                _renderData._instanceCount = _instanceCountLast;

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
                _render_data_changed = true;
                *(_renderData._worldPositionPtr) = worldPosLocal;
                data.setValue(_renderData);
            }
        }
        else
        {
            unsigned int instanceCount = instanceCountLocal->second->data.getValue<unsigned int>();
            auto worldPositionLocal = inputs_named.find("worldPosition");
            bool worldPositionOkay = true;
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

                    for(int i = 0; i < _renderData._instanceCount; i++)
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
                glm::mat4* worldPosLocal = *(_renderData._worldPositionPtr);
                _renderData._instanceCount = instanceCount;

                if(worldPosLocal != nullptr)
                {
                    delete[] worldPosLocal;
                }
                worldPosLocal = new glm::mat4[instanceCount];

                if(worldPositionOkay)
                {
                    auto worldPositions = worldPositionLocal->second->data.getValue<std::vector<Vector3>>();

                    for(int i = 0; i < _renderData._instanceCount; i++)
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
                    for(int i = 0; i < _renderData._instanceCount; i++)
                    {
                        worldPosLocal[i] = glm::mat4(1.0f);
                    }
                }

                _instanceCountLast = instanceCount;
                _render_data_changed = true;
                *(_renderData._worldPositionPtr) = worldPosLocal;
                data.setValue(_renderData);
            }
        }

        ImGui::Text("This node converts data to be rendered.");
        ImGui::Text("Warn: Data types are not checked!");
    }
private:
    unsigned int _instanceCountLast = 0;
    RenderNodeData _renderData;
    bool _render_data_changed = false;
};
