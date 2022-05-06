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

        auto list_it = inputs_named.find("list");
        if(list_it != inputs_named.end())
        {
            ImGui::BeginDisabled();
            if(list_it->second->data.isOfType<std::vector<float>>())
            {
                auto list = list_it->second->data.getValue<std::vector<float>>();
                if(idx >= list.size())
                {
                    idx = list.size() - 1;
                }

                float value = list.at(idx);
                data.setValue(value);
                ImGui::InputFloat("Value", &value);
            }
            else if(list_it->second->data.isOfType<std::vector<int>>())
            {
                auto list = list_it->second->data.getValue<std::vector<int>>();
                if(idx >= list.size())
                {
                    idx = list.size() - 1;
                }

                int value = list.at(idx);
                data.setValue(value);
                ImGui::InputInt("Value", &value);
            }
            else if(list_it->second->data.isOfType<std::vector<unsigned int>>())
            {
                auto list = list_it->second->data.getValue<std::vector<unsigned int>>();
                if(idx >= list.size())
                {
                    idx = list.size() - 1;
                }

                unsigned int value = list.at(idx);
                data.setValue(value);
                ImGui::InputScalar("Value", ImGuiDataType_U32, &value);
            }
            else if(list_it->second->data.isOfType<std::vector<Vector2>>())
            {
                auto list = list_it->second->data.getValue<std::vector<Vector2>>();
                if(idx >= list.size())
                {
                    idx = list.size() - 1;
                }

                Vector2 value = list.at(idx);
                data.setValue(value);
                ImGui::InputFloat2("Value", value.data);
            }
            else if(list_it->second->data.isOfType<std::vector<Vector3>>())
            {
                auto list = list_it->second->data.getValue<std::vector<Vector3>>();
                if(idx >= list.size())
                {
                    idx = list.size() - 1;
                }

                Vector3 value = list.at(idx);
                data.setValue(value);
                ImGui::InputFloat3("Value", value.data);
            }
            else if(list_it->second->data.isOfType<std::vector<Vector4>>())
            {
                auto list = list_it->second->data.getValue<std::vector<Vector4>>();
                if(idx >= list.size())
                {
                    idx = list.size() - 1;
                }

                Vector4 value = list.at(idx);
                data.setValue(value);
                ImGui::InputFloat4("Value", value.data);
            }
            ImGui::EndDisabled();
        }
    }

private:
    int idx = 0;
};
