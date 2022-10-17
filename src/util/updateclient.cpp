#include "updateclient.h"
#include <Windows.h>
#include <winhttp.h>
#include "../log/logger.h"
#include "version.h"

#include <string>
#include <sstream>
#include <cassert>

static const std::string GetTextRequestFromUrl(const std::wstring& hostname, const std::wstring& path)
{
    std::string result;

    HINTERNET hSession = WinHttpOpen(
        L"WinHTTP Agent/5.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if(hSession)
    {
        HINTERNET hConnect = WinHttpConnect(hSession, hostname.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);

        if(hConnect)
        {
            LPCWSTR types[] = { L"text/*", NULL };

            HINTERNET hRequest = WinHttpOpenRequest(
                hConnect,
                L"GET",
                path.c_str(),
                NULL,
                WINHTTP_NO_REFERER,
                types,
                WINHTTP_FLAG_SECURE
            );

            if(hRequest)
            {
                BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

                if(bResults)
                {
                    if(WinHttpReceiveResponse(hRequest, NULL))
                    {
                        // Got an http response
                        DWORD dwSize = 0;
                        LPSTR outBuffer;
                        do
                        {
                            dwSize = 0;
                            if(!WinHttpQueryDataAvailable(hRequest, &dwSize))
                            {
                                L_ERROR("GetTextRequestFromUrl :: Error querying available data.");
                                L_ERROR("GetTextRequestFromUrl :: Error code = %u.", GetLastError());
                                break;
                            }

                            outBuffer = new char[dwSize + 1];
                            ZeroMemory(outBuffer, dwSize + 1);

                            DWORD dwRead = 0;
                            if(!WinHttpReadData(hRequest, (LPVOID)outBuffer, dwSize, &dwRead))
                            {
                                L_ERROR("GetTextRequestFromUrl :: Error reading available data.");
                                L_ERROR("GetTextRequestFromUrl :: Error code = %u.", GetLastError());
                            }
                            else
                            {
                                result += outBuffer;
                            }

                            delete[] outBuffer;

                        } while(dwSize > 0);
                    }
                }
                else
                {
                    L_ERROR("GetTextRequestFromUrl :: Error sending request.");
                    L_ERROR("GetTextRequestFromUrl :: Error code = %u.", GetLastError());
                }

                WinHttpCloseHandle(hRequest);
            }
            else
            {
                L_ERROR("GetTextRequestFromUrl :: Error get request failed.");
                L_ERROR("GetTextRequestFromUrl :: Error code = %u.", GetLastError());
            }

            WinHttpCloseHandle(hConnect);
        }
        else
        {
            L_ERROR("GetTextRequestFromUrl :: Error connecting.");
            L_ERROR("GetTextRequestFromUrl :: Error code = %u.", GetLastError());
        }

        WinHttpCloseHandle(hSession);
    }
    else
    {
        L_ERROR("GetTextRequestFromUrl :: Error in session creation.");
        L_ERROR("GetTextRequestFromUrl :: Error code = %u.", GetLastError());
    }

    return result;
}

static void GetDataRequestFromUrl(const std::wstring& hostname, const std::wstring& path, const std::string& filename, std::atomic<size_t>* current_size)
{
    HINTERNET hSession = WinHttpOpen(
        L"WinHTTP Agent/5.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if(hSession)
    {
        HINTERNET hConnect = WinHttpConnect(hSession, hostname.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);

        if(hConnect)
        {
            LPCWSTR types[] = { L"application/*", NULL };

            HINTERNET hRequest = WinHttpOpenRequest(
                hConnect,
                L"GET",
                path.c_str(),
                NULL,
                WINHTTP_NO_REFERER,
                types,
                WINHTTP_FLAG_SECURE
            );

            if(hRequest)
            {
                BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

                if(bResults)
                {
                    if(WinHttpReceiveResponse(hRequest, NULL))
                    {
                        // Got an http response
                        DWORD dwSize = 0;
                        LPSTR outBuffer;
                        FILE* f = fopen(filename.c_str(), "wb");
                        if(f)
                        {
                            do
                            {
                                dwSize = 0;
                                if(!WinHttpQueryDataAvailable(hRequest, &dwSize))
                                {
                                    L_ERROR("GetDataRequestFromUrl :: Error querying available data.");
                                    L_ERROR("GetDataRequestFromUrl :: Error code = %u.", GetLastError());
                                    break;
                                }

                                outBuffer = new char[dwSize];
                                ZeroMemory(outBuffer, dwSize);

                                DWORD dwRead = 0;
                                if(!WinHttpReadData(hRequest, (LPVOID)outBuffer, dwSize, &dwRead))
                                {
                                    L_ERROR("GetDataRequestFromUrl :: Error reading available data.");
                                    L_ERROR("GetDataRequestFromUrl :: Error code = %u.", GetLastError());
                                }
                                else
                                {
                                    current_size->fetch_add((size_t)dwRead, std::memory_order_seq_cst);
                                    fwrite(outBuffer, 1, dwRead, f);
                                }

                                delete[] outBuffer;

                            } while(dwSize > 0);

                            fclose(f);
                        }
                        else
                        {
                            L_ERROR("GetDataRequestFromUrl :: Error opening/creating file.");
                        }
                    }
                }
                else
                {
                    L_ERROR("GetDataRequestFromUrl :: Error sending request.");
                    L_ERROR("GetDataRequestFromUrl :: Error code = %u.", GetLastError());
                }

                WinHttpCloseHandle(hRequest);
            }
            else
            {
                L_ERROR("GetDataRequestFromUrl :: Error get request failed.");
                L_ERROR("GetDataRequestFromUrl :: Error code = %u.", GetLastError());
            }

            WinHttpCloseHandle(hConnect);
        }
        else
        {
            L_ERROR("GetDataRequestFromUrl :: Error connecting.");
            L_ERROR("GetDataRequestFromUrl :: Error code = %u.", GetLastError());
        }

        WinHttpCloseHandle(hSession);
    }
    else
    {
        L_ERROR("GetDataRequestFromUrl :: Error in session creation.");
        L_ERROR("GetDataRequestFromUrl :: Error code = %u.", GetLastError());
    }
}

static void ParseVersionString(const std::string& version, int* major, int* minor, int* patch)
{
    std::istringstream ss(version);
    char dot;
    ss >> *major >> dot >> *minor>> dot >> *patch;
}

static bool IsRemoteVersionNewer(int remote_major, int remote_minor, int remote_patch)
{
    if(remote_major > NodeRenderer_VERSION_MAJOR) return true;

    if(
        remote_major == NodeRenderer_VERSION_MAJOR && 
        remote_minor > NodeRenderer_VERSION_MINOR) return true;
    if(
        remote_major == NodeRenderer_VERSION_MAJOR && 
        remote_minor == NodeRenderer_VERSION_MINOR && 
        remote_patch > NodeRenderer_VERSION_PATCH) return true;

    return false;
}

const bool IsUpdateAvailable(UpdateData* data)
{
    assert(data != nullptr);

    std::string response = GetTextRequestFromUrl(L"api.github.com", L"/repos/lPrimemaster/NodeRenderer/releases");

    size_t first_tag_idx = response.find("\"tag_name\"");
    if (first_tag_idx == std::string::npos) 
    {
        L_ERROR("GetBinLatestVersion :: Error reading github version release manifest.");
        return false;
    }
    
    size_t end = response.find(",", first_tag_idx) - 1;
    std::string last_remote_version = response.substr(first_tag_idx + 13, end - (first_tag_idx + 13));

    int remote_major, remote_minor, remote_patch;
    ParseVersionString(last_remote_version, &remote_major, &remote_minor, &remote_patch);

    bool newer = IsRemoteVersionNewer(remote_major, remote_minor, remote_patch);

    data->v_major = remote_major;
    data->v_minor = remote_minor;
    data->v_patch = remote_patch;
    data->v_string = last_remote_version;

    if(newer)
    {
        size_t first_tag_idx = response.find("\"browser_download_url\"");
        if (first_tag_idx == std::string::npos) 
        {
            L_ERROR("GetBinLatestVersion :: Error reading github version release manifest.");
            return false;
        }
        
        size_t end = response.find(",", first_tag_idx) - 3;
        data->download_url = response.substr(first_tag_idx + 24, end - (first_tag_idx + 24));

        first_tag_idx = response.find("\"size\"");
        if (first_tag_idx == std::string::npos) 
        {
            L_ERROR("GetBinLatestVersion :: Error reading github version release manifest.");
            return false;
        }
        
        end = response.find(",", first_tag_idx);
        std::string size_str = response.substr(first_tag_idx + 7, end - (first_tag_idx + 7));

        data->download_size = std::stoull(size_str);
    }

    return newer;
}

static constexpr const char* installer_name = "nr_update.exe"; 

void DownloadClient(const std::string& url, std::atomic<size_t>* current_size)
{
    std::string url_path = url.substr(url.find(".com") + 4);
    std::wstring url_path_w(url_path.size(), L' ');
    url_path_w.resize(std::mbstowcs(&url_path_w[0], url_path.c_str(), url_path.size()));

    GetDataRequestFromUrl(L"github.com", url_path_w, installer_name, current_size);
}

const bool UpdateClient()
{
    // Now run the installer and close the program
    TCHAR dir[512];
    DWORD rLen = GetCurrentDirectory(512, dir);
    if(512 < rLen)
    {
        L_ERROR("UpdateClient :: Couldn't allocate space for the full update executable directory [Max 512].");
        return false;
    }

    if(!ShellExecute(NULL, "runas", installer_name, NULL, dir, SW_SHOWNORMAL))
    {
        L_ERROR("UpdateClient :: Failed to create installer process from file.");
        L_ERROR("UpdateClient :: Windows Error = %d.", GetLastError());
        return false;
    }
    else
    {
        // Exit the application flag
        return true;
    }
}
