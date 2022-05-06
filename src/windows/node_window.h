#pragma once
#include <vector>
#include "../../imgui/imgui.h"
#include "window.inl"
#include "../render/nodes/node.h"

class NodeWindow : public Window
{
public:
    NodeWindow(const std::string& name) : Window(name) { open = true; }

    ~NodeWindow()
    {
        for(auto n : nodes)
        {
            delete n;
        }
    }

    static const long long GetApptimeMs();

    inline PropertyNode* getRenderOutputNode()
    {
        return render_output_node;
    }

    inline bool isRenderOutputNodeChanged()
    {
        if(render_output_node_changed)
        {
            render_output_node_changed = false;
            return true;
        }

        return false;
    }

    virtual void render() override;

    inline void deleteNode(int idx)
    {
        PropertyNode* node = *(nodes.data() + idx);
        nodes.erase(nodes.begin() + idx);

        // Check if it is a global render node
        if(node == render_output_node)
        {
            render_output_node = nullptr;
        }

        // Clear the input dependencies of the node links
        for(IOIdxData out_dep : node->output_dependencies)
        {
            for(PropertyNode* other : nodes)
            {
                auto fit = other->inputs.find(out_dep);

                if(fit != other->inputs.end())
                {
                    other->inputs.erase(fit);
                }
            }
        }

        delete node;
    }

private:
    std::vector<PropertyNode*> nodes;

    PropertyNode* render_output_node = nullptr;
    bool render_output_node_changed = false;

    ImVec2 scrolling = ImVec2(0.0f, 0.0f);
    bool show_grid = true;
    int node_selected = -1;
    int last_node_id = 0;

    int drawing_line = 0;
    int link_from_id = 0;
    int link_output_slot = 0;

    static constexpr float  NODE_SLOT_RADIUS = 5.5f;
    static constexpr ImVec2 NODE_WINDOW_PADDING = ImVec2(8.0f, 8.0f);
};
