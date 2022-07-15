#pragma once
#include "node.h"
#include "../../math/vector.h"
#include "../renderer.h"

struct WorldPosNode final : public PropertyNode
{
    inline WorldPosNode(Renderer::Camera* camera) : PropertyNode(Type::WORLDPOS, 0, {}, 1, {}), camera(camera)
    {
        static int inc = 0;
        name = "WorldPos Node #" + std::to_string(inc++);
    }
    
    ~WorldPosNode() {  }

    inline virtual void render() override
    {
        ImGui::BeginDisabled();
        ImGui::InputFloat3("Camera World Position", camera->getPosition().data, "%.1f");
        ImGui::EndDisabled();
    }

private:
    Renderer::Camera* camera;
};