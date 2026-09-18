# D1：环境搭建

> ## ✅ 状态：已完成并实测通过
>
> 你的环境已装好，而且完整构建链路（配置 → 编译 GLFW/GLM → 编译着色器 → 运行）**已实际跑通**。
> 以下是实测结果，不是预期：
>
> | 组件 | 实际版本 / 位置 |
> |---|---|
> | Visual Studio | **18 Community（2026）**，MSVC 14.51.36231 + Windows SDK 10.0.26100 |
> | Vulkan SDK | **1.4.357.0**（`C:\VulkanSDK\1.4.357.0`） |
> | CMake | **4.3.1**（VS 自带，不在 PATH；`build.ps1` 会自动找到） |
> | 依赖 | GLFW 3.4 + GLM 1.0.1，已克隆到本地 `third_party/` |
> | 自检 | `env_check.exe` → **结论：全部通过 ✅** |
>
> **本页以下内容保留作参考**：换机器、重装环境、或以后出问题时回来查。
> 你现在可以直接进入 **D2**（讲义 `docs/ch02-instance-and-validation.md`，骨架 `src/hello_triangle.cpp`）。

你的机器现状（我实测的）：

| 项目 | 状态 |
|---|---|
| 显卡 | **Intel(R) UHD Graphics 630**（集成显卡） |
| Vulkan 版本 | **1.3.215**，conformance 1.3.1.1 → 教程全部章节可跑 |
| Vulkan loader | ✅ 已在 `C:\WINDOWS\System32\vulkan-1.dll`（驱动自带） |
| 验证层 | ❌ 未安装（`vulkaninfo` 显示 layers count = 0）—— **必须装 SDK** |
| C++ 编译器 | ❌ 无（无 VS / MSVC / clang / mingw） |
| CMake | ❌ 无 |
| Python | ⚠️ 只有 Microsoft Store 占位程序（0 字节），**不可用** |
| Git | ✅ 2.55.0 |

**结论：需要你自己执行两件事（都要 UAC 提权，我当前会话没有）。**

> ### 关于提权：已确认你可以
>
> 我查了你的账户令牌，结果是：
>
> ```
> BUILTIN\Administrators   Alias   S-1-5-32-544   Group used for deny only
> ```
>
> `Group used for deny only` 是 **UAC 筛选令牌**的典型特征 —— 意思是**你的账户确实是管理员**，
> 只是当前进程运行在"非提权"状态下。所以：
>
> - 双击安装程序时，Windows 会弹出 UAC 确认框，点"是"即可
> - 如果没弹框就直接失败了，改成**右键 → 以管理员身份运行**
> - 我（AI）在沙箱里跑的命令拿不到这个提权，所以这两步必须你亲手做
>
> 如果 UAC 弹框要求输入**管理员密码**而不是点"是"，说明你的账户其实不是管理员组，
> 那需要找 IT 或有管理员权限的同事协助 —— 遇到这种情况告诉我，我们换免安装的便携方案。

---

## 步骤 1：安装 Visual Studio 2022 Community

装 VS 的理由不是"要 IDE"，而是它是 Windows 上最省事的 MSVC + CMake + Windows SDK 一体包，且教程默认方案就是它。

1. 打开 https://visualstudio.microsoft.com/zh-hans/vs/community/ 下载安装程序
2. 运行，在工作负载页**只勾选**：
   - ✅ **使用 C++ 的桌面开发**（Desktop development with C++）
3. 右侧"安装详细信息"里确认勾上了（通常默认就有）：
   - ✅ MSVC v143 - VS 2022 C++ x64/x86 生成工具
   - ✅ Windows 11 SDK
   - ✅ **适用于 Windows 的 C++ CMake 工具**（这一项很关键，我们要用 CMake）
4. 语言包保持默认（中文即可），安装位置默认（`C:\Program Files\Microsoft Visual Studio\2022\Community`）
5. 安装体积约 **8–12 GB**，预留 20 GB 空间

> 如果你更想要轻量方案（不需要 IDE 调试器）：装 **Visual Studio 2022 生成工具**（Build Tools），同样勾"使用 C++ 的桌面开发"+"CMake 工具"。省约 3 GB。但 VS Community 的图形化调试器对你后面排查 Vulkan 崩溃很有价值，我建议装完整版。

**命令行方式**（在**管理员** PowerShell 里跑，效果等价）：

```powershell
winget install --id Microsoft.VisualStudio.2022.Community --accept-package-agreements --accept-source-agreements --override "--quiet --wait --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"
```

