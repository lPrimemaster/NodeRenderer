#pragma once
#include <string>
#include <atomic>

struct UpdateData
{
    int v_major, v_minor, v_patch;
    std::string v_string;
    std::string download_url;
    size_t download_size;
};

const bool IsUpdateAvailable(UpdateData* data);
void DownloadClient(const std::string& url, std::atomic<size_t>* current_size);
const bool UpdateClient();
