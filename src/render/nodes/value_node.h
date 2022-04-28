#pragma once
#include "node.h"

struct ValueNode final : public PropertyNode
{
    inline explicit ValueNode(float val = 0.0f) : PropertyNode(0.0f)
    {
        static int inc = 0;
        _input_count = 0;
        _output_count = 1;
        name = "Value Node #" + std::to_string(inc++);
        data.setValue(0.0f);
    }
    
    ~ValueNode() {  }

    inline virtual void render() override
    {
        ImGui::InputFloat("Value", &data.getValue<float>());
    }
};
