#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <functional>
#include <filesystem>

namespace Loader
{
    struct ProductModule
    {
        std::string productId;
        std::string name;
        std::string binaryDir;
        std::string targetProcessName;
        std::string explicitExeName;
        std::string downloadUrl;
        std::string localFileName;
        std::string description;
    };

    class ProductManager
    {
    public:
        static ProductManager& Instance()
        {
            static ProductManager s_instance;
            return s_instance;
        }

        ProductManager();
        ~ProductManager();

        void RegisterModule(const ProductModule& mod);
        const ProductModule* GetModule(const std::string& productId) const;

        std::string GetAppStorageDir() const;
        std::string FindLatestBuild(const std::string& dirPath) const;
        bool EnsureBinaryAvailable(const ProductModule& mod, std::string& outExePath, std::string& outError);

        bool LaunchProduct(const std::string& productId, std::string& outError);
        bool StopActiveProduct();

        bool IsRunning(const std::string& productId) const;
        bool IsAnyRunning() const;

        std::string GetActiveProductId() const;
        std::string GetActiveProductName() const;
        DWORD GetActiveProcessId() const;

        void Update();

        static bool IsProcessNameRunning(const std::wstring& processSubstr);
        static void KillProcessByName(const std::wstring& processSubstr);

    private:
        void RegisterDefaultModules();
        void CleanupProcessHandles();

        std::map<std::string, ProductModule> m_modules;
        std::atomic<bool> m_isRunning{ false };
        std::string m_activeProductId;
        std::string m_activeProductName;
        HANDLE m_hProcess = NULL;
        HANDLE m_hThread = NULL;
        DWORD m_dwProcessId = 0;
    };
}
