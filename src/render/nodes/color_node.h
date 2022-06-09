#pragma once
#include "node.h"
#include "../../math/vector.h"

struct ColorNode final : public PropertyNode
{
    inline ColorNode() : PropertyNode(0, {}, 1, { "value" })
    {
        static int inc = 0;
        name = "Color Node #" + std::to_string(inc++);
    }
    
    ~ColorNode() {  }

    inline virtual void render() override
    {
        resetOutputsDataUpdate();

        if(ImGui::ColorPicker4("Color", outputs_named["value"]->getValue<Vector4>().data))
        {
            outputs_named["value"]->setDataChanged();
        }
    }
};
