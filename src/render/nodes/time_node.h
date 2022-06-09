#pragma once
#include "node.h"

struct TimeNode final : public PropertyNode
{
    inline TimeNode() : PropertyNode(0, {}, 1, { "t" })
    {
        static int inc = 0;
        name = "Time Node #" + std::to_string(inc++);
    }
    
    ~TimeNode() {  }

    inline virtual void render() override
    {
        ImGui::BeginDisabled();
        ImGui::InputFloat("Time (s)", &s, 0.0f, 0.0f, "%.2f");
        ImGui::EndDisabled();
    }

    inline virtual void update() override
    {
        auto data = outputs[0];
        data->resetDataUpdate();
        s = NodeWindow::GetApptimeMs() / 1000.0f;
        s = std::ceil(s * 100.0f) / 100.0f;
        data->setValue(s);
    }

private:
    float s = 0.0f;
};
