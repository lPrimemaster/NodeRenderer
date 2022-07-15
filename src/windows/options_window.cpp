#include "options_window.h"
#include "node_window.h"

#include <Windows.h>
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

static constexpr ImVec4 textColor = ImVec4(0.2f, 0.5f, 0.1f, 1.0f);

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

void OptionsWindow::render()
{
    setWindowPos(ImVec2(WIDTH, collapsed_pos_y - ImGui::GetWindowSize().y + 19.0f));
    ImGuiIO& io = ImGui::GetIO();

    if(!io.SetClipboardTextFn) io.SetClipboardTextFn = SetClipboardDataFunc;

    if(ImGui::Button("Save Scene"))
    {
        data64 = nodeWindow->serializeWindowState();

        if(b64buffer) delete[] b64buffer;
        b64buffer = new char[data64.size() + 1];
        memcpy(b64buffer, data64.c_str(), data64.size() + 1);

        ImGui::OpenPopup("Save Data");
    }

    if(ImGui::Button("Load Scene"))
    {
        if(b64buffer) delete[] b64buffer;
        b64buffer = new char[102400];
        b64buffer[0] = '\0';
        ImGui::OpenPopup("Load Data");
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
        ImGui::EndPopup();
    }

    if(ImGui::BeginPopupModal("Load Data", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputTextMultiline("base64", b64buffer, 102400, ImVec2(0, 0));

        if(ImGui::Button("Done"))
        {
            nodeWindow->deserializeWindowState(b64buffer);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

}

void OptionsWindow::update()
{
    if(collapsed)
    {
        setWindowPos(ImVec2(WIDTH, collapsed_pos_y));
    }
}
