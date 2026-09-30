import { NextRequest, NextResponse } from "next/server";

// Rota de versão do Loader: GET /api/loader/version
export async function GET(request: NextRequest) {
  return NextResponse.json({
    // Versão atual do aplicativo distribuído
    version: "1.0.0",

    // Link direto do binário mais recente para o Auto-Updater
    download_url: "https://vortexcheats-five.vercel.app/downloads/velocy.ru.exe",

    // Link do instalador completo para novos usuários no site
    installer_url: "https://vortexcheats-five.vercel.app/downloads/velocy.ru_Setup_v1.0.0.exe",

    // Mensagem de novidades exibida no modal do loader
    changelog: "Lançamento inicial oficial do velocy.ru com suporte a FiveM External e atualizações automáticas.",

    // Se true, impede o uso do loader antigo até atualizar
    mandatory: true,

    // Data de lançamento
    released_at: new Date().toISOString()
  });
}
