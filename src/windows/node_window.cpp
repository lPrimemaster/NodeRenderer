#include "node_window.h"
#include "../log/logger.h"
#include "../render/nodes/nodedef.h"
#include "../render/renderer.h"
#include "../util/base64.h"
#include "../util/misc.inl"
#include <chrono>

// TODO: Drag rectangle and clipboard select from nodes and links 
//       (we could use serialization internally since it is already implemented, or we can copy the node data directly)
// TODO: Blueprint mode
// TODO: Blueprint window

static const std::chrono::steady_clock::time_point app_start = std::chrono::steady_clock::now();

const long long NodeWindow::GetApptimeMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - app_start).count();
}

void NodeWindow::setDrawActiveList(Renderer::DrawList* dl)
{
    activeDL = dl;
}

void NodeWindow::render()
{
    // Initialization
    ImGuiIO& io = ImGui::GetIO();

    // Draw a list of nodes on the left side
    static bool window_mode_large = false;
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
    ImGui::SameLine(500.0f);
    bool window_size_changed = ImGui::Button(window_mode_large ? "Retract Window" : "Expand Window");
    ImGui::SameLine(610.0f);
    floating_clicked = ImGui::Button(floating_w ? "Dock Window" : "Float Window");
    floating_w ^= floating_clicked;
    ImGui::SameLine(720.0f);
    ImGui::Text("Mouse over inputs/outputs for additional details.");
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(60, 60, 70, 200));
    ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    ImGui::PopStyleVar(); // WindowPadding
    ImGui::PushItemWidth(120.0f);

    // FIXME: ???
    if(window_size_changed)
    {
        window_mode_large = !window_mode_large;
        ImVec2 original_size = getWindowSize(); // Window start y size (assuming some stuff from other cu's here)
        if(window_mode_large) original_size.y *= 3.0f;
        else                  original_size.y /= 3.0f;
        L_TRACE("New Window Size Y: %.2f", original_size.y);
        setWindowSize(original_size);
    }

    const ImVec2 offset = ImGui::GetCursorScreenPos() + scrolling;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    static constexpr float WINDOW_DRAG_ZONE = 10.0f;

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
    bool moving_node_or_making_link = false;
    bool input_hovered = false;
    bool output_hovered = false;
    struct TooltipDisplay
    {
        bool enabled = false;
        int slot_idx = 0;
        PropertyNode* node = nullptr;
        ImVec2 pos = ImVec2(0, 0);
    } static tooltip_display;

    tooltip_display.enabled = false;

    if(!nodes.empty())
    {
        // Display links
        draw_list->ChannelsSplit(4);
        draw_list->ChannelsSetCurrent(0); // Background
        for (int node_idx = 0; node_idx < nodes.size(); node_idx++)
        {
            PropertyNode* self = nodes[node_idx];
            for(auto i : nodes[node_idx]->inputs)
            {
                PropertyNode* other = i.second->_data_holder_instance;

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
            ImVec2 outtext_pad = ImVec2(node->_output_max_pad_px, 0);
            node->_render_data.size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING + intext_pad + outtext_pad;
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
                        
                        if(link_from_id == node_idx)
                        {
                            L_ERROR("Unable to link node with itself.");
                        }
                        else
                        {
                            L_DEBUG("========== New Node Link ==========");
                            L_DEBUG("From \"%s\" (%d)",
                                nodes[link_from_id]->name.c_str(),
                                link_output_slot
                            );
                            L_DEBUG("To \"%s\" (%d)",
                                node->name.c_str(),
                                slot_idx
                            );
                            L_DEBUG("===================================");

                            IOIdxData idxd;
                            idxd.other_idx = link_output_slot;
                            idxd.self_idx = slot_idx;
                            idxd.self_id = node_idx;
                            node->inputs.emplace(idxd, nodes[link_from_id]->outputs[link_output_slot]);
                            node->inputs_named.emplace(node->_input_labels[slot_idx], nodes[link_from_id]->outputs[link_output_slot]);
                            nodes[link_from_id]->output_dependencies.push_back(idxd);

                            node->onConnection(node->_input_labels[slot_idx]);
                        }
                    }
                }

                if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                {
                    tooltip_display.enabled = true;
                    tooltip_display.slot_idx = slot_idx;
                    tooltip_display.node = node;
                    // tooltip_display.pos = offset_in;
                    tooltip_display.pos = io.MousePos + ImVec2(15, 15);

                    // Render help tooltip
                    ImVec2 tooltip_rect_min = tooltip_display.pos;

                    draw_list->ChannelsSetCurrent(3);
                    ImGui::SetCursorScreenPos(tooltip_rect_min + NODE_WINDOW_PADDING);
                    ImGui::BeginGroup();
                    
                    // Need to copy the string here in the param
                    // TODO: Justify text as well
                    auto wrapTextManual = [](std::string str, int nth_char = 50) -> std::string {
                        int n = 0;
                        while(n + nth_char < str.size()) // Just to make sure string is not zero sized
                        {
                            n = str.rfind(' ', n + nth_char);
                            if(n != std::string::npos)
                            {
                                str[n] = '\n';
                            }
                            else break;
                        }
                        return str;
                    };
                    
                    std::string input_name = node->_input_labels[slot_idx];
                    input_name[0] = std::toupper(input_name[0]);

                    ImGui::Text("%s [%s]", input_name.c_str(), node->name.c_str());
                    ImVec2 line_draw_pos = ImGui::GetCursorScreenPos();
                    line_draw_pos.y += 1.0f;
                    ImGui::Spacing();
                    float line_text_offset = ImGui::GetTextLineHeightWithSpacing() / 2.2f;

                    // Build text description
                    std::string description = wrapTextManual(node->inputs_description[node->_input_labels[slot_idx]]);
                    ImGui::Text(description.c_str());
                    ImGui::Spacing();
                    ImGui::Text("Valid input types:");
                    int idx = 0;
                    for(auto type_name : *node->allowed_inputs_type_name[node->_input_labels[slot_idx]])
                    {
                        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
                        ImGui::Text(("\t" + type_name + "\n").c_str());
                        draw_list->AddCircleFilled(cursor_pos + ImVec2(20, line_text_offset), 2.0f, IM_COL32_WHITE);
                    }

                    ImGui::EndGroup();

                    // Handmade separator
                    draw_list->AddLine(line_draw_pos, line_draw_pos + ImVec2(ImGui::GetItemRectSize().x, 0), IM_COL32(255, 255, 255, 127));
                    ImVec2 size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
                    ImVec2 tooltip_rect_max = tooltip_rect_min + size;

                    // Same style as the nodes
                    draw_list->ChannelsSetCurrent(2);
                    ImU32 tooltip_bg_color = IM_COL32(60, 60, 60, 255);
                    ImGui::SetCursorScreenPos(tooltip_rect_min);
                    draw_list->AddRectFilled(tooltip_rect_min, tooltip_rect_max, tooltip_bg_color, 4.0f);
                    draw_list->AddRect(tooltip_rect_min, tooltip_rect_max, IM_COL32(100, 100, 100, 255), 4.0f);

                    draw_list->ChannelsSetCurrent(0);
                }

                ImGui::PopID();
            }

            for (int slot_idx = 0; slot_idx < node->_output_count; slot_idx++)
            {
                ImVec2 offset_out = offset + node->getOutputPos(slot_idx);
                if(node->_output_labels.size() == node->_output_count) // Draw output text only if it exists
                    draw_list->AddText(offset_out + ImVec2(7.5f - node->_output_max_pad_px, -ImGui::GetTextLineHeight() / 2), IM_COL32(255, 255, 255, 255), node->_output_labels[slot_idx].c_str());
                draw_list->AddCircleFilled(offset_out, NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));
                ImGui::SetCursorScreenPos(offset_out - ImVec2(NODE_SLOT_RADIUS, NODE_SLOT_RADIUS));
                ImGui::PushID(node->id + node->_input_count + slot_idx + 1);
                ImGui::InvisibleButton("", ImVec2(NODE_SLOT_RADIUS*2, NODE_SLOT_RADIUS*2));

                if(ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    moving_node_or_making_link = true;

                    // Draw line
                    drawing_line = 50;
                    ImVec2 p1 = offset_out;
                    ImVec2 p2 = io.MousePos;
                    draw_list->AddBezierCurve(p1, p1 + ImVec2(+50, 0), p2 + ImVec2(-50, 0), p2, IM_COL32(200, 200, 100, 255), 3.0f);
                    link_from_id = node_idx;
                    link_output_slot = slot_idx;

                    ImVec2 windowSize = ImGui::GetWindowSize();
                    ImVec2 windowPos  = ImGui::GetWindowPos();

                    int scroll_x = (windowPos.x + WINDOW_DRAG_ZONE > io.MousePos.x) ? 10 : 
                        ((windowPos.x + windowSize.x - WINDOW_DRAG_ZONE < io.MousePos.x) ? -10 : 0);
                    int scroll_y = (windowPos.y + WINDOW_DRAG_ZONE > io.MousePos.y) ? 10 : 
                        ((windowPos.y + windowSize.y - WINDOW_DRAG_ZONE < io.MousePos.y) ? -10 : 0);

                    scrolling = scrolling + ImVec2(scroll_x, scroll_y);
                }

                ImGui::PopID();
            }

            if(node_move && drawing_line == 0)
            {
                moving_node_or_making_link = true;
                node->_render_data.pos = node->_render_data.pos + io.MouseDelta;
            }

            ImGui::PopID();
        }

        draw_list->ChannelsMerge();
    }

    // Open context menu
    if(ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsWindowHovered(/* ImGuiHoveredFlags_AllowWhenBlockedByPopup */))
    {
        node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
        open_context_menu = true;
    }
    if(open_context_menu)
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

                // Menu'ed nodes
                if(ImGui::BeginMenu("Math"))
                {
                    if (ImGui::MenuItem("Value Node"))
                    {
                        newNode = new ValueNode(1);
                    }
                    // if (ImGui::MenuItem("Color Node"))
                    // {
                    //     newNode = new ColorNode();
                    // }
                    if (ImGui::MenuItem("Math Node"))
                    {
                        newNode = new MathNode();
                    }
                    if (ImGui::MenuItem("Function Node"))
                    {
                        newNode = new FunctionNode();
                    }
                    ImGui::EndMenu();
                }
                if(ImGui::BeginMenu("List"))
                {
                    if (ImGui::MenuItem("List Node"))
                    {
                        newNode = new ListNode();
                    }
                    if (ImGui::MenuItem("List Access Node"))
                    {
                        newNode = new ListAccessNode();
                    }
                    if (ImGui::MenuItem("List Join Node"))
                    {
                        newNode = new ListJoinNode();
                    }
                    ImGui::EndMenu();
                }
                if(ImGui::BeginMenu("Mesh"))
                {
                    if (ImGui::MenuItem("Mesh Node"))
                    {
                        newNode = new MeshNode();
                    }
                    if (ImGui::MenuItem("Mesh Interpolator Node"))
                    {
                        newNode = new MeshInterpolatorNode();
                    }
                    ImGui::EndMenu();
                }
                if(ImGui::BeginMenu("Misc"))
                {
                    if (ImGui::MenuItem("Display Node"))
                    {
                        newNode = new DisplayNode();
                    }
                    if (ImGui::MenuItem("Feedback Node"))
                    {
                        newNode = new FeedbackNode();
                    }
                    ImGui::EndMenu();
                }
                if(ImGui::BeginMenu("Rendering"))
                {
                    if (ImGui::MenuItem("Render Node"))
                    {
                        // TODO: We can allow more than one render node in fact
                        if(render_output_node == nullptr)
                        {
                            newNode = new RenderNode();
                            render_output_node = newNode;
                            render_output_node_changed = true;
                        }
                        else
                        {
                            L_WARNING("There can only be one render node.");
                        }
                    }
                    if (ImGui::MenuItem("Camera Node"))
                    {
                        newNode = new CameraNode(activeDL->camera);
                    }
                    ImGui::EndMenu();
                }

                // Free nodes
                if (ImGui::MenuItem("Time Node"))
                {
                    newNode = new TimeNode();
                }
                // if (ImGui::MenuItem("WorldPos Node"))
                // {
                //     newNode = new WorldPosNode(activeDL->camera);
                // }
                if (ImGui::MenuItem("Path Node"))
                {
                    newNode = new PathNode();
                }
                if (ImGui::MenuItem("Audio Node"))
                {
                    newNode = new AudioNode();
                }

                if(newNode)
                {
                    newNode->id = last_node_id;
                    newNode->_render_data.pos = scene_pos;
                    last_node_id += (newNode->_input_count + newNode->_output_count + 1);

                    switch(newNode->priority)
                    {
                        case PropertyNode::Priority::NORMAL:   nodes.push_back(newNode);             break;
                        // case PropertyNode::Priority::FEEDBACK: nodes.insert(nodes.begin(), newNode); break;
                        case PropertyNode::Priority::FEEDBACK: nodes.push_back(newNode);             break;
                        case PropertyNode::Priority::RENDER:   nodes.push_back(newNode);             break;
                    }

                }

                ImGui::EndMenu();
            }
            // if (ImGui::MenuItem("Paste", NULL, false, false)) {}
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

    // Selection rect
    if(!moving_node_or_making_link)
    {
        static ImVec2 mouse_pos;
        static ImVec2 p0 = ImVec2(0, 0);
        static ImVec2 p1 = ImVec2(0, 0);
        static bool inital_click = false;
        if(ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered(/* ImGuiHoveredFlags_AllowWhenBlockedByPopup */))
        {
            mouse_pos = io.MousePos;
            inital_click = true;
        }
        else if(inital_click && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            // Figure out the min and max of the rectangle to draw based on the mouse points
            p0 = ImVec2(std::min(io.MousePos.x, mouse_pos.x), std::min(io.MousePos.y, mouse_pos.y));
            p1 = ImVec2(std::max(io.MousePos.x, mouse_pos.x), std::max(io.MousePos.y, mouse_pos.y));

            

            draw_list->AddRectFilled(p0, p1, IM_COL32(41, 74, 122, 50));
            draw_list->AddRect(p0, p1, IM_COL32(41, 74, 122, 255), 0.0f, 0, 2.0f);

            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 windowPos  = ImGui::GetWindowPos();

            int scroll_x = (windowPos.x + WINDOW_DRAG_ZONE > io.MousePos.x) ? 10 : 
                ((windowPos.x + windowSize.x - WINDOW_DRAG_ZONE < io.MousePos.x) ? -10 : 0);
            int scroll_y = (windowPos.y + WINDOW_DRAG_ZONE > io.MousePos.y) ? 10 : 
                ((windowPos.y + windowSize.y - WINDOW_DRAG_ZONE < io.MousePos.y) ? -10 : 0);

            scrolling = scrolling + ImVec2(scroll_x, scroll_y);

            for(auto* node : nodes)
            {
                float x0 = offset.x + node->_render_data.pos.x;
                float x1 = x0 + node->_render_data.size.x;
                float y0 = offset.y + node->_render_data.pos.y;
                float y1 = y0 + node->_render_data.size.y;

                if(x0 > p0.x && x0 < p1.x && x1 > p0.x && x1 < p1.x)
                {
                    if(y0 > p0.y && y0 < p1.y && y1 > p0.y && y1 < p1.y)
                    {
                        static const ImColor BLUE_CAPS = IM_COL32(255, 0, 0, 255);
                        static constexpr float OFFSET_CAPS = 3.0f;
                        static constexpr float THICKN_CAPS = 2.0f;
                        static constexpr float LEN_CAPS    = 7.5f;

                        // Four end caps on the selected nodes
                        const ImVec2 left_top_corner[3] = {
                            ImVec2(x0 - OFFSET_CAPS, y0 + LEN_CAPS), 
                            ImVec2(x0 - OFFSET_CAPS, y0 - OFFSET_CAPS), 
                            ImVec2(x0 + LEN_CAPS,    y0 - OFFSET_CAPS)
                        };
                        draw_list->AddPolyline(left_top_corner, 3, BLUE_CAPS, ImDrawFlags_RoundCornersAll, THICKN_CAPS);

                        const ImVec2 left_bot_corner[3] = {
                            ImVec2(x0 - OFFSET_CAPS, y1 - LEN_CAPS), 
                            ImVec2(x0 - OFFSET_CAPS, y1 + OFFSET_CAPS), 
                            ImVec2(x0 + LEN_CAPS,    y1 + OFFSET_CAPS)
                        };
                        draw_list->AddPolyline(left_bot_corner, 3, BLUE_CAPS, ImDrawFlags_RoundCornersAll, THICKN_CAPS);

                        const ImVec2 right_bot_corner[3] = {
                            ImVec2(x1 + OFFSET_CAPS, y1 - LEN_CAPS), 
                            ImVec2(x1 + OFFSET_CAPS, y1 + OFFSET_CAPS), 
                            ImVec2(x1 - LEN_CAPS,    y1 + OFFSET_CAPS)
                        };
                        draw_list->AddPolyline(right_bot_corner, 3, BLUE_CAPS, ImDrawFlags_RoundCornersAll, THICKN_CAPS);

                        const ImVec2 right_top_corner[3] = {
                            ImVec2(x1 + OFFSET_CAPS, y0 + LEN_CAPS), 
                            ImVec2(x1 + OFFSET_CAPS, y0 - OFFSET_CAPS), 
                            ImVec2(x1 - LEN_CAPS,    y0 - OFFSET_CAPS)
                        };
                        draw_list->AddPolyline(right_top_corner, 3, BLUE_CAPS, ImDrawFlags_RoundCornersAll, THICKN_CAPS);
                    }
                }
            }

        }
        else if(ImGui::IsMouseReleased(ImGuiMouseButton_Left) && p1.x - p0.x > 0.5f && p1.y - p0.y > 0.5f /* Prevents zero sized quads */
                && ImGui::IsWindowHovered(/* ImGuiHoveredFlags_AllowWhenBlockedByPopup */))
        {
            inital_click = false;
            window_selection_buffer.selected_nodes.clear();
            L_TRACE("SEL_RECT = min[%.2f,%.2f] max[%.2f,%.2f]", p0.x, p0.y, p1.x, p1.y);
            // Check which nodes are inside the selection
            for(auto* node : nodes)
            {
                float x0 = offset.x + node->_render_data.pos.x;
                float x1 = x0 + node->_render_data.size.x;
                float y0 = offset.y + node->_render_data.pos.y;
                float y1 = y0 + node->_render_data.size.y;

                if(x0 > p0.x && x0 < p1.x && x1 > p0.x && x1 < p1.x)
                {
                    if(y0 > p0.y && y0 < p1.y && y1 > p0.y && y1 < p1.y)
                    {
                        window_selection_buffer.selected_nodes.push_back(node);
                    }
                }
            }
            p0 = ImVec2(0, 0);
            p1 = ImVec2(0, 0);
        }
    }

    ImGui::PopStyleVar();

    // Tooltips
    // if(tooltip_display.enabled)
    // {
    //     // printf("Input types [hovered input]: %s\n", tooltip_display.node->allowed_inputs_type_name[tooltip_display.node->_input_labels[tooltip_display.slot_idx]]->at(0).c_str());
    //     // draw_list->ChannelsSetCurrent(3); // Top

    //     ImVec2 tooltip_rect_min = tooltip_display.pos;

    //     draw_list->ChannelsSplit(2);
    //     draw_list->ChannelsSetCurrent(1);
    //     ImGui::SetCursorScreenPos(tooltip_rect_min + NODE_WINDOW_PADDING);
    //     ImGui::BeginGroup();

    //     ImGui::Text("Tooltip title goes in here.");
    //     ImGui::TextWrapped("A very long tooltip description will probably go in here, telling us what the functionality of something is.");

    //     ImGui::EndGroup();

    //     ImVec2 size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
    //     ImVec2 tooltip_rect_max = tooltip_rect_min + size;

    //     // Same style as the nodes
    //     draw_list->ChannelsSetCurrent(0);
    //     ImU32 tooltip_bg_color = IM_COL32(60, 60, 60, 255);
    //     ImGui::SetCursorScreenPos(tooltip_rect_min);
    //     draw_list->AddRectFilled(tooltip_rect_min, tooltip_rect_max, tooltip_bg_color, 4.0f);
    //     draw_list->AddRect(tooltip_rect_min, tooltip_rect_max, IM_COL32(100, 100, 100, 255), 4.0f);
    //     draw_list->ChannelsMerge();
    // }

    // Scrolling
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        scrolling = scrolling + io.MouseDelta;

    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::EndGroup();
}

