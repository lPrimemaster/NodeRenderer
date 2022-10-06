#pragma once
#include "node.h"

struct FeedbackNode final : public PropertyNode
{
    inline FeedbackNode() : PropertyNode(Type::FEEDBACK, 1, { "in" }, 1, { "out" })
    {
        static int inc = 0;
        name = "Feedback Node #" + std::to_string(inc++);
        priority = PropertyNode::Priority::FEEDBACK;
    }
    
    ~FeedbackNode() {  }

    inline virtual void render() override
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "This node is obsolete.");
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
                L_ERROR("Feedback node does not support lists!");
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
    bool first_connect = true;
};
