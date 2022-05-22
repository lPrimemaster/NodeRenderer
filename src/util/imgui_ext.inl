#pragma once
#include "../imgui/imgui.h"
#include <filesystem>

namespace ImGuiExt
{
    inline bool FileBrowser(std::string* out_path)
    {
        if(ImGui::Button("Select Mesh File"))
        {
            ImGui::OpenPopup("File Browser");
        }
        if(ImGui::BeginPopup("File Browser"))
        {
            static std::filesystem::path curr_path = std::filesystem::current_path();
            static std::filesystem::path curr_selected = curr_path;
            static char curr_drive = 'C';
            static bool validLoadPath = false;
            bool toLoad = false;

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

            memcpy(edit_path, curr_path.string().c_str(), curr_path.string().size() + 1);
            
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

                    if(isdir || dir_entry.path().extension().string() == ".obj")
                    {
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
                            else // Select the item if it is an .obj
                            {
                                if(out_path != nullptr)
                                {
                                    validLoadPath = true;
                                    toLoad = true;
                                    *out_path = std::filesystem::absolute(dir_entry.path()).string();
                                    ImGui::CloseCurrentPopup();
                                }
                                else
                                {
                                    L_ERROR("filebrowser: returned a path for loading but parameter \'out_path\' in nullptr.");
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
                                L_ERROR("filebrowser: returned a path for loading but parameter \'out_path\' in nullptr.");
                            }
                        }
                    }
                }
                ImGui::EndChild();
            }

            if(ImGui::Button("Load") && validLoadPath)
            {
                if(out_path != nullptr)
                {
                    toLoad = true;
                    *out_path = std::filesystem::absolute(curr_selected).string();
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    L_ERROR("filebrowser: returned a path for loading but parameter \'out_path\' in nullptr.");
                }
            }
            ImGui::EndPopup();
            
            return toLoad;
        }
        return false;
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
}
