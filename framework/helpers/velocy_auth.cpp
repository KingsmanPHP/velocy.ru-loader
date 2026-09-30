#include "velocy_auth.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <intrin.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

namespace VelocyAuth {

    static UserSession g_session;

    UserSession& GetSession() {
        return g_session;
    }

    // Configuração de Domínios
    static const wchar_t* kPrimaryHost = L"vortexcheats-five.vercel.app";
    static const wchar_t* kFallbackHost = L"velocy.ru";

    // HWID Único da Máquina
    std::string GetLocalHWID() {
        int cpuInfo[4] = { 0 };
        __cpuid(cpuInfo, 0);

        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0');
        ss << std::setw(8) << cpuInfo[0] << std::setw(8) << cpuInfo[3];

        __cpuid(cpuInfo, 1);
        ss << std::setw(8) << cpuInfo[0] << std::setw(8) << cpuInfo[3];
        std::string cpuHash = ss.str();

        DWORD serial = 0;
        std::string diskSerial = "DISK00000000";
        if (GetVolumeInformationW(L"C:\\", NULL, 0, &serial, NULL, NULL, NULL, 0)) {
            std::stringstream ds;
            ds << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << serial;
            diskSerial = ds.str();
        }

        std::string biosUuid = "BIOSDEFAULT00000";
        const DWORD sig = 'RSMB';
        DWORD sz = GetSystemFirmwareTable(sig, 0, NULL, 0);
        if (sz > 0) {
            std::vector<BYTE> buf(sz);
            if (GetSystemFirmwareTable(sig, 0, buf.data(), sz)) {
                uint64_t h = 14695981039346656037ULL;
                for (size_t i = 0; i < buf.size(); ++i) {
                    h ^= buf[i];
                    h *= 1099511628211ULL;
                }
                std::stringstream bs;
                bs << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << h;
                biosUuid = bs.str();
            }
        }

        std::string raw = cpuHash + "-" + diskSerial + "-" + biosUuid;
        uint64_t h1 = 14695981039346656037ULL;
        uint64_t h2 = 0xCBF29CE484222325ULL;
        for (size_t i = 0; i < raw.length(); ++i) {
            h1 = (h1 ^ static_cast<uint8_t>(raw[i])) * 1099511628211ULL;
            h2 = (h2 ^ static_cast<uint8_t>(raw[raw.length() - 1 - i])) * 1099511628211ULL;
        }

        std::stringstream finalSs;
        finalSs << std::hex << std::uppercase << std::setfill('0');
        uint16_t p1 = static_cast<uint16_t>((h1 >> 48) & 0xFFFF);
        uint16_t p2 = static_cast<uint16_t>((h1 >> 32) & 0xFFFF);
        uint16_t p3 = static_cast<uint16_t>((h2 >> 16) & 0xFFFF);
        uint16_t p4 = static_cast<uint16_t>(h2 & 0xFFFF);
        finalSs << "HWID-" << std::setw(4) << p1 << "-" << std::setw(4) << p2 << "-" << std::setw(4) << p3 << "-" << std::setw(4) << p4;
        return finalSs.str();
    }

    // Caminho da sessão
    static std::string GetSessionPath() {
        char appData[MAX_PATH];
        if (GetEnvironmentVariableA("APPDATA", appData, MAX_PATH) > 0) {
            std::string dir = std::string(appData) + "\\velocy";
            CreateDirectoryA(dir.c_str(), NULL);
            return dir + "\\session.json";
        }
        return "session.json";
    }

    bool SalvarSessao() {
        std::ofstream file(GetSessionPath(), std::ios::trunc);
        if (!file.is_open()) return false;

        file << "{\n";
        file << "  \"token\": \"" << g_session.token << "\",\n";
        file << "  \"username\": \"" << g_session.username << "\",\n";
        file << "  \"email\": \"" << g_session.email << "\",\n";
        file << "  \"user_id\": \"" << g_session.user_id << "\",\n";
        file << "  \"role\": \"" << g_session.role << "\",\n";
        file << "  \"hwid\": \"" << g_session.hwid << "\"\n";
        file << "}\n";
        file.close();
        return true;
    }

    bool CarregarSessao() {
        std::ifstream file(GetSessionPath());
        if (!file.is_open()) return false;

        std::string line;
        std::string token, username, email, user_id, role, hwid;

        auto extractVal = [](const std::string& l, const std::string& key) -> std::string {
            size_t p = l.find("\"" + key + "\":");
            if (p != std::string::npos) {
                size_t st = l.find("\"", p + key.length() + 3);
                if (st != std::string::npos) {
                    size_t ed = l.find("\"", st + 1);
                    if (ed != std::string::npos) return l.substr(st + 1, ed - st - 1);
                }
            }
            return "";
        };

        while (std::getline(file, line)) {
            std::string v;
            if (!(v = extractVal(line, "token")).empty()) token = v;
            if (!(v = extractVal(line, "username")).empty()) username = v;
            if (!(v = extractVal(line, "email")).empty()) email = v;
            if (!(v = extractVal(line, "user_id")).empty()) user_id = v;
            if (!(v = extractVal(line, "role")).empty()) role = v;
            if (!(v = extractVal(line, "hwid")).empty()) hwid = v;
        }
        file.close();

        if (!token.empty() && hwid == GetLocalHWID()) {
            g_session.authenticated = true;
            g_session.token = token;
            g_session.username = username;
            g_session.email = email;
            g_session.user_id = user_id;
            g_session.role = role.empty() ? "VIP Member" : role;
            g_session.hwid = hwid;
            AtualizarAssinaturas();
            return true;
        }
        return false;
    }

