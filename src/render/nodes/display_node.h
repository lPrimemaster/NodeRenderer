#pragma once
#include "node.h"

struct DisplayNode final : public PropertyNode
{
    inline DisplayNode() : PropertyNode(1, { "in" }, 1, { "out" })
    {
        static int inc = 0;
        name = "Display Node #" + std::to_string(inc++);
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
                break;
            }
            ImGui::EndDisabled();
        }


        
    }

    virtual void update() override
    {
        outputs[0]->resetDataUpdate();
        auto in = inputs_named.find("in");
        if(in != inputs_named.end() && in->second->dataChanged())
        {
            outputs[0]->setValueDynamic(in->second->getValueDynamic());
        }
    }
};
