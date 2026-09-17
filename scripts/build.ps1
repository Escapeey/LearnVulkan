<#
.SYNOPSIS
    配置并编译 LearnVulkan。

.EXAMPLE
    .\scripts\build.ps1 -Configure     # 首次：配置（会联网拉取 GLFW/GLM）
    .\scripts\build.ps1                # 之后：增量编译
    .\scripts\build.ps1 -Clean         # 清理后重新配置编译
#>
[CmdletBinding()]
param(
    [switch]$Configure,
    [switch]$Clean,
    [ValidateSet('Debug', 'Release')]
    [string]$Config = 'Debug'
)

$ErrorActionPreference = 'Stop'

$root      = Split-Path -Parent $PSScriptRoot
$buildDir  = Join-Path $root 'build'

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "清理 $buildDir ..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

if ($Clean -or $Configure -or -not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))) {
    Write-Host "配置 CMake ($Config) ..." -ForegroundColor Cyan
    if (-not $env:VULKAN_SDK) {
        Write-Warning '环境变量 VULKAN_SDK 为空！请确认已安装 Vulkan SDK 并重启终端。'
    }
    cmake -S $root -B $buildDir -G "Visual Studio 17 2022" -A x64
    if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败 (exit $LASTEXITCODE)" }
}

Write-Host "编译 ($Config) ..." -ForegroundColor Cyan
cmake --build $buildDir --config $Config
if ($LASTEXITCODE -ne 0) { throw "编译失败 (exit $LASTEXITCODE)" }

$exe = Join-Path $buildDir "$Config\learn_vulkan.exe"
Write-Host ""
Write-Host "编译成功: $exe" -ForegroundColor Green
Write-Host "运行:     .\scripts\run.ps1 -Config $Config" -ForegroundColor Green
