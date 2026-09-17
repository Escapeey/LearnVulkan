<#
.SYNOPSIS
    运行 LearnVulkan 的某个目标。

.DESCRIPTION
    learn_vulkan  教程主程序（D2 起跟着章节长大）      —— 默认
    env_check     D1 环境自检（换机器/升级驱动后可重跑）

.EXAMPLE
    .\scripts\run.ps1
    .\scripts\run.ps1 -Target env_check
    .\scripts\run.ps1 -Config Release
#>
[CmdletBinding()]
param(
    [ValidateSet('learn_vulkan', 'env_check')]
    [string]$Target = 'learn_vulkan',

    [ValidateSet('Debug', 'Release')]
    [string]$Config = 'Debug'
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$exe  = Join-Path $root "build\$Config\$Target.exe"

if (-not (Test-Path $exe)) {
    throw "找不到 $exe`n请先运行: .\scripts\build.ps1"
}

Write-Host "运行 $exe" -ForegroundColor Cyan
Write-Host ("-" * 60)
& $exe
$code = $LASTEXITCODE
Write-Host ("-" * 60)
Write-Host "退出码: $code" -ForegroundColor $(if ($code -eq 0) { 'Green' } else { 'Red' })
exit $code
