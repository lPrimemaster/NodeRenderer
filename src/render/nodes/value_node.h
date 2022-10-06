#pragma once
#include "node.h"

struct ValueNode final : public PropertyNode
{
    using NodeType = PropertyNode::Type;
    using Type = PropertyGenericData::ValidType;

    inline explicit ValueNode(float val = 0.0f) : PropertyNode(NodeType::VALUE, 0, {}, 1, { "value" })
    {
        static int inc = 0;
        name = "Value Node #" + std::to_string(inc++);
        outputs[0]->setValue(val);

        setOutputNominalTypes<float, int, unsigned int, Vector2, Vector3, Vector4>(
            "value",
            "Returns the inserted value."
        );
    }
    
    ~ValueNode() {  }

    inline virtual void render() override
    {
        outputs[0]->resetDataUpdate();

        static const char* const type_names[] = {
            "float",
            "int",
            "uint",
            "Vector2",
            "Vector3",
            "Vector4"
        };

        ImGui::Combo("Type", &currenttypeid, type_names, sizeof(type_names) / sizeof(type_names[0]));
        Type type = static_cast<Type>(currenttypeid);


        switch (type)
        {
        case Type::FLOAT:
            if(currenttypeid != lasttypeid)
            {
                outputs[0]->setValue<float>(1.0f);
                lasttypeid = currenttypeid;
            }
            if(ImGui::InputFloat("value", &outputs[0]->getValue<float>()))
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
            if(ImGui::InputInt("value", &outputs[0]->getValue<int>()))
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
            if(ImGui::InputScalar("value", ImGuiDataType_U32, &outputs[0]->getValue<unsigned int>()))
            {
                outputs[0]->setDataChanged();
            }
            break;
        case Type::VECTOR2:
            if(currenttypeid != lasttypeid)
            {
                outputs[0]->setValue<Vector2>(Vector2(1, 1));
                lasttypeid = currenttypeid;
            }
            if(ImGui::InputFloat2("value", outputs[0]->getValue<Vector2>().data))
            {
                outputs[0]->setDataChanged();
            }
            break;
        case Type::VECTOR3:
            if(currenttypeid != lasttypeid)
            {
                outputs[0]->setValue<Vector3>(Vector3(1, 1, 1));
                lasttypeid = currenttypeid;
            }
            if(ImGui::InputFloat3("value", outputs[0]->getValue<Vector3>().data))
            {
                outputs[0]->setDataChanged();
            }
            break;
        case Type::VECTOR4:
            if(currenttypeid != lasttypeid)
            {
                outputs[0]->setValue<Vector4>(Vector4(1, 1, 1, 1));
                lasttypeid = currenttypeid;
            }
            if(ImGui::InputFloat4("value", outputs[0]->getValue<Vector4>().data))
            {
                outputs[0]->setDataChanged();
            }
            break;
        
        default:
            break;
        }
    }

    inline virtual ByteBuffer serialize() const override
    {
        ByteBuffer buffer = PropertyNode::serialize();

        buffer.add(currenttypeid);

        return buffer;
    }

    inline virtual void deserialize(ByteBuffer& buffer) override
    {
        PropertyNode::deserialize(buffer);

        buffer.get(&currenttypeid);
        lasttypeid = currenttypeid;
    }

private:
    int currenttypeid = 0;
    int lasttypeid = 0;
};
