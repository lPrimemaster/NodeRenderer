#pragma once
#include "node.h"
#include "../../math/vector.h"
#include "../../../glm/glm/glm.hpp"
#include "../../../muparser/include/muParser.h"

struct ListAccessNode final : public PropertyNode
{
    inline ListAccessNode() : PropertyNode(Type::LISTACCESS, 2, { "index", "list" }, 1, { "value" })
    {
        static int inc = 0;
        name = "List Access Node #" + std::to_string(inc++);

        inputs_description["index"] = "The list index to lookup.";
        inputs_description["list"] = "The list object to lookup.";

        setOutputNominalTypes<float>("value", 
            "The list index corresponding value."
        );

    }
    
    ~ListAccessNode()
    {
        
    }

    enum class DisplayType
    {
        FLOAT,
        INT,
        UNSIGNED_INT,
        VECTOR2,
        VECTOR3,
        VECTOR4
    };

    inline virtual void render() override
    {
        auto data = outputs[0];
        auto idx_it = inputs_named.find("index");
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

        if(list_connected)
        {
            ImGui::BeginDisabled(!mod && !val_connected);
            switch(dtype)
            {
            case DisplayType::FLOAT:
                ImGui::InputFloat("Value", data->getValuePtr<float>());
                break;
            case DisplayType::INT:
                ImGui::InputInt("Value", data->getValuePtr<int>());
                break;
            case DisplayType::UNSIGNED_INT:
                ImGui::InputScalar("Value", ImGuiDataType_U32, data->getValuePtr<unsigned int>());
                break;
            case DisplayType::VECTOR2:
                ImGui::InputFloat2("Value", data->getValuePtr<Vector2>()->data);
                break;
            case DisplayType::VECTOR3:
                ImGui::InputFloat3("Value", data->getValuePtr<Vector3>()->data);
                break;
            case DisplayType::VECTOR4:
                ImGui::InputFloat4("Value", data->getValuePtr<Vector4>()->data);
                break;
            }
            ImGui::EndDisabled();
        }
    }

    inline virtual void update() override
    {
        auto data = outputs[0];
        data->resetDataUpdate();

        disconnectInputIfNotOfType<unsigned int, int>("index");

        disconnectInputIfNotOfType<
            std::vector<float>, 
            std::vector<int>, 
            std::vector<unsigned int>,
            std::vector<Vector2>,
            std::vector<Vector3>,
            std::vector<Vector4>
        >("list");

        auto idx_it = inputs_named.find("index");
        if(idx_it != inputs_named.end())
        {
            if(idx_it->second->isOfType<unsigned int>())
            {
                idx = (int)idx_it->second->getValue<unsigned int>();
            }
            else if(idx_it->second->isOfType<int>())
            {
                idx = idx_it->second->getValue<int>();
            }
            if(idx < 0) idx = 0;
        }

        auto list_it = inputs_named.find("list");

        // Is there a value input connected?
        auto value = inputs_named.find("value");
        val_connected = (value != inputs_named.end());
        list_connected = (list_it != inputs_named.end());

        if(list_it != inputs_named.end())
        {
            if(list_it->second->isOfType<std::vector<float>>())
            {
                auto& list = list_it->second->getValue<std::vector<float>>();
                if(idx >= list.size())
                {
                    idx = (int)list.size() - 1;
                }

                dtype = DisplayType::FLOAT;

                if(val_connected && !disconnectInputIfNotOfType<float>("value"))
                {
                    auto& valueData = value->second;
                    auto valueValue = valueData->getValue<float>();

                    if(valueData->dataChanged() || (valueValue != list[idx]))
                    {
                        list[idx] = valueValue;
                    }
                }
                data->setValue(list[idx]);
            }
            else if(list_it->second->isOfType<std::vector<int>>())
            {
                auto& list = list_it->second->getValue<std::vector<int>>();
                if(idx >= list.size())
                {
                    idx = (int)list.size() - 1;
                }

                dtype = DisplayType::INT;

                if(val_connected && !disconnectInputIfNotOfType<int>("value"))
                {
                    auto& valueData = value->second;
                    auto valueValue = valueData->getValue<int>();

                    if(valueData->dataChanged() || (valueValue != list[idx]))
                    {
                        list[idx] = valueValue;
                    }
                }
                data->setValue(list[idx]);
            }
            else if(list_it->second->isOfType<std::vector<unsigned int>>())
            {
                auto& list = list_it->second->getValue<std::vector<unsigned int>>();
                if(idx >= list.size())
                {
                    idx = (int)list.size() - 1;
                }

                dtype = DisplayType::UNSIGNED_INT;
                

                if(val_connected && !disconnectInputIfNotOfType<unsigned int>("value"))
                {
                    auto& valueData = value->second;
                    auto valueValue = valueData->getValue<unsigned int>();

                    if(valueData->dataChanged() || (valueValue != list[idx]))
                    {
                        list[idx] = valueValue;
                    }
                data->setValue(list[idx]);
                }
            }
            else if(list_it->second->isOfType<std::vector<Vector2>>())
            {
                auto& list = list_it->second->getValue<std::vector<Vector2>>();
                if(idx >= list.size())
                {
                    idx = (int)list.size() - 1;
                }

                dtype = DisplayType::VECTOR2;

                if(val_connected && !disconnectInputIfNotOfType<Vector2>("value"))
                {
                    auto& valueData = value->second;
                    auto valueValue = valueData->getValue<Vector2>();

                    if(valueData->dataChanged() || (valueValue != list[idx]))
                    {
                        list[idx] = valueValue;
                    }
                }
                data->setValue(list[idx]);
            }
            else if(list_it->second->isOfType<std::vector<Vector3>>())
            {
                auto& list = list_it->second->getValue<std::vector<Vector3>>();
                if(idx >= list.size())
                {
                    idx = (int)list.size() - 1;
                }

                dtype = DisplayType::VECTOR3;

                if(val_connected && !disconnectInputIfNotOfType<Vector3>("value"))
                {
                    auto& valueData = value->second;
                    auto valueValue = valueData->getValue<Vector3>();

                    if(valueData->dataChanged() || (valueValue != list[idx]))
                    {
                        list[idx] = valueValue;
                    }
                }
                data->setValue(list[idx]);
            }
            else if(list_it->second->isOfType<std::vector<Vector4>>())
            {
                auto& list = list_it->second->getValue<std::vector<Vector4>>();
                if(idx >= list.size())
                {
                    idx = (int)list.size() - 1;
                }

                dtype = DisplayType::VECTOR4;
                
                if(val_connected && !disconnectInputIfNotOfType<Vector4>("value"))
                {
                    auto& valueData = value->second;
                    auto valueValue = valueData->getValue<Vector4>();

                    if(valueData->dataChanged() || (valueValue != list[idx]))
                    {
                        list[idx] = valueValue;
                    }
                }
                data->setValue(list[idx]);
            }
        }
    }

    inline virtual ByteBuffer serialize() const override
    {
        ByteBuffer buffer = PropertyNode::serialize();

        buffer.add(idx);
        buffer.add(mod);

        return buffer;
    }

    inline virtual void deserialize(ByteBuffer& buffer) override
    {
        PropertyNode::deserialize(buffer);
        
        buffer.get(&idx);
        buffer.get(&mod);

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
            setInputsOrdered(
                {
                    "index",
                    "list"
                }
            );
        }

    }

private:
    int idx = 0;
    bool mod = false;
    bool val_connected = false;
    bool list_connected = false;

    DisplayType dtype;
};