    void LimparSessao() {
        g_session = UserSession();
        DeleteFileA(GetSessionPath().c_str());
    }

    // Helper para realizar requisições HTTPS via WinHTTP
    static bool HttpRequest(const std::wstring& verb, const std::wstring& path, const std::string& body, const std::string& bearerToken, std::string& outResponse, int& outStatusCode) {
        const wchar_t* hosts[] = { kPrimaryHost, kFallbackHost };

        for (const wchar_t* host : hosts) {
            HINTERNET hSession = WinHttpOpen(L"VelocyLoader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!hSession) continue;

            WinHttpSetTimeouts(hSession, 5000, 5000, 5000, 5000);

            HINTERNET hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
            if (!hConnect) {
                WinHttpCloseHandle(hSession);
                continue;
            }

            HINTERNET hRequest = WinHttpOpenRequest(hConnect, verb.c_str(), path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
            if (!hRequest) {
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }

            DWORD dwSecurityFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                                   SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                                   SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                                   SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
            WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwSecurityFlags, sizeof(dwSecurityFlags));

            std::wstring headers = L"Content-Type: application/json\r\nAccept: application/json\r\n";
            if (!bearerToken.empty()) {
                std::wstring wToken(bearerToken.begin(), bearerToken.end());
                headers += L"Authorization: Bearer " + wToken + L"\r\n";
            }

            BOOL bResults = WinHttpSendRequest(
                hRequest,
                headers.c_str(),
                (DWORD)headers.length(),
                body.empty() ? NULL : (LPVOID)body.c_str(),
                (DWORD)body.length(),
                (DWORD)body.length(),
                0
            );

            if (bResults) {
                bResults = WinHttpReceiveResponse(hRequest, NULL);
            }

            if (bResults) {
                DWORD statusCode = 0;
                DWORD statusSize = sizeof(statusCode);
                WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
                outStatusCode = (int)statusCode;

                // Se o servidor retornou 301/302/307/308 ou 404, esta rota não é servida diretamente por este host; tenta o próximo
                if (outStatusCode == 301 || outStatusCode == 302 || outStatusCode == 307 || outStatusCode == 308 || outStatusCode == 404) {
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    continue;
                }

                DWORD dwSize = 0;
                outResponse.clear();
                do {
                    dwSize = 0;
                    if (!WinHttpQueryDataAvailable(hRequest, &dwSize) || dwSize == 0) break;
                    std::vector<char> buf(dwSize + 1, 0);
                    DWORD dwDownloaded = 0;
                    if (WinHttpReadData(hRequest, buf.data(), dwSize, &dwDownloaded)) {
                        outResponse.append(buf.data(), dwDownloaded);
                    }
                } while (dwSize > 0);

                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return true;
            }

            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
        }
        return false;
    }

    // Parser simples de JSON string
    static std::string GetJsonString(const std::string& json, const std::string& key) {
        size_t pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";

        size_t colon = json.find(":", pos);
        if (colon == std::string::npos) return "";

        size_t quoteStart = json.find("\"", colon);
        if (quoteStart == std::string::npos) return "";

        size_t quoteEnd = json.find("\"", quoteStart + 1);
        if (quoteEnd == std::string::npos) return "";

        return json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
    }

    static std::string Trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static std::string EscapeJson(const std::string& str) {
        std::string res;
        for (char c : str) {
            if (c == '"') res += "\\\"";
            else if (c == '\\') res += "\\\\";
            else res += c;
        }
        return res;
    }

    // Login Direto com Email e Senha
    bool LoginDireto(const std::string& email, const std::string& password, std::string& outError) {
        std::string cleanEmail = Trim(email);
        std::string cleanPass = Trim(password);
        std::string hwid = GetLocalHWID();

        std::string body = "{\"email\":\"" + EscapeJson(cleanEmail) + "\",\"password\":\"" + EscapeJson(cleanPass) + "\",\"hwid\":\"" + hwid + "\"}";

        std::string resp;
        int status = 0;
        bool ok = HttpRequest(L"POST", L"/api/auth/login", body, "", resp, status);

        // Log de diagnóstico em %APPDATA%\velocy\auth_log.txt
        std::string logPath = GetSessionPath();
        size_t lastSlash = logPath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            logPath = logPath.substr(0, lastSlash + 1) + "auth_log.txt";
            std::ofstream logF(logPath, std::ios::app);
            if (logF.is_open()) {
                logF << "[LOGIN ATTEMPT] Email: [" << cleanEmail << "] PassLen: " << cleanPass.length() << " HWID: " << hwid << " Status: " << status << " Resp: " << resp << "\n";
                logF.close();
            }
        }

        if (!ok || resp.empty()) {
            outError = "Falha ao conectar com o servidor. Verifique sua conexao.";
            return false;
        }

        if (status >= 400 || resp.find("\"error\"") != std::string::npos) {
            std::string err = GetJsonString(resp, "error");
            if (err.empty()) err = GetJsonString(resp, "message");
            if (err.find("Invalid login credentials") != std::string::npos || err.find("inválidas") != std::string::npos || err.find("invalidas") != std::string::npos) {
                outError = "E-mail ou Senha incorretos.";
            } else if (err.find("HWID") != std::string::npos) {
                outError = "HWID Mismatch: Este computador nao bate com o cadastro no site.";
            } else {
                outError = err.empty() ? "Credenciais invalidas ou erro no servidor." : err;
            }
            return false;
        }

        std::string token = GetJsonString(resp, "access_token");
        if (token.empty()) token = GetJsonString(resp, "token");

        if (token.empty()) {
            outError = "Resposta invalida do servidor de autenticacao.";
            return false;
        }

        std::string username = GetJsonString(resp, "username");
        if (username.empty()) username = GetJsonString(resp, "name");
        if (username.empty()) username = cleanEmail.substr(0, cleanEmail.find("@"));

        std::string userId = GetJsonString(resp, "id");
        std::string role = GetJsonString(resp, "role");

        g_session.authenticated = true;
        g_session.token = token;
        g_session.username = username;
        g_session.email = email;
        g_session.user_id = userId;
        g_session.role = role.empty() ? "VIP Member" : role;
        g_session.hwid = hwid;

        SalvarSessao();
        AtualizarAssinaturas();
        return true;
    }

