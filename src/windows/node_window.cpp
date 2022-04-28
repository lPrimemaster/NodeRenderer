#include "node_window.h"
#include "../log/logger.h"
#include "../render/nodes/nodedef.h"
#include <chrono>

static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

static const std::chrono::steady_clock::time_point app_start = std::chrono::steady_clock::now();

const long long NodeWindow::GetApptimeMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - app_start).count();
}

void NodeWindow::render()
{
    // Initialization
    ImGuiIO& io = ImGui::GetIO();

    // Draw a list of nodes on the left side
    bool open_context_menu = false;
    int node_hovered_in_list = -1;
    int node_hovered_in_scene = -1;
    ImGui::BeginChild("node_list", ImVec2(100, 0));
    ImGui::Text("Nodes");
    ImGui::Separator();
    for (int node_idx = 0; node_idx < nodes.size(); node_idx++)
    {
        PropertyNode* node = nodes[node_idx];
        ImGui::PushID(node->id);
        if (ImGui::Selectable(node->name.c_str(), node_idx == node_selected))
            node_selected = node_idx;
        if (ImGui::IsItemHovered())
        {
            node_hovered_in_list = node_idx;
            open_context_menu |= ImGui::IsMouseClicked(1);
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginGroup();

    // Create our child canvas
    ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x, scrolling.y);
    ImGui::SameLine(0.01f, 400.0f);
    ImGui::Checkbox("Show grid", &show_grid);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(60, 60, 70, 200));
    ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    ImGui::PopStyleVar(); // WindowPadding
    ImGui::PushItemWidth(120.0f);

    const ImVec2 offset = ImGui::GetCursorScreenPos() + scrolling;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Display grid
    if (show_grid)
    {
        ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
        float GRID_SZ = 64.0f;
        ImVec2 win_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_sz = ImGui::GetWindowSize();
        for (float x = fmodf(scrolling.x, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
            draw_list->AddLine(ImVec2(x, 0.0f) + win_pos, ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
        for (float y = fmodf(scrolling.y, GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
            draw_list->AddLine(ImVec2(0.0f, y) + win_pos, ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
    }

    // Additional variables
    bool open_rename = false;

    if(!nodes.empty())
    {
        // Display links
        draw_list->ChannelsSplit(2);
        draw_list->ChannelsSetCurrent(0); // Background
        for (int node_idx = 0; node_idx < nodes.size(); node_idx++)
        {
            PropertyNode* self = nodes[node_idx];
            for(auto i : nodes[node_idx]->inputs)
            {
                PropertyNode* other = i.second;

                ImVec2 p1 = offset + other->getOutputPos(i.first.other_idx);
                ImVec2 p2 = offset + self->getInputPos(i.first.self_idx);
                draw_list->AddBezierCurve(p1, p1 + ImVec2(+50, 0), p2 + ImVec2(-50, 0), p2, IM_COL32(200, 200, 100, 255), 3.0f);
            }
        }

        // Display nodes
        if(drawing_line > 0) drawing_line--;
        for (int node_idx = 0; node_idx < nodes.size(); node_idx++)
        {
            PropertyNode* node = nodes[node_idx];
            ImGui::PushID(node->id);
            ImVec2 node_rect_min = offset + node->_render_data.pos;

            // Display node contents first
            draw_list->ChannelsSetCurrent(1); // Foreground
            bool old_any_active = ImGui::IsAnyItemActive();
            ImVec2 intext_pad = ImVec2(node->_input_max_pad_px, 0);
            ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING + intext_pad);
            ImGui::BeginGroup(); // Lock horizontal position
            ImGui::Text("%s", node->name.c_str());
            // ImGui::Text("_________");
            node->render();
            // ImGui::SliderFloat("##value", &node->Value, 0.0f, 1.0f, "Alpha %.2f");
            // ImGui::ColorEdit3("##color", &node->Color.x);
            ImGui::EndGroup();

            // Save the size of what we have emitted and whether any of the widgets are being used
            bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
            node->_render_data.size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING + intext_pad;
            ImVec2 node_rect_max = node_rect_min + node->_render_data.size;

            // Display node box
            bool node_move = false;
            draw_list->ChannelsSetCurrent(0); // Background
            ImGui::SetCursorScreenPos(node_rect_min);
            ImGui::InvisibleButton("node", node->_render_data.size);
            if (ImGui::IsItemHovered())
            {
                node_hovered_in_scene = node_idx;
                open_context_menu |= ImGui::IsMouseClicked(1);
            }

            
            // Handle node keybinds
            if(ImGui::IsItemFocused())
            {
                // rename
                if(ImGui::IsKeyPressed(ImGuiKey_F2))
                {
                    open_rename = true;
                }
                if(ImGui::IsKeyPressed(ImGuiKey_Delete))
                {
                    deleteNode(node_selected);
                    ImGui::PopID();
                    break;
                }
            }
            
            bool node_active = ImGui::IsItemActive();
            if (node_widgets_active || node_active)
                node_selected = node_idx;
            if (node_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                node_move = true;

            ImU32 node_bg_color = (node_hovered_in_list == node_idx || node_hovered_in_scene == node_idx || (node_hovered_in_list == -1 && node_selected == node_idx)) ? IM_COL32(75, 75, 75, 255) : IM_COL32(60, 60, 60, 255);
            draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
            draw_list->AddRect(node_rect_min, node_rect_max, IM_COL32(100, 100, 100, 255), 4.0f);

            for (int slot_idx = 0; slot_idx < node->_input_count; slot_idx++)
            {
                ImVec2 offset_in = offset + node->getInputPos(slot_idx);
                draw_list->AddText(offset_in + ImVec2(NODE_SLOT_RADIUS + 5, -ImGui::GetTextLineHeight() / 2), IM_COL32(255, 255, 255, 255), node->_input_labels[slot_idx].c_str());
                draw_list->AddCircleFilled(offset_in, NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));
                ImGui::SetCursorScreenPos(offset_in - ImVec2(NODE_SLOT_RADIUS, NODE_SLOT_RADIUS));
                ImGui::PushID(node->id + slot_idx + 1);
                ImGui::InvisibleButton("", ImVec2(NODE_SLOT_RADIUS*2, NODE_SLOT_RADIUS*2));

                if(ImGui::IsItemHovered())
                {
                    if(drawing_line > 0 && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    {
                        drawing_line = 0;
                        // TODO: Prevent link with self
                        if(link_from_id == node_idx)
                        {
                            L_ERROR("Unable to link node with itself.");
                        }

                        L_DEBUG("Make a link (%d) :-:=:-: From (%s:%d) to (%s:%d)",
                            drawing_line,
                            nodes[link_from_id]->name.c_str(),
                            link_from_id,
                            node->name.c_str(),
                            node_idx
                        );

                        IOIdxData idxd;
                        idxd.other_idx = link_output_slot;
                        idxd.self_idx = slot_idx;
                        node->inputs.emplace(idxd, nodes[link_from_id]);
                        nodes[link_from_id]->output_dependencies.push_back(idxd);
                    }
                }

                ImGui::PopID();
            }

            for (int slot_idx = 0; slot_idx < node->_output_count; slot_idx++)
            {
                ImVec2 offset_out = offset + node->getOutputPos(slot_idx);
                draw_list->AddCircleFilled(offset_out, NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));
                ImGui::SetCursorScreenPos(offset_out - ImVec2(NODE_SLOT_RADIUS, NODE_SLOT_RADIUS));
                ImGui::PushID(node->id + node->_input_count + slot_idx + 1);
                ImGui::InvisibleButton("", ImVec2(NODE_SLOT_RADIUS*2, NODE_SLOT_RADIUS*2));

                if(ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    // Draw line
                    drawing_line = 50;
                    ImVec2 p1 = offset_out;
                    ImVec2 p2 = io.MousePos;
                    draw_list->AddBezierCurve(p1, p1 + ImVec2(+50, 0), p2 + ImVec2(-50, 0), p2, IM_COL32(200, 200, 100, 255), 3.0f);
                    link_from_id = node_idx;
                    link_output_slot = slot_idx;
                }

                ImGui::PopID();
            }

            if(node_move && drawing_line == 0)
            {
                node->_render_data.pos = node->_render_data.pos + io.MouseDelta;
            }

            ImGui::PopID();
        }
        draw_list->ChannelsMerge();
    }

    // Open context menu
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) || !ImGui::IsAnyItemHovered())
        {
            node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
            open_context_menu = true;
        }
    if (open_context_menu)
    {
        ImGui::OpenPopup("context_menu");
        if (node_hovered_in_list != -1)
            node_selected = node_hovered_in_list;
        if (node_hovered_in_scene != -1)
            node_selected = node_hovered_in_scene;
    }

    // Draw context menu
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (ImGui::BeginPopup("context_menu"))
    {
        PropertyNode* node = node_selected != -1 ? nodes[node_selected] : NULL;
        ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
        if (node)
        {
            ImGui::Text("Node '%s'", node->name.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Rename...", NULL, false, true))
            {
                open_rename = true;
            }
            if (ImGui::MenuItem("Delete", NULL, false, true))
            {
                // Tracking and cleaning up dependants
                deleteNode(node_selected);

            }
            if (ImGui::MenuItem("Copy", NULL, false, false)) {}
        }
        else
        {
            if (ImGui::BeginMenu("Add..."))
            {
                PropertyNode* newNode = nullptr;

                if (ImGui::MenuItem("Value Node"))
                {
                    newNode = new ValueNode(1);
                }
                if (ImGui::MenuItem("Vector Node"))
                {
                    newNode = new VectorNode(0, 0, 0, 0);
                }
                if (ImGui::MenuItem("Color Node"))
                {
                    newNode = new ColorNode(1, 1, 1, 1);
                }
                if (ImGui::MenuItem("Math Node"))
                {
                    newNode = new MathNode();
                }
                if (ImGui::MenuItem("Function Node"))
                {
                    newNode = new FunctionNode();
                }
                if (ImGui::MenuItem("Time Node"))
                {
                    newNode = new TimeNode();
                }

                if(newNode)
                {
                    newNode->id = last_node_id;
                    newNode->_render_data.pos = scene_pos;
                    last_node_id += (newNode->_input_count + newNode->_output_count + 1);
                    nodes.push_back(newNode);
                }

                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Paste", NULL, false, false)) {}
        }
        ImGui::EndPopup();
    }

    static char localname[128];
    static bool focus_once = true;
    if(open_rename)
    {
        ImGui::OpenPopup("rename_popup");
        open_rename = false;
        focus_once = true;
        PropertyNode* node = node_selected != -1 ? nodes[node_selected] : NULL;

        if(node)
            memcpy(localname, node->name.c_str(), node->name.size() + 1);
    }
    if (ImGui::BeginPopup("rename_popup"))
    {
        PropertyNode* node = node_selected != -1 ? nodes[node_selected] : NULL;

        if(focus_once)
        {
            ImGui::SetKeyboardFocusHere();
            focus_once = false;
        }
        ImGui::InputText("", localname, 128);
        ImGui::SameLine();

        if(ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
        }

        if(ImGui::Button("OK") || ImGui::IsKeyPressed(ImGuiKey_Enter))
        {
            if(strlen(localname) > 0)
            {
                node->name = localname;
            }
            else
            {
                L_ERROR("Node can't have an empty name.");
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();

    // Scrolling
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        scrolling = scrolling + io.MouseDelta;

    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::EndGroup();
}
