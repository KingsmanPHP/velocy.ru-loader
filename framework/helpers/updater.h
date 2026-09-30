#pragma once

#include <string>
#include <functional>
#include <atomic>

#define LOADER_VERSION "1.0.0"

namespace AutoUpdater {

    struct UpdateInfo {
        bool has_update = false;
        bool mandatory = false;
        std::string latest_version;
        std::string download_url;
        std::string changelog;
    };

    // Estado global de atualização
    extern std::atomic<bool> g_is_checking;
    extern std::atomic<bool> g_is_updating;
    extern std::atomic<float> g_update_progress;
    extern UpdateInfo g_update_info;
    extern bool g_show_update_modal;

    // Retorna a versão atual do loader
    inline const char* GetCurrentVersion() {
        return LOADER_VERSION;
    }

    // Inicia a verificação de atualizações em segundo plano
    void CheckForUpdatesAsync();

    // Baixa o novo executável e inicia o processo de substituição e reinicialização
    bool DownloadAndApplyUpdate(const std::string& url);

} // namespace AutoUpdater