// On Save to file
// BUG: [*][ERROR]: getListData(): Received a non valid type.
const std::string NodeWindow::serializeWindowState()
{
    ByteBuffer buffer;

    // Number of nodes
    buffer.add<size_t>(nodes.size());

    // The last node id
    buffer.add(last_node_id);

    // Serialize types for reconstruction
    for(auto n : nodes)
    {
        buffer.add(n->type);
    }

    // Serialize the node data
    for(auto n : nodes)
    {
        buffer.add(n->serialize());
    }

    return base64_encode(buffer.front(), (unsigned int)buffer.size());
}

// On Load from file
void NodeWindow::deserializeWindowState(const std::string& state_string)
{
    std::vector<unsigned char> data = base64_decode(state_string);
    ByteBuffer buffer;
    buffer.addRawData(data.data(), data.size());

    // Get the number of nodes
    size_t n_count = 0;
    buffer.get(&n_count);

    // Get the last node id
    buffer.get(&last_node_id);

    std::vector<PropertyNode*> local_nodes;

    // Delete all the old nodes if we have any
    deleteAllNodes();

    // Get all of the node types
    for(size_t i = 0; i < n_count; i++)
    {
        PropertyNode::Type t;
        buffer.get(&t);

        PropertyNode* newNode = nullptr;
        switch (t)
        {
            case PropertyNode::Type::VALUE: newNode = new ValueNode(); break;
            case PropertyNode::Type::COLOR: newNode = new ColorNode(); break;
            case PropertyNode::Type::MATH: newNode = new MathNode(); break;
            case PropertyNode::Type::FUNCTION: newNode = new FunctionNode(); break;
            case PropertyNode::Type::TIME: newNode = new TimeNode(); break;
            case PropertyNode::Type::WORLDPOS: newNode = new WorldPosNode(activeDL->camera); break;
            case PropertyNode::Type::RENDER: // Render node needs to have these details
            { 
                newNode = new RenderNode();
                render_output_node = newNode;
                render_output_node_changed = true;
            } break;
            case PropertyNode::Type::LIST: newNode = new ListNode(); break;
            case PropertyNode::Type::LISTACCESS: newNode = new ListAccessNode(); break;
            case PropertyNode::Type::LISTJOIN: newNode = new ListJoinNode(); break;
            case PropertyNode::Type::MESH: newNode = new MeshNode(); break;
            case PropertyNode::Type::PATH: newNode = new PathNode(); break;
            case PropertyNode::Type::CAMERA: newNode = new CameraNode(activeDL->camera); break;
            case PropertyNode::Type::AUDIO: newNode = new AudioNode(); break;
            case PropertyNode::Type::DISPLAY: newNode = new DisplayNode(); break;
            case PropertyNode::Type::FEEDBACK: newNode = new FeedbackNode(); break;
            case PropertyNode::Type::TEST: newNode = new TestNode(); break;
            case PropertyNode::Type::MESHINTERP: newNode = new MeshInterpolatorNode(); break;
            default: L_ERROR("Node Window deserialization encountered an invalid node type."); break;
        }

        if(newNode)
        {
            local_nodes.push_back(newNode);
        }
    }

    // Deserialize the node itself
    for(auto node : local_nodes)
    {
        // This should update the node itself, preventing empty type connections
        node->deserialize(buffer);
    }

    // Push the nodes to the window
    for(auto node : local_nodes)
    {
        switch(node->priority)
        {
            case PropertyNode::Priority::NORMAL:   [[fallthrough]];
            case PropertyNode::Priority::RENDER:   nodes.push_back(node);             break;
            // case PropertyNode::Priority::FEEDBACK: nodes.insert(nodes.begin(), node); break;
            case PropertyNode::Priority::FEEDBACK: nodes.push_back(node);             break;
        }
    }

    // Link the nodes
    // BUG: Fix all this!!!!!
    for(auto node : local_nodes)
    {
        for(IOIdxData idxd : node->output_dependencies)
        {
            unsigned char link_output_slot = idxd.other_idx;
            unsigned char slot_idx = idxd.self_idx;
            int id = idxd.self_id;

            if(id >= nodes.size())
            {
                L_ERROR("Attempted connecting node \"%s\" to an invalid node id %d(out of %d).", node->name.c_str(), id, nodes.size());
                L_WARNING("Ignoring node linking for this instance...");
                L_ERROR("This error is either a bug, or the savefile is corrupted.");
                continue;
            }

            if(link_output_slot >= node->outputs.size())
            {
                L_ERROR("Attempted connecting node \"%s\" with an invalid output slot %d(out of %d).", 
                    node->name.c_str(),
                    link_output_slot,
                    node->outputs.size()
                );
                L_WARNING("Ignoring node linking for this instance...");
                L_ERROR("This error is either a bug, or the savefile is corrupted.");
                continue;
            }

            auto other = nodes[id];

            if(slot_idx >= other->_input_labels.size())
            {
                L_ERROR("Attempted connecting node \"%s\" to node \"%s\" with an invalid input slot %d(out of %d).", 
                    node->name.c_str(),
                    other->name.c_str(),
                    slot_idx,
                    other->_input_labels.size()
                );
                L_WARNING("Ignoring node linking for this instance...");
                L_ERROR("This error is either a bug, or the savefile is corrupted.");
                continue;
            }

            other->inputs.emplace(idxd, node->outputs[link_output_slot]);
            other->inputs_named.emplace(other->_input_labels[slot_idx], node->outputs[link_output_slot]);
        }
    }

    // Start the save file scene with the node window open
    setWindowCollapsed(false);
}

void NodeWindow::CopyPasteBuffer::copy(const SelectionBuffer& sb)
{

}

void NodeWindow::CopyPasteBuffer::paste()
{
    
}