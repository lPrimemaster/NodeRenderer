#pragma once
#include "node.h"
#include "../../math/vector.h"

struct WorldPosNode final : public PropertyNode
{
    inline WorldPosNode() : PropertyNode(Vector3(0.1f, 0.1f, 0.1f))
    {
        static int inc = 0;
        _input_count = 0;
        _output_count = 1;
        name = "WorldPos Node #" + std::to_string(inc++);
    }
    
    ~WorldPosNode() {  }

    inline virtual void render() override
    {
        ImGui::InputFloat3("Sample Delta", data.getValue<Vector3>().data);
    }
};