---

## 步骤 2：安装 Vulkan SDK

当前最新为 **1.4.321.0**（2024 系列已到 1.4.x）。**必须装这个**，因为：

- 提供 `vulkan/vulkan.h` 头文件和 `vulkan-1.lib` 导入库（没有它链接不过）
- 提供 **`glslc.exe`**（GLSL → SPIR-V 编译器）
- **提供验证层 `VkLayer_khronos_validation.dll`** ← 这是你学习期间最重要的调试工具
- 提供 `vkcube.exe`、`vulkaninfo.exe` 用于自检

1. 打开 https://vulkan.lunarg.com/sdk/home#windows
2. 下载 **Windows 最新版 Installer**（形如 `VulkanSDK-1.4.321.0-Installer.exe`）
3. 安装时注意：
   - 安装路径保持默认 `C:\VulkanSDK\1.4.321.0`（**不要**装到带空格/中文的路径）
   - 安装向导会问是否设置环境变量 → **全部勾选**（`VULKAN_SDK`、`PATH`、`VK_LAYER_PATH` 等）
4. 装完后**关掉所有终端重新开**（环境变量才生效）

**命令行方式**（管理员 PowerShell）：

```powershell
winget install --id KhronosGroup.VulkanSDK --accept-package-agreements --accept-source-agreements
```

---

## 步骤 3：验证环境

**新开一个 PowerShell**，逐条执行，确认输出：

```powershell
# 1. SDK 环境变量
echo $env:VULKAN_SDK
# 期望：C:\VulkanSDK\1.4.321.0

# 2. 验证层现在应该能枚举出来了（之前是 0）
vulkaninfo --summary
# 期望：Devices 下仍是 Intel(R) UHD Graphics 630；
#       并且不再出现 "Registry lookup failed to get layer manifest files"

# 3. 编译器
cl
# 期望：报 "Microsoft (R) C/C++ 优化编译器" 版本信息（这个"报错"是正常的，说明找到了）

# 4. CMake
cmake --version
# 期望：cmake version 3.2x 或更高

# 5. glslc
glslc --version
# 期望：shaderc v2024.x ...

# 6. 驱动自检demo
& "$env:VULKAN_SDK\Bin\vkcube.exe"
# 期望：弹出一个旋转的立方体窗口
```

**六条全部通过才算 D1 完成。** 任何一条失败，把命令原文和输出贴给我。

### 常见失败

| 现象 | 原因 | 解决 |
|---|---|---|
| `$env:VULKAN_SDK` 为空 | 终端没重启 | 关掉所有终端重开；或注销重登 |
| `cl` 找不到 | 在普通 PowerShell 里跑，没进 VS 开发者环境 | 用"开始菜单 → x64 Native Tools Command Prompt for VS 2022"；或直接跳过这条，后面用 CMake（CMake 会自动找 MSVC） |
| `cmake` 找不到 | 装 VS 时没勾 CMake 工具 | 打开 VS Installer → 修改 → 勾"适用于 Windows 的 C++ CMake 工具" |
| `vkcube` 报错找不到设备 | 驱动太旧 | 到 Intel 官网更新显卡驱动 |
| 验证层仍为 0 | SDK 装了但环境变量没设 | 检查 `$env:VK_LAYER_PATH` 是否指向 `C:\VulkanSDK\<ver>\Bin` |

---

## 已为你排查：教程官方 FAQ 里的 Windows 专属坑

