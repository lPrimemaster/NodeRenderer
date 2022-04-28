#pragma once
#include "node.h"

struct MeshNode final : public PropertyNode
{
    inline MeshNode() : PropertyNode()
    {
        static int inc = 0;
        _input_count = 0;
        _output_count = 1;
        name = "Mesh Node #" + std::to_string(inc++);
    }
    
    ~MeshNode() {  }

    inline virtual void render() override
    {
        ImGui::InputFloat4("Value", data.getValue<float*>());
    }
};
