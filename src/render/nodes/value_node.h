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

    inline explicit ValueNode(float val = 0.0f) : PropertyNode(0, {}, 1, { "value" })
    {
        static int inc = 0;
        name = "Value Node #" + std::to_string(inc++);
        type = Type::FLOAT;
        outputs[0]->setValue(val);
    }
    
    ~ValueNode() {  }

    inline virtual void render() override
    {
        outputs[0]->resetDataUpdate();

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
                outputs[0]->setValue<float>(1.0f);
                lasttypeid = currenttypeid;
            }
            if(ImGui::InputFloat("Value", &outputs[0]->getValue<float>()))
            {
                outputs[0]->setDataChanged();
            }
            break;
        case Type::INT:
            if(currenttypeid != lasttypeid)
            {
                outputs[0]->setValue<int>(1);
                lasttypeid = currenttypeid;
            }
            if(ImGui::InputInt("Value", &outputs[0]->getValue<int>()))
            {
                outputs[0]->setDataChanged();
            }
            break;
        case Type::UINT:
            if(currenttypeid != lasttypeid)
            {
                outputs[0]->setValue<unsigned int>(1U);
                lasttypeid = currenttypeid;
            }
            if(ImGui::InputScalar("Value", ImGuiDataType_U32, &outputs[0]->getValue<unsigned int>()))
            {
                outputs[0]->setDataChanged();
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
