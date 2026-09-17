# D1：环境搭建

你的机器现状（我已探测，2024 实测）：

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

**结论：需要你自己执行两件事（都要 UAC 管理员权限，我当前会话没有）。**

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
