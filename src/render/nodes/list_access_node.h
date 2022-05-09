#pragma once
#include "node.h"
#include "../../math/vector.h"
#include "../../../glm/glm/glm.hpp"
#include "../../../muparser/include/muParser.h"

struct ListAccessNode final : public PropertyNode
{
    inline ListAccessNode() : PropertyNode()
    {
        static int inc = 0;
        setInputsOrdered(
            {
                "index",
                "list"
            }
        );
        _output_count = 1;
        
        name = "List Access Node #" + std::to_string(inc++);
    }
    
    ~ListAccessNode()
    {

    }

    inline virtual void render() override
    {
        data.resetDataUpdate();

        disconnectInputIfNotOfType<unsigned int, int>("index");

        disconnectInputIfNotOfType<
            std::vector<float>, 
            std::vector<int>, 
            std::vector<unsigned int>,
            std::vector<Vector2>,
            std::vector<Vector3>,
            std::vector<Vector4>
        >("list");

        // TODO: Make own map implementation with cached iterators for every frame
        auto idx_it = inputs_named.find("index");
        if(idx_it != inputs_named.end())
        {
            if(idx_it->second->data.isOfType<unsigned int>())
            {
                idx = (int)idx_it->second->data.getValue<unsigned int>();
            }
            else if(idx_it->second->data.isOfType<int>())
            {
                idx = idx_it->second->data.getValue<int>();
            }
        }

        if(idx_it != inputs_named.end())
        {
            ImGui::BeginDisabled();
            ImGui::InputInt("Index", &idx);
            ImGui::EndDisabled();
        }
        else
        {
            ImGui::InputInt("Index", &idx);
        }

        if(idx < 0) idx = 0;

        if(ImGui::Checkbox("Modifiable", &mod))
        {
            if(mod)
            {
                setInputsOrdered(
                    {
                        "index",
                        "list",
                        "value"
                    }
                );
            }
            else
            {
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("value");
                setInputsOrdered(
                    {
                        "index",
                        "list"
                    }
                );
            }
        }

        auto list_it = inputs_named.find("list");

        // Is there a value input connected?
        auto value = inputs_named.find("value");
        bool val_connected = (value != inputs_named.end());

        if(list_it != inputs_named.end())
        {
            ImGui::BeginDisabled(!mod && !val_connected);
            if(list_it->second->data.isOfType<std::vector<float>>())
            {
                auto& list = list_it->second->data.getValue<std::vector<float>>();
                if(idx >= list.size())
                {
                    idx = list.size() - 1;
                }

                data.setValue(list[idx]);
                ImGui::InputFloat("Value", list.data() + idx);

                if(val_connected && !disconnectInputIfNotOfType<float>("value"))
                {
                    auto& valueData = value->second->data;
                    auto valueValue = valueData.getValue<float>();

                    if(valueData.dataChanged() || (valueValue != list[idx]))
                    {
                        list[idx] = valueValue;
                    }
                }
            }
            else if(list_it->second->data.isOfType<std::vector<int>>())
            {
                auto& list = list_it->second->data.getValue<std::vector<int>>();
                if(idx >= list.size())
                {
                    idx = list.size() - 1;
                }

                data.setValue(list[idx]);
                ImGui::InputInt("Value", list.data() + idx);

                if(val_connected && !disconnectInputIfNotOfType<int>("value"))
                {
                    auto& valueData = value->second->data;
                    auto valueValue = valueData.getValue<int>();

                    if(valueData.dataChanged() || (valueValue != list[idx]))
                    {
                        list[idx] = valueValue;
                    }
                }
            }
            else if(list_it->second->data.isOfType<std::vector<unsigned int>>())
            {
                auto& list = list_it->second->data.getValue<std::vector<unsigned int>>();
                if(idx >= list.size())
                {
                    idx = list.size() - 1;
                }

                data.setValue(list[idx]);
                ImGui::InputScalar("Value", ImGuiDataType_U32, list.data() + idx);

                if(val_connected && !disconnectInputIfNotOfType<unsigned int>("value"))
                {
                    auto& valueData = value->second->data;
                    auto valueValue = valueData.getValue<unsigned int>();

                    if(valueData.dataChanged() || (valueValue != list[idx]))
                    {
                        list[idx] = valueValue;
                    }
                }
            }
            else if(list_it->second->data.isOfType<std::vector<Vector2>>())
            {
                auto& list = list_it->second->data.getValue<std::vector<Vector2>>();
                if(idx >= list.size())
                {
                    idx = list.size() - 1;
                }

                data.setValue(list[idx]);
                ImGui::InputFloat2("Value", (list.data() + idx)->data);

                if(val_connected && !disconnectInputIfNotOfType<Vector2>("value"))
                {
                    auto& valueData = value->second->data;
                    auto valueValue = valueData.getValue<Vector2>();

                    if(valueData.dataChanged() || (valueValue != list[idx]))
                    {
                        list[idx] = valueValue;
                    }
                }
            }
            else if(list_it->second->data.isOfType<std::vector<Vector3>>())
            {
                auto& list = list_it->second->data.getValue<std::vector<Vector3>>();
                if(idx >= list.size())
                {
                    idx = list.size() - 1;
                }

                data.setValue(list[idx]);
                ImGui::InputFloat3("Value", (list.data() + idx)->data);

                if(val_connected && !disconnectInputIfNotOfType<Vector3>("value"))
                {
                    auto& valueData = value->second->data;
                    auto valueValue = valueData.getValue<Vector3>();

                    if(valueData.dataChanged() || (valueValue != list[idx]))
                    {
                        list[idx] = valueValue;
                    }
                }
            }
            else if(list_it->second->data.isOfType<std::vector<Vector4>>())
            {
                auto& list = list_it->second->data.getValue<std::vector<Vector4>>();
                if(idx >= list.size())
                {
                    idx = list.size() - 1;
                }

                data.setValue(list[idx]);
                ImGui::InputFloat4("Value", (list.data() + idx)->data);
                
                if(val_connected && !disconnectInputIfNotOfType<Vector4>("value"))
                {
                    auto& valueData = value->second->data;
                    auto valueValue = valueData.getValue<Vector4>();

                    if(valueData.dataChanged() || (valueValue != list[idx]))
                    {
                        list[idx] = valueValue;
                    }
                }
            }
            ImGui::EndDisabled();
        }
    }

private:
    int idx = 0;
    bool mod = false;
};
