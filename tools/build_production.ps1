# Script de Build e Empacotamento de Produção - velocy.ru
param (
    [string]$Configuration = "Release",
    [string]$Platform = "x64",
    [string]$Version = "1.0.0"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$ProjectRoot = Resolve-Path "$ScriptDir\.."
$DistDir = "$ProjectRoot\dist"
$StagingDir = "$DistDir\staging"
$InstallerScript = "$ProjectRoot\installer\setup.iss"

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host "         VELOCY.RU - PIPELINE DE PRODUÇÃO            " -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host "Versão: $Version" -ForegroundColor Yellow
Write-Host "Configuração: $Configuration | $Platform" -ForegroundColor Yellow

# 1. Fechar processos anteriores para liberar locks
Write-Host "`n[1/5] Verificando processos em execução..." -ForegroundColor Gray
Get-Process | Where-Object { $_.Path -like "*example_win32_directx11*" -or $_.Path -like "*velocy*" } | Stop-Process -Force -ErrorAction SilentlyContinue

# 2. Compilar com MSBuild
Write-Host "`n[2/5] Compilando solução Release x64..." -ForegroundColor Cyan
$MSBuildPath = "D:\vs\MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $MSBuildPath)) {
    $MSBuildPath = "MSBuild.exe"
}

$SlnPath = "$ProjectRoot\framework.sln"
& $MSBuildPath $SlnPath /p:Configuration=$Configuration /p:Platform=$Platform /v:m
if ($LASTEXITCODE -ne 0) {
    Write-Error "Falha na compilação do MSBuild (Código $LASTEXITCODE)"
    exit $LASTEXITCODE
}
Write-Host "  -> Compilação concluída com sucesso!" -ForegroundColor Green

# 3. Preparar diretório de Staging
Write-Host "`n[3/5] Preparando arquivos de staging..." -ForegroundColor Cyan
if (Test-Path $StagingDir) {
    Remove-Item $StagingDir -Recurse -Force
}
New-Item -ItemType Directory -Path $StagingDir -Force | Out-Null

$BuiltExe = "$ProjectRoot\thirdparty\imgui\examples\example_win32_directx11\Release\example_win32_directx11.exe"
if (-not (Test-Path $BuiltExe)) {
    Write-Error "Executável compilado não encontrado em: $BuiltExe"
    exit 1
}

Copy-Item $BuiltExe "$StagingDir\velocy.ru.exe" -Force
Write-Host "  -> Binário copiado como velocy.ru.exe" -ForegroundColor Green

# 4. Compilar Instalador com Inno Setup (ISCC)
Write-Host "`n[4/5] Gerando instalador com Inno Setup..." -ForegroundColor Cyan
$IsccCandidates = @(
    "C:\Users\ibra\AppData\Local\Programs\Inno Setup 6\ISCC.exe",
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    "C:\Program Files\Inno Setup 6\ISCC.exe"
)

$IsccPath = $IsccCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $IsccPath) {
    Write-Error "Compilador Inno Setup (ISCC.exe) não encontrado."
    exit 1
}

& $IsccPath "/DMyAppVersion=$Version" $InstallerScript
if ($LASTEXITCODE -ne 0) {
    Write-Error "Falha na geração do instalador com Inno Setup (Código $LASTEXITCODE)"
    exit $LASTEXITCODE
}

# 5. Resumo e Checksum
$SetupExe = "$DistDir\velocy.ru_Setup_v$Version.exe"
if (Test-Path $SetupExe) {
    $Item = Get-Item $SetupExe
    $Hash = (Get-FileHash $SetupExe -Algorithm SHA256).Hash
    $SizeMB = [math]::Round($Item.Length / 1MB, 2)

    Write-Host "`n=====================================================" -ForegroundColor Green
    Write-Host "   INSTALADOR DE PRODUÇÃO GERADO COM SUCESSO!        " -ForegroundColor Green
    Write-Host "=====================================================" -ForegroundColor Green
    Write-Host "Arquivo:   $($Item.FullName)" -ForegroundColor White
    Write-Host "Tamanho:   $SizeMB MB ($($Item.Length) bytes)" -ForegroundColor White
    Write-Host "SHA256:    $Hash" -ForegroundColor Yellow
    Write-Host "Versão:    $Version" -ForegroundColor White
    Write-Host "`nPronto para distribuição aos clientes ou upload na CDN/Vercel!" -ForegroundColor Cyan
} else {
    Write-Error "Instalador esperado não encontrado em $SetupExe"
    exit 1
}
