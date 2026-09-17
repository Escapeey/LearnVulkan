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

.\scripts\build.ps1 -Configure   # 首次配置（联网拉取 GLFW / GLM）
.\scripts\build.ps1              # 编译
.\scripts\run.ps1                # 运行
```

> **现在是 D1**：`src/main.cpp` 是环境自检程序。跑通它、看到"结论：全部通过 ✅"，
> 再回 `PROGRESS.md` 给 D1 打勾，然后我们开始 D2。

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
│   ├─ opengl-to-vulkan.md OpenGL -> Vulkan 职责对照
│   └─ chNN-*.md           每单元讲义（随进度生成）
├─ src/
│   └─ main.cpp            当前正在写的代码
├─ shaders/                GLSL 源码
├─ scripts/                build.ps1 / run.ps1
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
