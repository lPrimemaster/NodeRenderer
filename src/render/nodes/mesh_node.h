#pragma once
#include "node.h"

struct MeshNodeData
{
    
};

struct MeshNode final : public PropertyNode
{
    inline MeshNode() : PropertyNode(MeshNodeData())
    {
        static int inc = 0;
        _input_count = 0;
        _output_count = 1;
        name = "Mesh Node #" + std::to_string(inc++);
    }
    
    ~MeshNode() {  }

    inline virtual void render() override
    {
        data.resetDataUpdate();
        // TODO
        ImGui::InputFloat4("Value", data.getValue<float*>());
    }
};
