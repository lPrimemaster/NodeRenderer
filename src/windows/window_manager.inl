#pragma once
#include <vector>
#include "window.inl"
#include "node_window.h"
#include "analytics_window.h"
#include "options_window.h"
#include "update_window.h"

class WindowManager
{
public:
    void renderAll() const
    {
        for(auto w : windows)
        {
            w->finalRender();
        }
    }
    static WindowManager& Instance()
    {
        static WindowManager _wmaganer;
        return _wmaganer;
    }

    static NodeWindow* GetNodeWindow()
    {
        return dynamic_cast<NodeWindow*>(Instance().windows[0]);
    }

    static AnalyticsWindow* GetAnalyticsWindow()
    {
        return dynamic_cast<AnalyticsWindow*>(Instance().windows[1]);
    }

    static OptionsWindow* GetOptionsWindow()
    {
        return dynamic_cast<OptionsWindow*>(Instance().windows[2]);
    }

    static UpdateCheckWindow* GetUpdateCheckWindow()
    {
        return dynamic_cast<UpdateCheckWindow*>(Instance().windows[3]);
    }

    // FIXME: This won't work if the window is not the last one, since the indices will change when it is popped from the vector
    static void KillWindow(const Window* window)
    {
        WindowManager& instance = Instance();
        for(int i = 0; i < instance.windows.size(); i++)
        {
            if(window == instance.windows[i])
            {
                delete instance.windows[i];
                // instance.windows[i] = nullptr;
                instance.windows.erase(instance.windows.begin() + i);
            }
        }
    }

private:
    WindowManager()
    {
        NodeWindow* nodeEditor = new NodeWindow("Node Editor");
        OptionsWindow* optionsWindow = new OptionsWindow("Options", nodeEditor);
        windows.push_back(nodeEditor);
        windows.push_back(new AnalyticsWindow("Analytics", nodeEditor, optionsWindow));
        windows.push_back(optionsWindow);
        windows.push_back(new UpdateCheckWindow("Update"));
    }

    ~WindowManager()
    {
        for(auto w : windows)
        {
            delete w;
        }
    }

    std::vector<Window*> windows;
};
