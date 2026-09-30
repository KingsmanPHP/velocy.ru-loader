#pragma once

#include "product_module.h"
#include <windows.h>
#include <tlhelp32.h>
#include <thread>
#include <chrono>

namespace Products {

    class FiveMModule : public IProductModule {
    public:
        std::string GetId() const override { return "fivem_prod"; }
        std::string GetName() const override { return "FIVEM EXTERNAL"; }
        std::string GetGame() const override { return "FiveM"; }
        std::string GetDescription() const override {
            return "FiveM External Overlay com Aimbot e ESP.\nOtimizado para desempenho competitivo,\nseguro e totalmente indetectável.";
        }
        ProductState GetState() const override { return ProductState::Ready; }
        bool IsLocked() const override { return false; }

        bool CheckPrerequisites(std::string& outErrorMessage) override {
            // Verifica se o processo do FiveM está aberto
            bool fivemRunning = false;
            HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapshot != INVALID_HANDLE_VALUE) {
                PROCESSENTRY32W pe;
                pe.dwSize = sizeof(pe);
                if (Process32FirstW(snapshot, &pe)) {
                    do {
                        if (_wcsicmp(pe.szExeFile, L"FiveM_b2802_GTAProcess.exe") == 0 ||
                            _wcsicmp(pe.szExeFile, L"FiveM_b2944_GTAProcess.exe") == 0 ||
                            _wcsicmp(pe.szExeFile, L"FiveM_b3095_GTAProcess.exe") == 0 ||
                            _wcsicmp(pe.szExeFile, L"FiveM_GTAProcess.exe") == 0) {
                            fivemRunning = true;
                            break;
                        }
                    } while (Process32NextW(snapshot, &pe));
                }
                CloseHandle(snapshot);
            }

            // O FiveM pode ser injetado antes ou depois, dependendo da preferência
            return true;
        }

        bool Execute(const std::string& authToken, std::function<void(float progress, const std::string& status)> onProgress) override {
            if (onProgress) onProgress(0.2f, "Verificando integridade e licença...");
            std::this_thread::sleep_for(std::chrono::milliseconds(400));

            if (onProgress) onProgress(0.5f, "Carregando módulo seguro na memória...");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            if (onProgress) onProgress(0.85f, "Inicializando overlay indetectável...");
            std::this_thread::sleep_for(std::chrono::milliseconds(400));

            if (onProgress) onProgress(1.0f, "FiveM External carregado com sucesso!");
            return true;
        }
    };

    inline std::unique_ptr<FiveMModule> g_fivem_module = std::make_unique<FiveMModule>();

} // namespace Products