我按[教程官方 FAQ](https://tutorial.vulkan.net.cn/FAQ) 逐条查了你的机器，**结论是你目前是干净的**：

| 教程 FAQ 条目 | 你的机器 | 影响 |
|---|---|---|
| MSI Afterburner / RivaTuner Statistics Server 导致**核心验证层访问冲突** | ✅ 未安装、未运行 | 这是 FAQ 第一条。如果装了，D2–D11 会随机崩在验证层内部，报 access violation，**极难定位到是它**。以后若装了，跑 Vulkan 前先退出它 |
| Steam 覆盖层 `VK_LAYER_VALVE_steam_overlay` 让 `vkCreateSwapchainKHR` 报错 | ✅ 未安装 Steam | 注册表 `HKLM\SOFTWARE\Khronos\Vulkan\ImplicitLayers` 当前**不存在**，即没有任何第三方层会注入你的程序 |
| 看不到任何验证层消息（程序一闪而过） | ⚠️ 要注意习惯 | 官方建议：VS 里用 **Ctrl-F5** 而不是 F5（Ctrl-F5 保留终端窗口）。用本工程的 `run.ps1` 没这问题，它把输出留在当前窗口并打印退出码 |
| `vkCreateInstance` 返回 `VK_ERROR_INCOMPATIBLE_DRIVER` | ➖ 不适用 | 这是 macOS + MoltenVK 特有的，Windows 遇不到 |
| SDK 版本须 ≥ 1.1.106 才有 `VK_LAYER_KHRONOS_validation` | ✅ 满足 | 我们会装 1.4.x |

**以后想复查这项**（比如你装了 Afterburner 或 Steam 之后），在 PowerShell 里跑：

```powershell
# 有没有冲突进程在跑
Get-Process | Where-Object { $_.ProcessName -match 'Afterburner|RTSS|RivaTuner|steam' }

# 系统里注册了哪些会注入到每个 Vulkan 程序的隐式层
Get-Item 'HKLM:\SOFTWARE\Khronos\Vulkan\ImplicitLayers' -ErrorAction SilentlyContinue |
    Select-Object -ExpandProperty Property
```

第二条如果打印出东西来，说明有第三方层在全局注入 —— 遇到莫名其妙的崩溃时，这是第一个要怀疑的对象。

---

## 步骤 4：跑通冒烟测试

工程骨架已经搭好（`CMakeLists.txt` + `src/env_check.cpp` + `scripts/`），自检程序是教程"开发环境"章节那个测试程序的加强版。

它编译出两个目标，D1 只关心 `env_check`：

| 目标 | 文件 | 用途 |
|---|---|---|
| `env_check` | `src/env_check.cpp` | **D1 环境自检**（本页要跑的） |
| `learn_vulkan` | `src/hello_triangle.cpp` | 教程主程序，D2 起才开始写 |

```powershell
cd C:\Users\d00944037\Code\LearnVulkan

# 配置（首次会联网拉取 GLFW 和 GLM 源码，约 1-2 分钟）
.\scripts\build.ps1 -Configure

# 编译（两个目标一起编）
.\scripts\build.ps1

# 运行 D1 环境自检
.\scripts\run.ps1 -Target env_check
```

> ### ⚠️ 如果 `-Configure` 卡住或报 Git 错误
>
> 首次配置要从 GitHub 拉取 GLFW 和 GLM。**国内网络下这一步经常失败**（卡在 `Cloning into ...`，
> 或报 `fatal: unable to access ... schannel: ...`）。
>
> **不要反复重试**，直接看 [`third_party/README.md`](third_party/README.md)，里面三条路：
>
> 1. 给 git 配代理（最省事）
> 2. 换镜像源：`.\scripts\build.ps1 -Configure -GlfwUrl <镜像> -GlmUrl <镜像>`
> 3. 手动下载 ZIP 解压到 `third_party/glfw` 和 `third_party/glm`（最可靠）
>
> `build.ps1` 现在会在配置前打印它对每个依赖打算用哪种方式 ——
> 看到 `依赖 glfw : 本地源码 third_party\glfw` 就说明本地方式生效了。
>
> **另外**：`build.ps1` 现在会先做环境预检。如果 `cmake` 或 `VULKAN_SDK` 缺失，
> 它会直接红字告诉你缺什么、去哪儿补，而不是抛一屏 CMake 报错。

**期望输出**（窗口弹出，控制台打印类似）：

```
Vulkan 实例扩展数量: 13
GLFW 版本: 3.4.0
GLM 测试通过: mat4 * vec4 =
[1, 0, 0, 0]
...
按 ESC 或关闭窗口退出
```

控制台里 `Vulkan 实例扩展数量` **必须非零**。窗口能正常关闭、程序返回码 0，就算通过。

> 注意：**此时还没有验证层**，所以看不到验证层输出，这是正常的。D2 写了 debug messenger 之后才会开始有东西可看。

---

## 步骤 5：读两页教程建立全局图景

在写任何代码前，先读：

- https://tutorial.vulkan.net.cn/Overview
- https://tutorial.vulkan.net.cn/Drawing_a_triangle/Graphics_pipeline_basics/Introduction

**带着一个问题读**：「从"我要画一个三角形"到"屏幕上出现像素"，中间有哪 10 个对象必须先存在？」
读完在白纸上把这 10 个对象和它们的依赖顺序画出来。画完拍给你自己看，D2 开始前我们对照。

---

完成 D1 后，在 `PROGRESS.md` 里打勾，然后我们开始 D2（Instance + 验证层）。
