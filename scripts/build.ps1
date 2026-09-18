<#
.SYNOPSIS
    配置并编译 LearnVulkan。

.DESCRIPTION
    编译出两个目标：
      learn_vulkan  教程主程序（D2 起跟着章节长大）
      env_check     D1 环境自检

    会自动探测工具链，不需要你手动配 PATH：
      - Visual Studio 自带的 CMake 不在 PATH 上（在 Common7\IDE\CommonExtensions\...），
        所以除了 PATH 还会去 Visual Studio 安装目录里找。
      - CMake 生成器按已安装的最新 Visual Studio 自动选择，
        所以 VS 2022 / VS 2026（18）/ Build Tools 都能直接用。

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

    # 可选：手动指定 CMake 生成器（默认自动探测最新 Visual Studio）
    [string]$Generator,

    # 可选：覆盖依赖仓库地址（国内镜像）
    [string]$GlfwUrl,
    [string]$GlmUrl
)

# ⚠️ 这里【不能】用 $ErrorActionPreference = 'Stop'。
#
# PowerShell 5.1 会把「原生程序写到 stderr 的任何内容」包装成 ErrorRecord。
# 在 Stop 模式下，第一条这样的记录就会终止整个脚本 —— 而 CMake 和 MSBuild
# 在【完全正常】工作时也大量使用 stderr（编译器的 deprecation warning、
# MSBuild 的版本横幅都在 stderr）。用 Stop 会导致脚本在无害的警告处自杀。
#
# 所以这里保持默认的 Continue，并靠 $LASTEXITCODE 显式判断每一步的成败。
$ErrorActionPreference = 'Continue'

$root     = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root 'build'

# ---------------------------------------------------------------------------
# 工具探测
# ---------------------------------------------------------------------------
function Find-CMake {
    $onPath = Get-Command cmake -ErrorAction SilentlyContinue
    if ($onPath) { return $onPath.Source }

    # VS 自带的 CMake 不在 PATH 上，去安装目录里翻
    $roots = @()
    if ($env:ProgramFiles)          { $roots += (Join-Path $env:ProgramFiles 'Microsoft Visual Studio') }
    if (${env:ProgramFiles(x86)})   { $roots += (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio') }

    $hits = @()
    foreach ($r in $roots) {
        if (-not (Test-Path $r)) { continue }
        $hits += Get-ChildItem -Path $r -Filter 'cmake.exe' -Recurse -ErrorAction SilentlyContinue |
                 Where-Object { $_.FullName -match 'CommonExtensions\\Microsoft\\CMake' }
    }
    if ($hits.Count -gt 0) {
        # 目录名里 \18\ 比 \17\ 新，倒序取第一个
        return ($hits | Sort-Object FullName -Descending | Select-Object -First 1).FullName
    }
    return $null
}

function Find-VSGenerator {
    param([string]$CMakePath)
    $help = (& $CMakePath --help 2>&1 | Out-String)
    $gen = [regex]::Matches($help, 'Visual Studio (\d+) (\d{4})') |
           ForEach-Object {
               [pscustomobject]@{
                   Major = [int]$_.Groups[1].Value
                   Name  = "Visual Studio $($_.Groups[1].Value) $($_.Groups[2].Value)"
               }
           } |
           Sort-Object Major -Descending |
           Select-Object -First 1
    if ($gen) { return $gen.Name }
    return $null
}

# ---------------------------------------------------------------------------
# 环境预检
# ---------------------------------------------------------------------------
$problems = @()

$cmakeExe = Find-CMake
if (-not $cmakeExe) {
    $problems += '找不到 cmake。装 Visual Studio 时请勾选「适用于 Windows 的 C++ CMake 工具」，装完关掉所有终端重开。'
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

$cmakeVersion = (& $cmakeExe --version 2>&1 | Select-Object -First 1)
$sdkPath      = if ($env:VULKAN_SDK) { $env:VULKAN_SDK } else { $env:VK_SDK_PATH }

if (-not $Generator) { $Generator = Find-VSGenerator -CMakePath $cmakeExe }

Write-Host '环境预检通过：' -ForegroundColor Green
Write-Host "  Vulkan SDK : $sdkPath"
Write-Host "  CMake      : $cmakeVersion"
Write-Host "               $cmakeExe"
if ($Generator) {
    Write-Host "  生成器     : $Generator"
} else {
    Write-Host '  生成器     : (未探测到 Visual Studio，交给 CMake 自选默认)' -ForegroundColor Yellow
}

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
    Remove-Item -Recurse -Force $buildDir -ErrorAction Stop
}

$needConfigure = $Clean -or $Configure -or -not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))

if ($needConfigure) {
    Write-Host "配置 CMake ($Config) ..." -ForegroundColor Cyan

    $cmakeArgs = @('-S', $root, '-B', $buildDir)
    if ($Generator) {
        $cmakeArgs += @('-G', $Generator, '-A', 'x64')
    }
    if ($GlfwUrl) { $cmakeArgs += "-DGLFW_GIT_URL=$GlfwUrl" }
    if ($GlmUrl)  { $cmakeArgs += "-DGLM_GIT_URL=$GlmUrl" }

    # 2>&1 把 stderr 并进 stdout，避免 CMake 的警告被 PowerShell 当成错误记录刷红字
    & $cmakeExe @cmakeArgs 2>&1 | ForEach-Object { Write-Host $_ }
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
& $cmakeExe --build $buildDir --config $Config 2>&1 | ForEach-Object { Write-Host $_ }
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
