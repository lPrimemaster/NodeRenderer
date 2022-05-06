#pragma once
#include "node.h"

struct ValueNode final : public PropertyNode
{
    enum class Type
    {
        FLOAT,
        INT,
        UINT
    } type;

    inline explicit ValueNode(float val = 0.0f) : PropertyNode(0.0f)
    {
        static int inc = 0;
        _input_count = 0;
        _output_count = 1;
        name = "Value Node #" + std::to_string(inc++);
        type = Type::FLOAT;
        data.setValue(val);
    }
    
    ~ValueNode() {  }

    inline virtual void render() override
    {
        data.resetDataUpdate();

        static const char* const type_names[] = {
            "float",
            "int",
            "uint"
        };

        ImGui::Combo("Type", &currenttypeid, type_names, sizeof(type_names) / sizeof(type_names[0]));
        type = static_cast<Type>(currenttypeid);


        switch (type)
        {
        case Type::FLOAT:
            if(currenttypeid != lasttypeid)
            {
                data.setValue<float>(1.0f);
                lasttypeid = currenttypeid;
            }
            if(ImGui::InputFloat("Value", &data.getValue<float>()))
            {
                data.setDataChanged();
            }
            break;
        case Type::INT:
            if(currenttypeid != lasttypeid)
            {
                data.setValue<int>(1);
                lasttypeid = currenttypeid;
            }
            if(ImGui::InputInt("Value", &data.getValue<int>()))
            {
                data.setDataChanged();
            }
            break;
        case Type::UINT:
            if(currenttypeid != lasttypeid)
            {
                data.setValue<unsigned int>(1U);
                lasttypeid = currenttypeid;
            }
            if(ImGui::InputScalar("Value", ImGuiDataType_U32, &data.getValue<unsigned int>()))
            {
                data.setDataChanged();
            }
            break;
        
        default:
            break;
        }
    }

private:
    int currenttypeid = 0;
    int lasttypeid = 0;
};
