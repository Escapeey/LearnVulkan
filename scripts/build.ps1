<#
.SYNOPSIS
    配置并编译 LearnVulkan。

.DESCRIPTION
    编译出两个目标：
      learn_vulkan  教程主程序（D2 起跟着章节长大）
      env_check     D1 环境自检

    配置前会做环境预检，缺少 cmake 或 Vulkan SDK 时直接给出可操作的提示，
    而不是让你对着一屏 CMake 报错猜。

.EXAMPLE
    .\scripts\build.ps1 -Configure          # 首次：配置（会拉取 GLFW/GLM）
    .\scripts\build.ps1                     # 之后：增量编译
    .\scripts\build.ps1 -Clean              # 清理并重新配置编译
    .\scripts\build.ps1 -Config Release     # 发布配置

.EXAMPLE
    # GitHub 拉不动时换镜像（详见 third_party/README.md）
    .\scripts\build.ps1 -Configure -GlfwUrl "https://gitee.com/xxx/glfw.git" `
                                   -GlmUrl  "https://gitee.com/xxx/glm.git"
#>
[CmdletBinding()]
param(
    [switch]$Configure,
    [switch]$Clean,

    [ValidateSet('Debug', 'Release')]
    [string]$Config = 'Debug',

    # 可选：覆盖依赖仓库地址（国内镜像）
    [string]$GlfwUrl,
    [string]$GlmUrl
)

$ErrorActionPreference = 'Stop'

$root     = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root 'build'

# ---------------------------------------------------------------------------
# 环境预检
# ---------------------------------------------------------------------------
$problems = @()

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    $problems += '找不到 cmake。装 Visual Studio 2022 时请勾选「适用于 Windows 的 C++ CMake 工具」，装完关掉所有终端重开。'
}

if (-not $env:VULKAN_SDK -and -not $env:VK_SDK_PATH) {
    $problems += '环境变量 VULKAN_SDK 为空。请安装 Vulkan SDK，然后关掉【所有】终端重新打开（只重开当前窗口有时不生效）。'
}

if ($problems.Count -gt 0) {
    Write-Host ''
    Write-Host '环境预检未通过：' -ForegroundColor Red
    foreach ($p in $problems) { Write-Host "  [X] $p" -ForegroundColor Red }
    Write-Host ''
    Write-Host '排查步骤见 SETUP.md「常见失败」表。' -ForegroundColor Yellow
    exit 1
}

Write-Host "环境预检通过：VULKAN_SDK = $env:VULKAN_SDK" -ForegroundColor Green

# 依赖来源提示（本地源码 / 网络拉取）
foreach ($dep in @('glfw', 'glm')) {
    if (Test-Path (Join-Path $root "third_party\$dep\CMakeLists.txt")) {
        Write-Host "  依赖 $dep : 本地源码 third_party\$dep" -ForegroundColor Green
    } else {
        Write-Host "  依赖 $dep : 将从 GitHub 拉取（拉不动见 third_party\README.md）" -ForegroundColor DarkGray
    }
}

# ---------------------------------------------------------------------------
# 配置
# ---------------------------------------------------------------------------
if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "清理 $buildDir ..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

$needConfigure = $Clean -or $Configure -or -not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))

if ($needConfigure) {
    Write-Host "配置 CMake ($Config) ..." -ForegroundColor Cyan

    $cmakeArgs = @('-S', $root, '-B', $buildDir, '-G', 'Visual Studio 17 2022', '-A', 'x64')
    if ($GlfwUrl) { $cmakeArgs += "-DGLFW_GIT_URL=$GlfwUrl" }
    if ($GlmUrl)  { $cmakeArgs += "-DGLM_GIT_URL=$GlmUrl" }

    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Host ''
        Write-Host 'CMake 配置失败。' -ForegroundColor Red
        Write-Host '如果是 Git 拉取依赖超时/被拒绝，请看 third_party\README.md 里的三种替代方案。' -ForegroundColor Yellow
        exit $LASTEXITCODE
    }
}

# ---------------------------------------------------------------------------
# 编译
# ---------------------------------------------------------------------------
Write-Host "编译 ($Config) ..." -ForegroundColor Cyan
& cmake --build $buildDir --config $Config
if ($LASTEXITCODE -ne 0) {
    Write-Host ''
    Write-Host '编译失败。把上面的报错原文贴给我。' -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host ''
Write-Host '编译成功。可执行文件：' -ForegroundColor Green
Write-Host "  $buildDir\$Config\learn_vulkan.exe   (教程主程序)" -ForegroundColor Green
Write-Host "  $buildDir\$Config\env_check.exe      (D1 环境自检)" -ForegroundColor Green
Write-Host ''
Write-Host '运行:  .\scripts\run.ps1                       # 教程主程序' -ForegroundColor Green
Write-Host '       .\scripts\run.ps1 -Target env_check     # 环境自检' -ForegroundColor Green
