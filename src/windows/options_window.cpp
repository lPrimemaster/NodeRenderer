#include "options_window.h"
#include "node_window.h"
#include "../util/imgui_ext.inl"

#include <Windows.h>
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../../stb/stb_image.h"

#include "version.h"

static void SetClipboardDataFunc(void* user_data, const char* text)
{
    const size_t len = strlen(text) + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(hMem), text, len);
    GlobalUnlock(hMem);
    OpenClipboard(glfwGetWin32Window(glfwGetCurrentContext()));
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

void OptionsWindow::loadIcons(const std::initializer_list<const int> resources_id)
{
    HDC hMemDC = CreateCompatibleDC(NULL);
    HMODULE hModule = GetModuleHandle(NULL);

    for(auto rid : resources_id)
    {
        HRSRC res = FindResource(hModule, MAKEINTRESOURCE(rid), RT_RCDATA);
        if(res == NULL)
        {
            L_ERROR("FindResource() error!");
            L_ERROR("WAPI error code: %lu", GetLastError());
        }

        HGLOBAL hMemory = LoadResource(hModule, res);
        if(hMemory == NULL)
        {
            L_ERROR("LoadResource() error!");
            L_ERROR("WAPI error code: %lu", GetLastError());
        }

        DWORD dwSize = SizeofResource(hModule, res);
        LPVOID lpAddress = LockResource(hMemory);
        int w, h, d;
        unsigned char* data = stbi_load_from_memory((unsigned char*)lpAddress, dwSize, &w, &h, &d, 4);
        FreeResource(hMemory);

        GLuint image_texture;
        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        icons_id[rid] = image_texture;
        stbi_image_free(data);
    }

    DeleteObject(hMemDC);
}

