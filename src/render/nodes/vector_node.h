#pragma once
#include "node.h"
#include "../../math/vector.h"

struct VectorNode final : public PropertyNode
{
    inline VectorNode(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f) : PropertyNode(Vector4(x, y, z, w))
    {
        static int inc = 0;
        _input_count = 0;
        _output_count = 1;
        name = "Vector Node #" + std::to_string(inc++);
    }
    
    ~VectorNode() {  }

    inline virtual void render() override
    {
        ImGui::InputFloat4("Value", data.getValue<Vector4>().data);
    }
};