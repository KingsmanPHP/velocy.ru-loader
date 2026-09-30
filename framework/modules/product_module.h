#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Products {

    enum class ProductState {
        Ready,
        Running,
        Maintenance,
        Updating,
        Error
    };

    class IProductModule {
    public:
        virtual ~IProductModule() = default;

        virtual std::string GetId() const = 0;
        virtual std::string GetName() const = 0;
        virtual std::string GetGame() const = 0;
        virtual std::string GetDescription() const = 0;
        virtual ProductState GetState() const = 0;
        virtual bool IsLocked() const = 0;

        // Pré-requisitos (checa se o jogo está rodando, anticheat ativo, etc.)
        virtual bool CheckPrerequisites(std::string& outErrorMessage) = 0;

        // Injeção / Execução do módulo
        virtual bool Execute(const std::string& authToken, std::function<void(float progress, const std::string& status)> onProgress) = 0;
    };

} // namespace Products
