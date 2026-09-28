# LearnVulkan

跟着 https://tutorial.vulkan.net.cn/ （Overv VulkanTutorial 中文版）从 OpenGL 转 Vulkan 的学习工程。

**终点**：一个能加载 OBJ 模型、带纹理 / 深度 / Mipmap / MSAA 的 Vulkan 渲染器，并且你能解释每一步"在 OpenGL 里是谁做的"。

---

## 从这里开始

| 顺序 | 文件 | 作用 |
|---|---|---|
| 1 | [`ROADMAP.md`](ROADMAP.md) | **学习计划**：5 阶段 24 单元、里程碑、每个单元干什么 |
| 2 | [`SETUP.md`](SETUP.md) | **环境搭建**：D1 要你亲手做的事（装 VS + Vulkan SDK） |
| 3 | [`docs/opengl-to-vulkan.md`](docs/opengl-to-vulkan.md) | **对照词典**：OpenGL → Vulkan 职责映射 + 三张必画图 |
| 4 | [`PROGRESS.md`](PROGRESS.md) | **打卡**：进度 + 3 行总结 + 踩坑记录 |

---

## 快速开始（环境装好之后）

```powershell
cd LearnVulkan

.\scripts\build.ps1                   # 配置 + 编译（依赖已在 third_party\，不需要联网）
.\scripts\run.ps1 -Target env_check   # 环境自检（换机器/升级驱动后可重跑）
.\scripts\run.ps1                     # 教程主程序（D2 实现完之后才有画面）
```

编译出两个目标：

| 目标 | 文件 | 用途 |
|---|---|---|
| `env_check` | `src/env_check.cpp` | D1 环境自检（换机器/升级驱动后可重跑） |
| `learn_vulkan` | `src/hello_triangle.cpp` | **教程主程序**，跟着章节一路长大 |

> ## ✅ 当前进度：**D10 已完成**，进入 D11（命令缓冲 + 渲染与呈现）
>
> 环境：VS **18 (2026)** + Vulkan SDK **1.4.357.0**；GLFW 3.4 / GLM 1.0.1 在本地 `third_party/`；
> `env_check.exe` 全绿（loader 1.4.357 / 13 实例扩展 / 9 层，含 `VK_LAYER_KHRONOS_validation`）。
>
> ### 👉 下一步：做 **D11**（命令缓冲 + 渲染与呈现 —— 第一次出图 🎉）
> 1. 读讲义 **[`docs/ch11-command-buffers-and-presentation.md`](docs/ch11-command-buffers-and-presentation.md)**（动手前先读）
> 2. 教程对应章节 Command_buffers、Rendering_and_presentation
> 3. 写 `createCommandPool()` / `createCommandBuffer()` / `createSyncObjects()`
> 4. 写 `recordCommandBuffer()`（begin render pass → bind → draw → end render pass）+ `drawFrame()`（acquire → submit → present）
> 5. `mainLoop()` 每帧调 `drawFrame()`、末尾 `vkDeviceWaitIdle`；`cleanup()` 加 2 个 semaphore + 1 个 fence + commandPool 的销毁
> 6. `.\scripts\build.ps1` → `.\scripts\run.ps1`
> 7. 卡住就把**编译报错原文** + 你的代码贴给我（别只说"报错了"）
>
> 🎉 D11 结束时**彩色三角形第一次出现在屏幕上**——黑底 RGB 三角形。
>
> **已完成 / 已备好的讲义与自检题**：
> [D2 实例与验证层](docs/ch02-instance-and-validation.md) ·
> [D3 物理设备与队列](docs/ch03-physical-device-and-queues.md) ·
> [D4 窗口表面与交换链探测](docs/ch04-window-surface-and-swapchain-probe.md) ·
> [D5 交换链与图像视图](docs/ch05-swapchain-and-image-views.md) ·
> [D6 着色器模块](docs/ch06-shader-modules.md) ·
> [D7 固定功能·上](docs/ch07-fixed-functions-1.md) ·
> [D8 固定功能·下](docs/ch08-fixed-functions-2.md) ·
> [D9 渲染通道](docs/ch09-render-pass.md) ·
> [D10 帧缓冲 + 图形管线](docs/ch10-framebuffers-and-pipeline.md) ·
> [D11 命令缓冲 + 呈现](docs/ch11-command-buffers-and-presentation.md)
>
> 各单元的 `// TODO(chNN)` 骨架随进度逐单元补齐：做完当前单元、我 review 过实现后，再补下一单元的标记，
> 免得 `src/hello_triangle.cpp` 里堆满还没轮到你的标记，看不清当前该写什么。

---

## 构建方式：命令行（已选定方案 A）

**权威构建走命令行**，Visual Studio 只当代码编辑器用（看代码、跳转、补全）。

```powershell
.\scripts\build.ps1                       # 配置 + 编译 → 产物在 build\Debug\
.\scripts\run.ps1                         # 跑教程主程序
.\scripts\run.ps1 -Target env_check       # 跑 D1 环境自检
```

### ⚠️ 会有两套构建目录，别搞混

