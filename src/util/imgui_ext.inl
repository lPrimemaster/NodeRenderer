#pragma once
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h" // For ImTextStrToUtf8
#include <filesystem>

namespace ImGuiExt
{
    inline bool FileBrowser(std::string* out_path, std::vector<std::string> exts, const std::string& button_name = "Select File", const std::string& dialog_name = "Load", bool createFile = false, const std::string& default_filename = "filename")
    {
        static std::filesystem::path curr_path = std::filesystem::current_path();
        static std::filesystem::path curr_selected = curr_path;
        static char curr_drive = 'C';
        static bool validLoadPath = false;
        static bool filename_may_change = true;
        bool toLoad = false;
        bool openOWModal = false;
        static std::filesystem::path path_out;

        if(ImGui::Button(button_name.c_str()))
        {
            ImGui::OpenPopup("File Browser");
        }
        if(ImGui::BeginPopup("File Browser"))
        {
            char drive_str[] = { curr_drive, ':', '\\', '\0' };
            ImGui::PushItemWidth(4 * ImGui::GetFontSize());
            if(ImGui::BeginCombo("##drive_select", drive_str))
            {
                for(int i = 0; i < 26; i++)
                {
                    char drive_name = 'A' + i;
                    char path_str[] = { drive_name, ':', '\\', '\0' };
                    if(std::filesystem::is_directory(path_str))
                    {
                        bool selected = curr_drive == drive_name;
                        if(ImGui::Selectable(path_str, selected) && !selected)
                        {
                            curr_drive = drive_name;
                            curr_path = std::filesystem::absolute(path_str);
                        }
                    }
                }

                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            ImGui::SameLine();
            
            char edit_path[512];

            const ImWchar* tstart = (const ImWchar*)curr_path.wstring().c_str();
            const size_t wsize = curr_path.wstring().size();
            ImTextStrToUtf8(edit_path, 512, tstart, tstart + wsize);
            // memcpy(edit_path, curr_path.string().c_str(), curr_path.string().size() + 1);

            ImGui::PushItemWidth(500 - 4 * ImGui::GetFontSize() - 8);
            if(ImGui::InputText("##path_name", edit_path, 512))
            {
                if(std::filesystem::is_directory(edit_path))
                {
                    curr_path = std::filesystem::absolute(edit_path);
                }
                else
                {
                    L_WARNING("filebrowser: input is an invalid path.");
                }
            }
            ImGui::PopItemWidth();
            
            if(ImGui::BeginChild("#browser", ImVec2(500, 150), true))
            {
                bool selected = false;
                if(curr_path.has_parent_path() && curr_path.parent_path() != curr_path)
                {
                    const auto ppath = curr_path.parent_path();
                    selected = std::filesystem::absolute(ppath) == curr_selected;
                    if(ImGui::Selectable("..", &selected, ImGuiSelectableFlags_AllowDoubleClick))
                    {
                        validLoadPath = false;
                        curr_selected = std::filesystem::absolute(ppath);
                    }

                    if(ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                    {
                        ImGui::SetScrollY(0);
                        selected = true;
                        validLoadPath = false;
                        curr_path = std::filesystem::absolute(ppath);
                    }
                }

                for(auto const& dir_entry : std::filesystem::directory_iterator{curr_path})
                {
                    selected = dir_entry.path() == curr_selected;
                    bool isdir = dir_entry.is_directory();
                    char ifdir = isdir ? '/' : ' ';
                    bool isvalidext = false;
                    for(auto ext : exts) isvalidext |= dir_entry.path().extension().string() == ext;
                    if(isdir || isvalidext)
                    {
                        // TODO: Encode this as UTF-8 as well
                        if(ImGui::Selectable(((--dir_entry.path().end())->string() + ifdir).c_str(), &selected, ImGuiSelectableFlags_AllowDoubleClick))
                        {
                            curr_selected = std::filesystem::absolute(dir_entry.path());
                        }

                        if(ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                        {
                            ImGui::SetScrollY(0);
                            if(dir_entry.is_directory())
                            {
                                validLoadPath = false;
                                curr_path = std::filesystem::absolute(dir_entry.path());
                            }
                            else // Select the item if it is an .ext
                            {
                                if(out_path != nullptr)
                                {
                                    if(createFile)
                                    {
                                        path_out = std::filesystem::absolute(dir_entry.path());
                                        L_TRACE("CB: %s", path_out.string().c_str());
                                        openOWModal = true;
                                    }
                                    else
                                    {
                                        validLoadPath = true;
                                        toLoad = true;
                                        *out_path = std::filesystem::absolute(dir_entry.path()).string();
                                        ImGui::CloseCurrentPopup();
                                    }
                                }
                                else
                                {
                                    L_ERROR("filebrowser: returned a path for loading/saving but parameter \'out_path\' in nullptr.");
                                }
                            }
                        }
                        else if(ImGui::IsItemClicked())
                        {
                            if(out_path != nullptr)
                            {
                                validLoadPath = true;
                            }
                            else
                            {
                                L_ERROR("filebrowser: returned a path for loading/saving but parameter \'out_path\' in nullptr.");
                            }
                        }
                    }
                }
                ImGui::EndChild();
            }
            
            static char filename_full[512];
            if(createFile)
            {
                if(filename_may_change)
                {
                    filename_may_change = false;
                    strcpy(filename_full, default_filename.c_str());
                }

                ImGui::InputText("##filename", filename_full, 512);
                ImGui::SameLine();
            }
            
            if(ImGui::Button(dialog_name.c_str()))
            {
                if(out_path != nullptr)
                {
                    if(createFile)
                    {
                        path_out = std::filesystem::absolute(curr_path) / filename_full;

                        if(!path_out.has_extension() || std::find(exts.begin(), exts.end(), path_out.extension().string()) == exts.end())
                        {
                            path_out += *exts.begin();
                        }

                        // TODO: Check if the the file already exists and prompt for overwrite
                        if(std::filesystem::exists(path_out))
                        {
                            openOWModal = true;
                            L_TRACE("SA: %s", path_out.string().c_str());
                        }
                        else
                        {
                            filename_may_change = true;
                            toLoad = true;
                            *out_path = path_out.string();
                            ImGui::CloseCurrentPopup();
                        }
                    }
                    else if(validLoadPath)
                    {
                        toLoad = true;
                        auto path_out = std::filesystem::absolute(curr_selected);
                        *out_path = path_out.string();
                        ImGui::CloseCurrentPopup();
                    }
                }
                else
                {
                    L_ERROR("filebrowser: returned a path for loading/saving but parameter \'out_path\' in nullptr.");
                }
            }

            if(openOWModal)
            {
                ImGui::OpenPopup("Overwrite File");
            }

            // This needs to be here because of ImGui stack state manager (one could use a control variable instead)
            if(ImGui::BeginPopupModal("Overwrite File", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("The file already exists.");
                ImGui::Text("Do you want to overwrite it?");

                if(ImGui::Button("Overwrite"))
                {
                    filename_may_change = true;
                    toLoad = true;
                    *out_path = path_out.string();
                    ImGui::CloseCurrentPopup();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if(ImGui::Button("Cancel"))
                {
                    toLoad = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            ImGui::EndPopup();
        }

        if(toLoad) L_DEBUG("Path sent (possible): %s", out_path->c_str());

        return toLoad;
    }

    // Parameter speed is frames per type of char
    inline void SpinnerText(int frames_per_anim = 10)
    {
        static int frames = 0;
        static int idx = 0;
        static char chars[] = {'-', '\\', '|', '/'};
        ImGui::Text("%c", chars[idx % 4]);
        if(frames % frames_per_anim == 0) idx++;
        frames++;
    }

    inline ImVec2 SliderAutomatic(const float pct, const float width)
    {
        static ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImVec2 p0 = ImGui::GetCursorScreenPos();

        // Pad for button
        p0.x += 5.0f;
        p0.y += 7.0f;
        const ImVec2 p1 = ImVec2(p0.x + width, p0.y);
        const ImVec2 circle = ImVec2(p0.x + width * pct, p0.y);

        
        draw_list->AddLine(p0, circle, IM_COL32(66, 150, 250, 127), 8.0f);
        draw_list->AddLine(circle, p1, IM_COL32(41, 74, 122, 127), 8.0f);

        draw_list->AddCircleFilled(circle, 7.0f, IM_COL32(41, 74, 122, 255));
        draw_list->AddCircle(circle, 7.0f, IM_COL32(255, 255, 255, 255));

        return ImVec2(p1.x + 20.0f, p1.y);
    }
}
