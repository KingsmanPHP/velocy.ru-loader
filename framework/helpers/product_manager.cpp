#include "product_manager.h"
#include <tlhelp32.h>
#include <shlobj.h>
#include <urlmon.h>
#include <iostream>
#include <chrono>

#pragma comment(lib, "urlmon.lib")

namespace Loader
{
    ProductManager::ProductManager()
    {
        RegisterDefaultModules();
    }

    ProductManager::~ProductManager()
    {
        StopActiveProduct();
    }

    std::string ProductManager::GetAppStorageDir() const
    {
        char localAppData[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData)))
        {
            std::string dir = std::string(localAppData) + "\\Velocy\\bin";
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            return dir;
        }

        std::string fallback = "C:\\Velocy\\bin";
        std::error_code ec;
        std::filesystem::create_directories(fallback, ec);
        return fallback;
    }

    void ProductManager::RegisterDefaultModules()
    {
        ProductModule fivem;
        fivem.productId = "prod-fivem";
        fivem.name = "Rocket Baimless";
        fivem.binaryDir = "C:\\Users\\ibra\\Downloads\\rocket-baimless\\x64\\Release";
        fivem.targetProcessName = "FiveM_b2699_GTAProcess.exe";
        fivem.explicitExeName = "rocket-baimless.exe";
        fivem.downloadUrl = "https://wotnkcjfhmlwfqpivdon.supabase.co/storage/v1/object/public/app-config/loaders/prod-fivem_1790562570124_latest_build_27.09.2026.exe";
        fivem.localFileName = "rocket-baimless.exe";
        fivem.description = "FiveM External Overlay com Aimbot e ESP";
        RegisterModule(fivem);
    }

    void ProductManager::RegisterModule(const ProductModule& mod)
    {
        m_modules[mod.productId] = mod;
    }

    const ProductModule* ProductManager::GetModule(const std::string& productId) const
    {
        auto it = m_modules.find(productId);
        if (it != m_modules.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    std::string ProductManager::FindLatestBuild(const std::string& dirPath) const
    {
        std::error_code ec;
        if (!std::filesystem::exists(dirPath, ec) || !std::filesystem::is_directory(dirPath, ec))
        {
            return "";
        }

        std::filesystem::path newestFile;
        std::filesystem::file_time_type newestTime{};
        bool found = false;

        for (const auto& entry : std::filesystem::directory_iterator(dirPath, ec))
        {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".exe")
            {
                auto lastTime = entry.last_write_time(ec);
                if (!found || lastTime > newestTime)
                {
                    newestTime = lastTime;
                    newestFile = entry.path();
                    found = true;
                }
            }
        }

        if (found)
        {
            return newestFile.string();
        }

        return "";
    }

    bool ProductManager::EnsureBinaryAvailable(const ProductModule& mod, std::string& outExePath, std::string& outError)
    {
        std::string storageDir = GetAppStorageDir();
        std::string targetExe = storageDir + "\\" + (mod.localFileName.empty() ? "product.exe" : mod.localFileName);
        std::error_code ec;

        // 1. Se existir uma build recente na pasta de compilação local (desenvolvimento), copia para a pasta do app
        std::string devBuild = FindLatestBuild(mod.binaryDir);
        if (!devBuild.empty() && std::filesystem::exists(devBuild, ec))
        {
            // Copia se o arquivo de destino não existir ou se a build de dev for mais recente
            bool needCopy = true;
            if (std::filesystem::exists(targetExe, ec))
            {
                auto devTime = std::filesystem::last_write_time(devBuild, ec);
                auto targetTime = std::filesystem::last_write_time(targetExe, ec);
                if (targetTime >= devTime)
                {
                    needCopy = false;
                }
            }

            if (needCopy)
            {
                std::filesystem::copy_file(devBuild, targetExe, std::filesystem::copy_options::overwrite_existing, ec);
            }

            if (std::filesystem::exists(targetExe, ec))
            {
                outExePath = targetExe;
                return true;
            }
        }

        // 2. Se o arquivo já existe na pasta de armazenamento e tem tamanho válido
        if (std::filesystem::exists(targetExe, ec))
        {
            auto sz = std::filesystem::file_size(targetExe, ec);
            if (sz > 50000)
            {
                outExePath = targetExe;
                return true;
            }
        }

        // 3. Se não tiver localmente, faz o download seguro da URL autorizada
        if (!mod.downloadUrl.empty())
        {
            HRESULT hr = URLDownloadToFileA(NULL, mod.downloadUrl.c_str(), targetExe.c_str(), 0, NULL);
            if (SUCCEEDED(hr) && std::filesystem::exists(targetExe, ec))
            {
                auto sz = std::filesystem::file_size(targetExe, ec);
                if (sz > 50000)
                {
                    outExePath = targetExe;
                    return true;
                }
            }
            outError = "Falha ao baixar o executavel da URL segura (Codigo HRESULT: " + std::to_string(hr) + ")";
            return false;
        }

        outError = "Nenhum arquivo executavel disponivel para o produto.";
        return false;
    }

    bool ProductManager::LaunchProduct(const std::string& productId, std::string& outError)
    {
        Update(); // Verifica se algum processo anterior já encerrou

        if (m_isRunning)
        {
            outError = "Outro produto ja esta em execucao: " + m_activeProductName;
            return false;
        }

        const ProductModule* mod = GetModule(productId);
        if (!mod)
        {
            ProductModule dynamicMod;
            dynamicMod.productId = productId;
            dynamicMod.name = "Rocket Baimless";
            dynamicMod.binaryDir = "C:\\Users\\ibra\\Downloads\\rocket-baimless\\x64\\Release";
            dynamicMod.localFileName = "rocket-baimless.exe";
            dynamicMod.downloadUrl = "https://wotnkcjfhmlwfqpivdon.supabase.co/storage/v1/object/public/app-config/loaders/prod-fivem_1790562570124_latest_build_27.09.2026.exe";
            RegisterModule(dynamicMod);
            mod = GetModule(productId);
        }

        std::string exePath;
        if (!EnsureBinaryAvailable(*mod, exePath, outError))
        {
            return false;
        }

        STARTUPINFOA si{};
        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE; // Oculta qualquer janela inicial
        PROCESS_INFORMATION pi{};

        std::string workDir = std::filesystem::path(exePath).parent_path().string();

        // Linha de comando com argumentos para o cheat
        std::string cmdLine = "\"" + exePath + "\" --user \"VIP User\" --expires \"Lifetime\"";
        std::vector<char> cmdVec(cmdLine.begin(), cmdLine.end());
        cmdVec.push_back('\0');

        // CREATE_NO_WINDOW garante que nenhuma janela de console cmd seja alocada pelo Windows
        BOOL success = CreateProcessA(
            NULL,
            cmdVec.data(),
            NULL,
            NULL,
            FALSE,
            CREATE_NO_WINDOW,
            NULL,
            workDir.empty() ? NULL : workDir.c_str(),
            &si,
            &pi
        );

        if (!success)
        {
            DWORD err = GetLastError();
            outError = "Falha ao iniciar o processo (Codigo de erro: " + std::to_string(err) + ")";
            return false;
        }

        m_hProcess = pi.hProcess;
        m_hThread = pi.hThread;
        m_dwProcessId = pi.dwProcessId;
        m_activeProductId = productId;
        m_activeProductName = mod->name;
        m_isRunning = true;

        return true;
    }

    bool ProductManager::StopActiveProduct()
    {
        if (!m_isRunning || !m_hProcess)
        {
            m_isRunning = false;
            m_activeProductId.clear();
            m_activeProductName.clear();
            return false;
        }

        // Encerra imediatamente o processo do produto
        TerminateProcess(m_hProcess, 0);
        WaitForSingleObject(m_hProcess, 1000);

        CleanupProcessHandles();

        m_isRunning = false;
        m_activeProductId.clear();
        m_activeProductName.clear();

        return true;
    }

    void ProductManager::CleanupProcessHandles()
    {
        if (m_hThread && m_hThread != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hThread);
            m_hThread = NULL;
        }
        if (m_hProcess && m_hProcess != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hProcess);
            m_hProcess = NULL;
        }
        m_dwProcessId = 0;
    }

    void ProductManager::Update()
    {
        if (!m_isRunning || !m_hProcess)
        {
            return;
        }

        DWORD exitCode = 0;
        if (GetExitCodeProcess(m_hProcess, &exitCode))
        {
            if (exitCode != STILL_ACTIVE)
            {
                CleanupProcessHandles();
                m_isRunning = false;
                m_activeProductId.clear();
                m_activeProductName.clear();
            }
        }
    }

    bool ProductManager::IsRunning(const std::string& productId) const
    {
        return m_isRunning && (m_activeProductId == productId);
    }

    bool ProductManager::IsAnyRunning() const
    {
        return m_isRunning;
    }

    std::string ProductManager::GetActiveProductId() const
    {
        return m_activeProductId;
    }

    std::string ProductManager::GetActiveProductName() const
    {
        return m_activeProductName;
    }

    DWORD ProductManager::GetActiveProcessId() const
    {
        return m_dwProcessId;
    }

    bool ProductManager::IsProcessNameRunning(const std::wstring& targetSubstr)
    {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) return false;

        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        bool found = false;

        if (Process32FirstW(hSnap, &pe))
        {
            std::wstring target = targetSubstr;
            for (auto& c : target) c = towlower(c);

            do
            {
                std::wstring name = pe.szExeFile;
                for (auto& c : name) c = towlower(c);

                if (name.find(target) != std::wstring::npos)
                {
                    found = true;
                    break;
                }
            } while (Process32NextW(hSnap, &pe));
        }

        CloseHandle(hSnap);
        return found;
    }

    void ProductManager::KillProcessByName(const std::wstring& targetSubstr)
    {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) return;

        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);

        if (Process32FirstW(hSnap, &pe))
        {
            std::wstring target = targetSubstr;
            for (auto& c : target) c = towlower(c);

            do
            {
                std::wstring name = pe.szExeFile;
                for (auto& c : name) c = towlower(c);

                if (name.find(target) != std::wstring::npos)
                {
                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (hProc)
                    {
                        TerminateProcess(hProc, 0);
                        CloseHandle(hProc);
                    }
                }
            } while (Process32NextW(hSnap, &pe));
        }

        CloseHandle(hSnap);
    }
}
