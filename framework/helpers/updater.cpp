#include "updater.h"
#include <windows.h>
#include <winhttp.h>
#include <urlmon.h>
#include <shellapi.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "urlmon.lib")

namespace AutoUpdater {

    std::atomic<bool> g_is_checking{ false };
    std::atomic<bool> g_is_updating{ false };
    std::atomic<float> g_update_progress{ 0.f };
    UpdateInfo g_update_info;
    bool g_show_update_modal = false;

    // Comparador simples de versões semânticas (ex: "1.0.1" > "1.0.0")
    static bool IsNewerVersion(const std::string& latest, const std::string& current) {
        if (latest.empty() || current.empty()) return false;
        if (latest == current) return false;

        auto parseVersion = [](const std::string& v) -> std::vector<int> {
            std::vector<int> parts;
            std::stringstream ss(v);
            std::string item;
            while (std::getline(ss, item, '.')) {
                try {
                    parts.push_back(std::stoi(item));
                } catch (...) {
                    parts.push_back(0);
                }
            }
            while (parts.size() < 3) parts.push_back(0);
            return parts;
        };

        std::vector<int> l = parseVersion(latest);
        std::vector<int> c = parseVersion(current);

        for (size_t i = 0; i < 3; ++i) {
            if (l[i] > c[i]) return true;
            if (l[i] < c[i]) return false;
        }
        return false;
    }

    static std::string ExtractJsonString(const std::string& json, const std::string& key) {
        size_t pos = json.find("\"" + key + "\":");
        if (pos == std::string::npos) return "";

        size_t start = json.find("\"", pos + key.length() + 3);
        if (start == std::string::npos) return "";

        size_t end = json.find("\"", start + 1);
        if (end == std::string::npos) return "";

        return json.substr(start + 1, end - start - 1);
    }

    static bool ExtractJsonBool(const std::string& json, const std::string& key, bool defaultVal = false) {
        size_t pos = json.find("\"" + key + "\":");
        if (pos == std::string::npos) return defaultVal;

        size_t start = json.find_first_not_of(" \t\r\n", pos + key.length() + 3);
        if (start == std::string::npos) return defaultVal;

        if (json.substr(start, 4) == "true") return true;
        if (json.substr(start, 5) == "false") return false;
        return defaultVal;
    }

    void CheckForUpdatesAsync() {
        if (g_is_checking.load() || g_is_updating.load()) return;
        g_is_checking = true;

        std::thread([]() {
            HINTERNET hSession = WinHttpOpen(L"VortexUpdater/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!hSession) {
                g_is_checking = false;
                return;
            }

            WinHttpSetTimeouts(hSession, 4000, 4000, 4000, 4000);

            HINTERNET hConnect = WinHttpConnect(hSession, L"vortexcheats-five.vercel.app", INTERNET_DEFAULT_HTTPS_PORT, 0);
            if (!hConnect) {
                WinHttpCloseHandle(hSession);
                g_is_checking = false;
                return;
            }

            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/api/loader/version", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
            if (!hRequest) {
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                g_is_checking = false;
                return;
            }

            DWORD dwSecurityFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                                   SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                                   SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                                   SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
            WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwSecurityFlags, sizeof(dwSecurityFlags));

            if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                if (WinHttpReceiveResponse(hRequest, NULL)) {
                    DWORD statusCode = 0;
                    DWORD statusSize = sizeof(statusCode);
                    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);

                    if (statusCode == 200) {
                        std::string responseBody;
                        DWORD bytesAvailable = 0;
                        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
                            std::vector<char> buffer(bytesAvailable + 1);
                            DWORD bytesRead = 0;
                            if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead) && bytesRead > 0) {
                                responseBody.append(buffer.data(), bytesRead);
                            }
                        }

                        std::string latestVer = ExtractJsonString(responseBody, "version");
                        std::string downloadUrl = ExtractJsonString(responseBody, "download_url");
                        std::string changelog = ExtractJsonString(responseBody, "changelog");
                        bool mandatory = ExtractJsonBool(responseBody, "mandatory", false);

                        if (!latestVer.empty() && IsNewerVersion(latestVer, LOADER_VERSION)) {
                            g_update_info.has_update = true;
                            g_update_info.latest_version = latestVer;
                            g_update_info.download_url = downloadUrl;
                            g_update_info.changelog = changelog.empty() ? "Novas melhorias de desempenho e estabilidade." : changelog;
                            g_update_info.mandatory = mandatory;
                            g_show_update_modal = true;
                        }
                    }
                }
            }

            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            g_is_checking = false;
        }).detach();
    }

    bool DownloadAndApplyUpdate(const std::string& url) {
        if (g_is_updating.load() || url.empty()) return false;
        g_is_updating = true;
        g_update_progress = 0.05f;

        std::thread([url]() {
            // Obter caminho temporário para baixar o novo binário
            char tempPath[MAX_PATH];
            GetTempPathA(MAX_PATH, tempPath);
            std::string updateExe = std::string(tempPath) + "vortex_loader_update.exe";

            // Obter caminho do executável atual
            wchar_t currentExePathW[MAX_PATH];
            GetModuleFileNameW(NULL, currentExePathW, MAX_PATH);

            int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, currentExePathW, -1, NULL, 0, NULL, NULL);
            std::string currentExePath(sizeNeeded, 0);
            WideCharToMultiByte(CP_UTF8, 0, currentExePathW, -1, &currentExePath[0], sizeNeeded, NULL, NULL);
            if (!currentExePath.empty() && currentExePath.back() == '\0') currentExePath.pop_back();

            // Fazer o download via WinHttp ou URLDownloadToFile
            // Para simplicidade e confiabilidade em URLs externas HTTPS:
            HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), updateExe.c_str(), 0, NULL);
            if (FAILED(hr)) {
                g_is_updating = false;
                return;
            }

            g_update_progress = 0.90f;

            // Criar script batch auxiliar para aguardar o fechamento do PID atual, substituir e reiniciar
            DWORD currentPid = GetCurrentProcessId();
            std::string batScript = std::string(tempPath) + "vortex_update_runner.bat";

            std::ofstream bat(batScript, std::ios::trunc);
            if (bat.is_open()) {
                bat << "@echo off\r\n";
                bat << "timeout /t 1 /nobreak >nul\r\n";
                bat << ":loop\r\n";
                bat << "tasklist /fi \"pid eq " << currentPid << "\" 2>nul | find \"" << currentPid << "\" >nul\r\n";
                bat << "if not errorlevel 1 (\r\n";
                bat << "    timeout /t 1 /nobreak >nul\r\n";
                bat << "    goto loop\r\n";
                bat << ")\r\n";
                bat << "copy /y \"" << updateExe << "\" \"" << currentExePath << "\" >nul\r\n";
                bat << "del /f /q \"" << updateExe << "\" >nul\r\n";
                bat << "start \"\" \"" << currentExePath << "\"\r\n";
                bat << "del /f /q \"%~f0\" >nul\r\n";
                bat.close();
            }

            g_update_progress = 1.0f;

            // Executar o script em segundo plano oculto
            ShellExecuteA(NULL, "open", batScript.c_str(), NULL, NULL, SW_HIDE);

            // Fechar o aplicativo atual para permitir que o script o substitua
            ExitProcess(0);
        }).detach();

        return true;
    }

} // namespace AutoUpdater
