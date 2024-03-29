#pragma once
#include "node.h"
#include "../../math/vector.h"
#include "../../math/comb.inl"
#include "../renderer.h"
#include <numeric>

struct CameraNode final : public PropertyNode
{
    using NodeType = PropertyNode::Type;

    inline CameraNode(Renderer::Camera* camera) : PropertyNode(NodeType::CAMERA, 2, { "position", "lookAt" }, 0), cameraPosition(camera->getPosition()), camera(camera)
    {
        static int inc = 0;
        name = "Camera Node #" + std::to_string(inc++);
    }
    
    ~CameraNode() {  }

    enum class Type
    {
        ORBIT,
        FREE
    };

    inline virtual void render() override
    {
        static const char* const type_names[] = {
            "Orbit",
            "Free"
        };

        if(ImGui::Combo("Type", &currenttypeid, type_names, sizeof(type_names) / sizeof(type_names[0])))
        {
            type = static_cast<Type>(currenttypeid);

            switch (type)
            {
            case Type::ORBIT:
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("forward");
                setInputsOrdered(
                    {
                        "position",
                        "lookAt"
                    }
                );
                break;
            case Type::FREE:
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("lookAt");
                setInputsOrdered(
                    {
                        "position",
                        "forward"
                    }
                );
                break;

            default:
                break;
            }
        }

        ImGui::Checkbox("Automatic camera enabled", &enabled);
    }

    inline virtual void update() override
    {
        disconnectInputIfNotOfType<Vector3>("position");
        auto pos_in = inputs_named.find("position");
        bool posChanged = false;
        if(pos_in != inputs_named.end())
        {
            if(pos_in->second->dataChanged())
            {
                Vector3 position = pos_in->second->getValue<Vector3>();
                cameraPosition = position;
                posChanged = true;
            }
        }

        switch (type)
        {
        case Type::ORBIT:
        {
            disconnectInputIfNotOfType<Vector3>("lookAt");
            auto look_in = inputs_named.find("lookAt");
            if(look_in != inputs_named.end())
            {
                if(look_in->second->dataChanged() || posChanged)
                {
                    Vector3 lookVec = look_in->second->getValue<Vector3>();
                    cameraForward = Vector3::Normalize(lookVec - cameraPosition);
                }
            }
        }
        break;
        case Type::FREE:
        {
            disconnectInputIfNotOfType<Vector3>("forward");
            auto forward_in = inputs_named.find("forward");
            if(forward_in != inputs_named.end())
            {
                if(forward_in->second->dataChanged())
                {
                    Vector3 forward = forward_in->second->getValue<Vector3>();
                    cameraForward = Vector3::Normalize(forward);
                }
            }
        }
        break;

        default:
            break;
        }

        if(enabled)
        {
            this->camera->setPositionAndForwardVectorsAutomatic(cameraPosition, cameraForward);
        }
    }

    inline virtual ByteBuffer serialize() const override
    {
        ByteBuffer buffer = PropertyNode::serialize();

        buffer.add(currenttypeid);

        buffer.add(enabled);

        return buffer;
    }

    inline virtual void deserialize(ByteBuffer& buffer) override
    {
        PropertyNode::deserialize(buffer);

        buffer.get(&currenttypeid);

        type = static_cast<Type>(currenttypeid);

        switch (type)
        {
        case Type::ORBIT:
            disconnectInputIfNotOfType<PropertyNode::EmptyType>("forward");
            setInputsOrdered(
                {
                    "position",
                    "lookAt"
                }
            );
            break;
        case Type::FREE:
            disconnectInputIfNotOfType<PropertyNode::EmptyType>("lookAt");
            setInputsOrdered(
                {
                    "position",
                    "forward"
                }
            );
            break;

        default:
            break;
        }

        buffer.get(&enabled);

    }

private:
    int currenttypeid = 0;
    Type type = Type::ORBIT;

    Vector3 cameraPosition;
    Vector3 cameraForward;

    bool enabled = true;

    Renderer::Camera* camera;
};