void OptionsWindow::render()
{
    bool bar_open_last = bar_open;
    bar_open = ImGui::IsWindowHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    // HACK: This is nasty (handling node editor's window state here)...
    if((bar_open_last ^ bar_open) && !nodeWindow->isFloating())
    {
        ImVec2 newsize = nodeWindow->getWindowSize();
        if(bar_open) newsize.x -= (OPENED_WIDTH - COLLAPSED_WIDTH);
        else newsize.x += (OPENED_WIDTH - COLLAPSED_WIDTH);
        nodeWindow->setWindowSize(newsize);
        nodeWindow->setWindowPos(ImVec2(getBarWidth(), 0));

        ImGuiIO& io = ImGui::GetIO();
        if(!io.SetClipboardTextFn) io.SetClipboardTextFn = SetClipboardDataFunc;
    }

    setWindowPos(ImVec2(0, 0));
    setWindowSize(ImVec2(bar_open ? OPENED_WIDTH : COLLAPSED_WIDTH, ImGui::GetMainViewport()->Size.y));

    // Internal bar render
    if(bar_open)
    {
        // HACK: This padding might not be okie tokie for a different style
        const float text_padding = 25.0f;
        bool new_file = ImGui::ImageButton("new_file", (void*)(intptr_t)icons_id[IDI_ICON_NEWFILE], ImVec2(64, 64));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(0, text_padding));
        ImGui::Text("New file...");
        ImGui::EndGroup();

        bool load_file = ImGui::ImageButton("load_file", (void*)(intptr_t)icons_id[IDI_ICON_LOADFILE], ImVec2(64, 64));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(0, text_padding));
        ImGui::Text("Load file...");
        ImGui::EndGroup();

        bool save_file = ImGui::ImageButton("save_file", (void*)(intptr_t)icons_id[IDI_ICON_SAVEFILE], ImVec2(64, 64));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(0, text_padding));
        ImGui::Text("Save file...");
        ImGui::EndGroup();

        // Draw end text
        static const char* end_text0 = "Node Renderer";
        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ImGui::CalcTextSize(end_text0).x) / 2, ImGui::GetWindowSize().y - ImGui::GetTextLineHeightWithSpacing() * 3));
        ImGui::Text(end_text0);
        static const char* end_text1 = "v" NodeRenderer_VERSION_FULL;
        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ImGui::CalcTextSize(end_text1).x) / 2, ImGui::GetWindowSize().y - ImGui::GetTextLineHeightWithSpacing() * 2));
        ImGui::Text(end_text1);

        if(new_file)
        {
            if(nodeWindow->nodeCount() > 0)
            {
                ImGui::OpenPopup("New File");
            }
            else
            {
                // NOTE
                // Even tho there are no nodes, keep this here since this function might
                // handle something else like the editor's window settings in the future
                nodeWindow->deleteAllNodes();
            }
        }

        if(load_file)
        {
            if(b64buffer) delete[] b64buffer;
            b64buffer = new char[102400]; // FIXME: Might fail or not... ? Sussy ? In fact, this is wrong in so many ways...
            b64buffer[0] = '\0';
            ImGui::OpenPopup("Load Data");
        }

        if(save_file)
        {
            data64 = nodeWindow->serializeWindowState();
            if(b64buffer) delete[] b64buffer;
            b64buffer = new char[data64.size() + 1];
            memcpy(b64buffer, data64.c_str(), data64.size() + 1);

            ImGui::OpenPopup("Save Data");
        }
    }
    else
    {
        // Display a tringle for a menu open clear indication
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        float cx = ImGui::GetWindowSize().x / 2;
        float cy = ImGui::GetWindowSize().y / 2;

        ImVec2 tri_0_rc = ImVec2(2.5f, 0.0f);
        ImVec2 tri_1_rc = ImVec2(-2.5f, -3);
        ImVec2 tri_2_rc = ImVec2(-2.5f,  3);

        ImVec2 tri_c = ImVec2(cx, cy) + ImGui::GetWindowPos();

        draw_list->AddTriangleFilled(
            tri_c + tri_0_rc,
            tri_c + tri_1_rc,
            tri_c + tri_2_rc,
            IM_COL32_WHITE
        );
    }

    if(ImGui::BeginPopupModal("New File", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("The current scene will be erased!");

        if(ImGui::Button("I understand."))
        {
            nodeWindow->deleteAllNodes();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Take me back!"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if(ImGui::BeginPopupModal("Save Data", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputTextMultiline("base64", b64buffer, data64.size() + 1, ImVec2(0, 0), ImGuiInputTextFlags_ReadOnly);

        if(ImGui::Button("Ok"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Copy"))
        {
            ImGui::SetClipboardText(data64.c_str());
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        std::string where;
        static const std::vector<std::string> ext = { ".b64" };
        if(ImGuiExt::FileBrowser(&where, ext, "Save As...", "Save", true, "save.b64"))
        {
            FILE* f = fopen(where.c_str(), "wb");
            if(f != nullptr)
            {
                fwrite(data64.c_str(), sizeof(char), data64.size(), f);
                fclose(f);
            }
            else
            {
                L_ERROR("OptionsWindow: Could not create save file.");
            }

            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if(ImGui::BeginPopupModal("Load Data", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputTextMultiline("base64", b64buffer, 102400, ImVec2(0, 0));

        if(ImGui::Button("Load"))
        {
            nodeWindow->deserializeWindowState(b64buffer);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        std::string where;
        static const std::vector<std::string> ext = { ".b64" };
        if(ImGuiExt::FileBrowser(&where, ext, "Load File...", "Load"))
        {
            FILE* f = fopen(where.c_str(), "rb");
            if(f != nullptr)
            {
                fseek(f, 0, SEEK_END);
                long fsize = ftell(f);
                fseek(f, 0, SEEK_SET);
                fread(b64buffer, fsize, 1, f);
                fclose(f);
                nodeWindow->deserializeWindowState(b64buffer);
            }
            else
            {
                L_ERROR("OptionsWindow: Could not load save file.");
            }

            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}

void OptionsWindow::update()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(COLLAPSED_WIDTH, 0));
}
