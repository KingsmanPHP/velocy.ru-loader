#pragma once

#include <string>
#include <vector>
#include <Windows.h>

namespace VelocyAuth {

    struct ProductItem {
        std::string id;
        std::string name;
        std::string game;
        std::string version;
        std::string status;       // "active", "updating", etc.
        std::string expires_at;   // "Vitalício" ou data formatada
        std::string description;
    };

    struct UserSession {
        bool authenticated = false;
        std::string token;
        std::string user_id;
        std::string username;
        std::string email;
        std::string role;
        std::string hwid;
        std::vector<ProductItem> products;
    };

    // Obter instância global da sessão
    UserSession& GetSession();

    // HWID do computador (CPUID + Volume C: + BIOS SMBIOS)
    std::string GetLocalHWID();

    // Operações de Sessão em Disco (%APPDATA%\velocy\session.json)
    bool SalvarSessao();
    bool CarregarSessao();
    void LimparSessao();

    // Login Direto com Email e Senha
    bool LoginDireto(const std::string& email, const std::string& password, std::string& outError);

    // Login via Navegador com Código de Sessão
    std::string CriarCodigoNavegador();
    bool ChecarSessaoNavegador(const std::string& code);

    // Atualiza a lista de produtos/assinaturas do usuário
    bool AtualizarAssinaturas();

} // namespace VelocyAuth
