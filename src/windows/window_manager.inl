#pragma once
#include <vector>
#include "window.inl"
#include "node_window.h"

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

private:
    WindowManager() = default;
    ~WindowManager()
    {
        for(auto w : windows)
        {
            delete w;
        }
    }

    const std::vector<Window*> windows = {
        new NodeWindow("Node Editor")
    };
};
