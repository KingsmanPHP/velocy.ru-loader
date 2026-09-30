#pragma once

#include "product_module.h"

namespace Products {

    class FortniteModule : public IProductModule {
    public:
        std::string GetId() const override { return "fortnite_prod"; }
        std::string GetName() const override { return "FORTNITE EXTERNAL"; }
        std::string GetGame() const override { return "Fortnite"; }
        std::string GetDescription() const override {
            return "External indetectável com Aimbot suave e ESP\ncompleto. Produto em desenvolvimento e otimização\npara a temporada atual.";
        }
        ProductState GetState() const override { return ProductState::Maintenance; }
        bool IsLocked() const override { return true; }

        bool CheckPrerequisites(std::string& outErrorMessage) override {
            outErrorMessage = "Produto atualmente em manutenção e desenvolvimento.";
            return false;
        }

        bool Execute(const std::string& authToken, std::function<void(float progress, const std::string& status)> onProgress) override {
            return false;
        }
    };

    inline std::unique_ptr<FortniteModule> g_fortnite_module = std::make_unique<FortniteModule>();

} // namespace Products
