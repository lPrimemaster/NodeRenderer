#include "window.inl"
#include "window_manager.inl"

void Window::finalRender()
{
    update();

    
    if(tagged_delete)
    {
        WindowManager::KillWindow(this);
        return;
    }


    if(!floating_w)
    {
        ImGui::SetNextWindowSize(windowSize);
        ImGui::SetNextWindowPos(windowPos + ImGui::GetMainViewport()->Pos);
    }

    if(floating_clicked)
    {
        if(floating_w)
        {
            window_flags_last = window_flags;
            window_flags = 0;
        }
        else
        {
            window_flags = window_flags_last;
        }
    }

    ImGui::SetNextWindowCollapsed(collapsed);
    if(closeable)
    {
        collapsed = !ImGui::Begin(window_name, &open, window_flags);
        if(open && !collapsed)
        {
            render();
        }
        ImGui::End();
    }
    else
    {
        
        collapsed = !ImGui::Begin(window_name, nullptr, window_flags);
        if(!collapsed)
        {
            render();
        }
        ImGui::End();
    }
}