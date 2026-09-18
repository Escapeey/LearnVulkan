<#
.SYNOPSIS
    运行 LearnVulkan 的某个目标。

.DESCRIPTION
    learn_vulkan  教程主程序（D2 起跟着章节长大）      —— 默认
    env_check     D1 环境自检（换机器/升级驱动后可重跑）

    会依次在下面这些位置找 exe：
      1) build\<Config>\<Target>.exe    ← .\scripts\build.ps1 的产物（权威）
      2) build\<Target>.exe             ← 单配置生成器（Ninja）的情况
      3) out\build\*\...                ← Visual Studio 自己配置出来的那棵树
    找到哪个就用哪个，并打印实际用的是哪一个。

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

$ErrorActionPreference = 'Continue'

$root = Split-Path -Parent $PSScriptRoot

# ---------------------------------------------------------------------------
# 按优先级收集候选路径
# ---------------------------------------------------------------------------
$candidates = @(
    (Join-Path $root "build\$Config\$Target.exe"),   # build.ps1 的产物
    (Join-Path $root "build\$Target.exe")            # 单配置生成器
)

# Visual Studio 打开本工程时会自己配置到 out\build\<preset>\，把那些也算上
$vsRoot = Join-Path $root 'out\build'
if (Test-Path -LiteralPath $vsRoot) {
    foreach ($d in (Get-ChildItem -LiteralPath $vsRoot -Directory -ErrorAction SilentlyContinue)) {
        $candidates += (Join-Path $d.FullName "$Target.exe")
        $candidates += (Join-Path $d.FullName "$Config\$Target.exe")
    }
}

$exe = $candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1

if (-not $exe) {
    Write-Host ''
    Write-Host "找不到 $Target.exe，已查找以下位置：" -ForegroundColor Red
    foreach ($c in $candidates) { Write-Host "  $c" -ForegroundColor DarkGray }
    Write-Host ''
    Write-Host '请先构建： .\scripts\build.ps1' -ForegroundColor Yellow
    exit 1
}

# ---------------------------------------------------------------------------
# 提醒：如果用的是 VS 那棵树，说明 build.ps1 还没跑过或产物被删了
# ---------------------------------------------------------------------------
if ($exe -match '\\out\\build\\') {
    Write-Host '注意：这次用的是 Visual Studio 自己那套构建树（out\build\），不是 build.ps1 的产物。' -ForegroundColor Yellow
    Write-Host '      想用命令行这套，先跑： .\scripts\build.ps1' -ForegroundColor Yellow
    Write-Host ''
}

Write-Host "运行 $exe" -ForegroundColor Cyan
Write-Host ('-' * 60)
& $exe
$code = $LASTEXITCODE
Write-Host ('-' * 60)
Write-Host "退出码: $code" -ForegroundColor $(if ($code -eq 0) { 'Green' } else { 'Red' })
exit $code
