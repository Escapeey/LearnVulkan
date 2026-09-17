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
cd C:\Users\d00944037\Code\LearnVulkan

.\scripts\build.ps1 -Configure        # 首次配置（联网拉取 GLFW / GLM）
.\scripts\build.ps1                   # 编译
.\scripts\run.ps1 -Target env_check   # D1：跑环境自检
```

编译出两个目标：

| 目标 | 文件 | 用途 |
|---|---|---|
| `env_check` | `src/env_check.cpp` | D1 环境自检（换机器/升级驱动后可重跑） |
| `learn_vulkan` | `src/hello_triangle.cpp` | **教程主程序**，跟着章节一路长大 |

> **当前进度：D1**（环境搭建）
> 跑通 `.\scripts\run.ps1 -Target env_check`、看到"结论：全部通过 ✅"，回 `PROGRESS.md` 给 D1 打勾。
>
> **D2 材料已就绪**：讲义 [`docs/ch02-instance-and-validation.md`](docs/ch02-instance-and-validation.md) +
> 骨架 `src/hello_triangle.cpp`（标着 `// TODO(ch02)` 的函数等你实现）+ 自检题 [`docs/ch02-check.md`](docs/ch02-check.md)。
> **D2 / D3 / D4 材料均已备好**：
> [D2](docs/ch02-instance-and-validation.md)（含代码骨架）、
> [D3](docs/ch03-physical-device-and-queues.md)、
> [D4](docs/ch04-window-surface-and-swapchain-probe.md)。每个单元配 `chNN-check.md` 自检题。
>
> 我只提前备**接下来两三个单元**的材料，避免你还没做完 D2 就被 D10 的资料淹没。
> 等你做完一个单元并告诉我，我会针对你的实现 review，再补下一个单元。

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
│   ├─ opengl-to-vulkan.md                 OpenGL -> Vulkan 职责对照词典
│   ├─ ch02-instance-and-validation.md     D2 讲义  ┐
│   ├─ ch02-check.md                       D2 自检题 │
│   ├─ ch03-physical-device-and-queues.md  D3 讲义  ├ 已备课
│   ├─ ch03-check.md                       D3 自检题 │
│   ├─ ch04-window-surface-and-swapchain-probe.md  D4 讲义 │
│   ├─ ch04-check.md                       D4 自检题 ┘
│   └─ chNN-*.md                           后续单元（随进度生成）
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