| 目录 | 谁生成的 | 生成器 | 用途 |
|---|---|---|---|
| **`build\`** | **`scripts\build.ps1`** | Visual Studio 18 2026（MSBuild） | **权威** —— exe 在 `build\Debug\` |
| `out\build\x64-Debug\` | Visual Studio 自己 | Ninja | 只喂给 VS 的 IntelliSense，**不要在这里构建** |

VS 打开这个文件夹时会自动在 `out\build\` 里配一套（这样跳转/补全才能工作），**不影响**命令行那套。
两者互不干扰，也都被 `.gitignore` 挡住了。

**规则**：

- ✅ 要编译、要运行 → `.\scripts\build.ps1` / `.\scripts\run.ps1`
- ❌ 不要在 VS 里按 F5 / "生成" —— exe 会落到 `out\build\`，和命令行那套脱节
- 🖊️ VS 里就当编辑器：看代码、`Ctrl+点击` 跳转、看 IntelliSense 报错

> `run.ps1` 会**两个目录都找**。万一用的是 VS 那棵树，它会黄字提醒你。
> 另外 `out\` 目录可以随时删掉（VS 会重建），不影响命令行构建。

---

## 目录结构

```
LearnVulkan/
├─ ROADMAP.md              学习计划（主文档）
├─ SETUP.md                D1 环境搭建
├─ PROGRESS.md             进度打卡 + 踩坑记录
├─ CMakeLists.txt          构建入口
├─ cmake/Shaders.cmake     GLSL -> SPIR-V 规则
├─ docs/
│   ├─ opengl-to-vulkan.md   OpenGL -> Vulkan 职责对照词典（建议打印）
│   ├─ chNN-<主题>.md        每个单元的**讲义**（D2–D11 已备）
│   ├─ chNN-check.md         每个单元的**自检题**（含折叠提示与答题记录表）
│   ├─ ch02-steps.md         D2 的**分步实现指南**（5 个检查点，边写边验证）
│   ├─ your-gpu.md          **你这台机器的实测硬件数据**（各单元都要对照）
│   └─ verified.md          **已实测验证 vs 未验证清单**（该信任到什么程度）
├─ qa/
│   ├─ tooling.md            构建/运行工具问答（build.ps1 vs run.ps1）
│   └─ d2.md                 D2 概念问答（实例/验证层/调试信使/pNext）
├─ src/
│   ├─ hello_triangle.cpp  教程主程序（当前正在写的代码）
│   └─ env_check.cpp       D1 环境自检工具
├─ shaders/                GLSL 源码
├─ scripts/                build.ps1 / run.ps1
├─ third_party/            GLFW / GLM 本地源码（可选，见其 README）
└─ build/                  构建产物（已 gitignore）
```

---

## 工作方式：我搭骨架，你填核心

每个学习单元：

1. **我给**：`docs/chNN-*.md` 讲义（概念 + OpenGL 对照 + 参数为什么长这样 + 坑）
2. **你读**：教程对应章节，**先别看代码清单**
3. **你填**：我在 `src/` 里留好结构体和函数签名，核心逻辑标着 `// TODO(chNN)`
4. **你跑**：`.\scripts\build.ps1` → `.\scripts\run.ps1`，改一次跑一次
5. **卡住**：把**验证层原文** + 你的代码贴给我（不要只说"报错了"）
6. **通过**：我 review 你的实现，你写 3 行总结进 `PROGRESS.md`，`git tag` 存档

### 与教程的差异（本工程有意偏离的地方）

教程代码写于几年前，用的依赖版本和构建方式跟现在不完全一样。凡是本工程**故意**和教程不同的地方，都会记在这里，并说明原因 —— 遇到"Hmm 教程不是这么写的"时先查这张表。

| 位置 | 教程写法 | 本工程 | 原因 |
|---|---|---|---|
| GLM 初始化 | `#define GLM_FORCE_RADIANS` + `GLM_FORCE_DEPTH_ZERO_TO_ONE` | **只保留** `GLM_FORCE_DEPTH_ZERO_TO_ONE` | `GLM_FORCE_RADIANS` 自 GLM 0.9.9 起已是空操作，1.0 直接移除了该宏。留着无用，还可能触发编译期报错 |
| 构建方式 | Visual Studio `.vcxproj` 手动配置包含目录/库目录 | CMake + `FetchContent` | 命令行可验证，我能直接帮你编译排错；也免去手点一堆属性页 |
| 依赖获取 | 手动下载 GLFW/GLM 压缩包 | CMake 自动拉取，失败时可切本地目录/镜像 | GitHub 国内不稳定，见 `third_party/README.md` |
| 入口文件 | 每章从头写一个 `.cpp` | 一个 `src/hello_triangle.cpp` 一路长大 | 便于逐步 review 你的实现、也便于 `git diff` 看清每章改了什么 |
| 环境自检 | 无 | 额外提供 `src/env_check.cpp` | 换机器/升级驱动后可重跑，快速确认环境没变 |

### 里程碑

| 里程碑 | 内容 | 对应单元 |
|---|---|---|
| M0 | 环境自检全绿 | D1 |
| **M1** | **彩色三角形 + 缩放不崩 + 验证层零 error** | D2–D12 |
| M2 | 旋转的透视正确四边形（MVP + UBO） | D13–D16 |
| M3 | 贴图正确显示 | D17–D18 |
| **M4** | **带纹理的 OBJ 模型，深度遮挡正确** | D19–D20 |
| M5 | Mipmap + MSAA + 计算着色器全部跑通 | D21–D24 |

---

## 你的机器（已探测）

| 项目 | 值 |
|---|---|
| 显卡 | Intel(R) UHD Graphics 630（集成） |
| Vulkan | 1.3.215（conformance 1.3.1.1）→ 教程全章节可跑 |
| 待装 | Visual Studio 2022 + Vulkan SDK 1.4.x（见 `SETUP.md`） |

> 集显性能有限，但教程所有内容（含计算着色器）都在能力范围内。
> 如果卡顿，把窗口调小（`800x600` 已经够用）、关掉其他占显存的程序。

---

## 许可

- 本仓库的学习笔记、讲义、代码骨架采用 [MIT License](LICENSE)。
- 依赖库（GLFW、GLM）各自保留其原有许可证，见 `third_party/` 内对应源码。
