#pragma once
#include "node.h"

struct DisplayNode final : public PropertyNode
{
    inline DisplayNode() : PropertyNode(Type::DISPLAY, 1, { "in" }, 1, { "out" })
    {
        static int inc = 0;
        name = "Display Node #" + std::to_string(inc++);

        inputs_description["in"] = "Any numeric or vector value to be visualized.";

        // TODO: All output/input nodes will break when new types are added
        setOutputNominalTypes<float, int, unsigned int, Vector2, Vector3, Vector4>("out", 
            "The same value as the input value."
        );
    }
    
    ~DisplayNode() {  }

    inline virtual void render() override
    {
        auto in = inputs_named.find("in");
        if(in != inputs_named.end())
        {
            PropertyGenericData::TypeDataBuffer buffer = in->second->getValueDynamic();
            ImGui::BeginDisabled();
            switch (buffer.vtype)
            {
            case PropertyGenericData::ValidType::FLOAT: ImGui::InputFloat("value", (float*)buffer.data); break;
            case PropertyGenericData::ValidType::INT:   ImGui::InputInt("value", (int*)buffer.data); break;
            case PropertyGenericData::ValidType::UINT:  ImGui::InputScalar("value", ImGuiDataType_U32, buffer.data); break;
            case PropertyGenericData::ValidType::VECTOR2: ImGui::InputFloat2("value", ((Vector2*)buffer.data)->data); break;
            case PropertyGenericData::ValidType::VECTOR3: ImGui::InputFloat3("value", ((Vector3*)buffer.data)->data); break;
            case PropertyGenericData::ValidType::VECTOR4: ImGui::InputFloat4("value", ((Vector4*)buffer.data)->data); break;

            case PropertyGenericData::ValidType::LIST_FLOAT:
            case PropertyGenericData::ValidType::LIST_INT:
            case PropertyGenericData::ValidType::LIST_UINT:
            case PropertyGenericData::ValidType::LIST_VECTOR2:
            case PropertyGenericData::ValidType::LIST_VECTOR3:
            case PropertyGenericData::ValidType::LIST_VECTOR4:
                L_ERROR("Use a list view node instead!");
                disconnectInputIfNotOfType<EmptyType>("in");
                first_connect = true;
                break;
            }
            ImGui::EndDisabled();
        }
    }

    virtual void update() override
    {
        outputs[0]->resetDataUpdate();
        auto in = inputs_named.find("in");
        if(in != inputs_named.end() && (in->second->dataChanged() || first_connect))
        {
            first_connect = false;
            outputs[0]->setValueDynamic(in->second->getValueDynamic());
        }
    }

private:
    bool first_connect;
};
