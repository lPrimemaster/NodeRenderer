#pragma once
#include "node.h"

// TODO
struct HistogramNode final : public PropertyNode
{
    inline HistogramNode() : PropertyNode(Type::HISTOGRAM, 1, { "in" }, 0)
    {
        static int inc = 0;
        name = "Histogram Node #" + std::to_string(inc++);
    }
    
    ~HistogramNode() {  }

    inline virtual void render() override
    {
        auto in = inputs_named.find("in");
        if(in != inputs_named.end())
        {
            PropertyGenericData::TypeDataBuffer buffer = in->second->getValueDynamic();
            
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
                L_ERROR("Histograms don't support list inputs!");
                disconnectInputIfNotOfType<EmptyType>("in");
                first_connect = true;
                break;
            }
            ImGui::Text("WIP");
            // ImGui::PlotHistogram("Plotting: " + in->second->_data_holder_instance->name, );

        }
    }

    virtual void update() override
    {
        
    }

private:
    bool first_connect;
};
