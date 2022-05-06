#pragma once
#include "node.h"
#include "../../math/vector.h"

struct ColorNode final : public PropertyNode
{
    inline ColorNode(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) : PropertyNode(Vector4(r, g, b, a))
    {
        static int inc = 0;
        _input_count = 0;
        _output_count = 1;
        name = "Color Node #" + std::to_string(inc++);
    }
    
    ~ColorNode() {  }

    inline virtual void render() override
    {
        data.resetDataUpdate();
        if(ImGui::ColorPicker4("Color", data.getValue<Vector4>().data))
        {
            data.setDataChanged();
        }
    }
};
