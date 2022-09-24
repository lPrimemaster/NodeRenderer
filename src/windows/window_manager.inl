#pragma once
#include <vector>
#include "window.inl"
#include "node_window.h"
#include "analytics_window.h"
#include "options_window.h"

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

private:
    WindowManager()
    {
        NodeWindow* nodeEditor = new NodeWindow("Node Editor");
        OptionsWindow* optionsWindow = new OptionsWindow("Options", nodeEditor);
        windows.push_back(nodeEditor);
        windows.push_back(new AnalyticsWindow("Analytics", nodeEditor, optionsWindow));
        windows.push_back(optionsWindow);
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