    // Login via Navegador
    std::string CriarCodigoNavegador() {
        std::string resp;
        int status = 0;
        if (HttpRequest(L"POST", L"/api/auth/loader-session", "{}", "", resp, status)) {
            std::string code = GetJsonString(resp, "sessionCode");
            if (!code.empty()) return code;
        }
        return "";
    }

    bool ChecarSessaoNavegador(const std::string& code) {
        if (code.empty()) return false;
        std::string hwid = GetLocalHWID();
        std::wstring path = L"/api/auth/loader-session?code=" + std::wstring(code.begin(), code.end()) + L"&hwid=" + std::wstring(hwid.begin(), hwid.end());

        std::string resp;
        int status = 0;
        if (HttpRequest(L"GET", path, "", "", resp, status)) {
            if (resp.find("\"authenticated\"") != std::string::npos) {
                g_session.authenticated = true;
                g_session.token = GetJsonString(resp, "access_token");
                g_session.username = GetJsonString(resp, "name");
                if (g_session.username.empty()) g_session.username = GetJsonString(resp, "username");
                g_session.email = GetJsonString(resp, "email");
                g_session.role = GetJsonString(resp, "role");
                g_session.hwid = hwid;

                SalvarSessao();
                AtualizarAssinaturas();
                return true;
            }
        }
        return false;
    }

    // Busca produtos e assinaturas do usuário
    bool AtualizarAssinaturas() {
        if (!g_session.authenticated || g_session.token.empty()) return false;

        std::string resp;
        int status = 0;
        if (!HttpRequest(L"GET", L"/api/subscriptions", "", g_session.token, resp, status)) {
            return false;
        }

        g_session.products.clear();

        // Parser dos blocos de licenças retornados pela API
        size_t pos = 0;
        while ((pos = resp.find("\"product_id\":", pos)) != std::string::npos) {
            ProductItem item;
            item.id = GetJsonString(resp.substr(pos, 300), "product_id");

            if (item.id == "prod-cs2") {
                pos += 50;
                continue;
            }
            if (item.name.empty()) {
                item.name = "FiveM Legit External";
            }

            item.game = "FiveM / GTA V";
            item.version = "v1.0.4";
            item.status = "Ativo (Online)";

            size_t expPos = resp.find("\"expires_at\":", pos);
            if (expPos != std::string::npos && expPos < pos + 500) {
                std::string exp = GetJsonString(resp.substr(expPos, 200), "expires_at");
                if (exp.find("2028") != std::string::npos || exp.find("2099") != std::string::npos || exp.empty()) {
                    item.expires_at = "Vitalício";
                } else {
                    item.expires_at = exp.substr(0, 10);
                }
            } else {
                item.expires_at = "Vitalício";
            }

            g_session.products.push_back(item);
            pos += 50;
        }

        // Se a resposta contiver produtos mas não licenças individuais (ou fallback padrão)
        if (g_session.products.empty()) {
            ProductItem fivem;
            fivem.id = "prod-fivem";
            fivem.name = "FiveM Legit External";
            fivem.game = "FiveM / GTA V";
            fivem.version = "v2.0.1";
            fivem.status = "Ativo";
            fivem.expires_at = "Vitalício";
            fivem.description = "Aimbot Legit, ESP Completo, No-Clip e Proteções contra tela.";
            g_session.products.push_back(fivem);
        }

        return true;
    }

} // namespace VelocyAuth
