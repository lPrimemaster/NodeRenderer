#pragma once
#include <vector>
#include "../../imgui/imgui.h"
#include "window.inl"
#include "../render/nodes/node.h"
#include "../util/misc.inl"

namespace RasterRenderer
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
    
    inline void deleteAllNodes()
    {
        for(auto n : nodes)
        {
            delete n;
        }
        nodes.clear();
    }

    inline int nodeCount() const
    {
        return (int)nodes.size();
    }

    static const long long GetApptimeMs();

    PropertyNode* createNodeDynamic(const PropertyNode::Type& t);

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

    inline bool isFloating() const
    {
        return floating_w;
    }

    void setDrawActiveList(RasterRenderer::DrawList* dl);

    inline RasterRenderer::DrawList* getDrawActiveList() const
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

    inline int getNodeIndex(PropertyNode* node)
    {
        for(int i = 0; i < (int)nodes.size(); i++)
        {
            if(nodes[i] == node) return i;
        }
        return -1;
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
                            other->onDisconnect(named_node_it->first);
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

    RasterRenderer::DrawList* activeDL = nullptr;

    struct SelectionBuffer
    {
        std::vector<PropertyNode*> selected_nodes;

        inline void invalidate()
        {
            for(auto* node : selected_nodes)
            {
                node->_select_candidate = false;
            }
            selected_nodes.clear();
        }

        inline void clear()
        {
            selected_nodes.clear();
        }
    };

    struct CopyPasteBuffer
    {
       std::vector<PropertyNode*> copies;
       void copy(const SelectionBuffer& sb);
       void paste();
    };

    SelectionBuffer window_selection_buffer;
    CopyPasteBuffer window_copypaste_buffer;
};
