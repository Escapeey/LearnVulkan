<#
.SYNOPSIS
    运行 LearnVulkan。

.EXAMPLE
    .\scripts\run.ps1
    .\scripts\run.ps1 -Config Release
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Config = 'Debug'
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$exe  = Join-Path $root "build\$Config\learn_vulkan.exe"

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
