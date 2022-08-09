#pragma once
#include <vector>
#include "../../imgui/imgui.h"
#include "window.inl"
#include "../render/nodes/node.h"

namespace Renderer
{
    struct DrawList;
}

class NodeWindow : public Window
{
public:
    NodeWindow(const char* name) : Window(name, false) 
    { 
        open = true; 
        window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize; 
    }

    ~NodeWindow()
    {
        for(auto n : nodes)
        {
            delete n;
        }
    }

    // BUG: [*][ERROR]: setValueDynamic(): Received a non valid type.
    //      [*][WARNING]: This node requires an input with types: [...]
    // On Load from file
    inline void deleteAllNodes()
    {
        for(auto n : nodes)
        {
            delete n;
        }
        nodes.clear();
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

    void setDrawActiveList(Renderer::DrawList* dl);

    inline Renderer::DrawList* getDrawActiveList() const
    {
        return activeDL;
    }

    virtual void render() override;

    inline virtual void update() override
    {
        for (int node_idx = 0; node_idx < nodes.size(); node_idx++)
        {
            nodes[node_idx]->update();
        }
    }

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
                    // Erase named input
                    auto named_node_it = other->inputs_named.begin();
                    while(named_node_it != other->inputs_named.end())
                    {
                        if(named_node_it->second == fit->second)
                        {
                            named_node_it = other->inputs_named.erase(named_node_it);
                            break;
                        }
                        else
                        {
                            ++named_node_it;
                        }
                    }

                    // Erase input
                    other->inputs.erase(fit);
                }
            }
        }

        delete node;
    }

    const std::string serializeWindowState();

    void deserializeWindowState(const std::string& state_string);

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

    Renderer::DrawList* activeDL = nullptr;
};
