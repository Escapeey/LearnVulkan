# 学习进度打卡

> 规则：**每完成一个单元才打勾**，并在下面写 3 行以内的总结（不写总结等于没完成）。
> 打勾前必须能回答该单元 `docs/chNN-check.md` 的自检问题。

## 阶段 0：环境

- [x] **D1** 环境搭建 + 冒烟测试 —— **已实测通过 ✅**
  - Visual Studio：**18 Community（2026）**，MSVC 14.51.36231 + Windows SDK 10.0.26100
  - Vulkan SDK：**1.4.357.0**（`C:\VulkanSDK\1.4.357.0`，环境变量已生效）
  - CMake：**4.3.1**（VS 自带，不在 PATH —— `build.ps1` 已自动探测）
  - 构建：`env_check.exe` 和 `learn_vulkan.exe` 均编译成功；`glslc` 已产出 `shader.vert.spv` / `shader.frag.spv`
  - 自检结果：**全部通过 ✅** —— loader 1.4.357 / 13 个实例扩展（含 `VK_EXT_debug_utils`）/ **9 个层（含 `VK_LAYER_KHRONOS_validation`）** / GLM 深度映射正确
  - 总结：（这里写你自己的 3 行）

## 阶段 1：第一个三角形（M1）

- [ ] **D2** 基础代码 + Instance + 验证层
  - 总结：
- [ ] **D3** 物理设备 + 队列族 + 逻辑设备
  - 总结：
- [ ] **D4** 窗口表面 + 交换链探测
  - 总结：
- [ ] **D5** 交换链创建 + 图像视图
  - 总结：
- [ ] **D6** 着色器模块
  - 总结：
- [ ] **D7** 固定功能（上）
  - 总结：
- [ ] **D8** 固定功能（下）
  - 总结：
- [ ] **D9** 渲染通道
  - 总结：
- [ ] **D10** 帧缓冲 + 创建图形管线
  - 总结：
- [ ] **D11** 命令缓冲 + 渲染与呈现
  - 总结：
- [x] **D12** 飞行帧 + 交换链重建
  - 总结：
    - 在飞帧：命令缓冲/信号量/fence 都要"每帧或每图各留一份"——命令缓冲按 MAX_FRAMES_IN_FLIGHT 分、renderFinished 信号量按 imageIndex 分，不能共用一个。
    - 踩了两个验证层坑：单命令缓冲没执行完就 reset/submit → 00045/00049/00071；renderFinished 按帧槽复用却撞上 3 张图 → 00067。改成按帧/按图索引后验证层清零。
    - 缩放：recreateSwapChain 先 vkDeviceWaitIdle 再整套重建（含静态 viewport 的管线）；放大后的"锯齿"是没开 MSAA，属后面多重采样章节，不归 D12。
- [x] 🎉 **M1 验收**：彩色三角形 + 缩放不崩 + 验证层零 error
  - 复述检验（合上代码画出 vkCreateInstance → 三角形的完整调用链）：`通过 / 未通过`　← 这行留给你自己合上代码自测后填

## 阶段 2：让画面动起来（M2）

- [ ] **D13** 顶点输入描述 + 顶点缓冲
- [ ] **D14** 暂存缓冲 + 索引缓冲
- [ ] **D15** 描述符布局 + UBO
- [ ] **D16** 描述符池与集合 + MVP
- [ ] 🎉 **M2 验收**：旋转的透视正确四边形

## 阶段 3：纹理与 3D（M3 / M4）

- [ ] **D17** 图像：贴图加载 + 布局转换
- [ ] **D18** 图像视图/采样器 + 组合图像采样器
- [ ] **D19** 深度缓冲
- [ ] **D20** 加载模型
- [ ] 🎉 **M4 验收**：带纹理的 OBJ 模型，深度遮挡正确

## 阶段 4：质量与进阶（M5）

- [ ] **D21** 生成 Mipmaps
- [ ] **D22** 多重采样 MSAA
- [ ] **D23** 计算着色器（上）
- [ ] **D24** 计算着色器（下）
- [ ] 🎉 **M5 验收**：教程全部完成

## 收尾

- [ ] **D25** 总复盘 + 下一步方向

---

## 我的踩坑记录（每踩一个坑写一行，这是最值钱的部分）

| 日期 | 单元 | 现象 | 根因 | 解决 |
|---|---|---|---|---|
| 首次构建 | D1 | 预检误报"找不到 cmake"，但其实已装 | VS 自带的 CMake 在 `Common7\IDE\CommonExtensions\...`，**不在 PATH 上** | `build.ps1` 增加 `Find-CMake`，PATH 找不到就去 VS 安装目录翻 |
| 首次构建 | D1 | 配置到 glm 处脚本退出（exit 1），日志停在 `-- 依赖 glm: 使用本地源码` | **⚠️ 根因至今未确认。** 真正的 CMake 报错写进了 **stderr**，而我用的 `Tee-Object` **只捕获 stdout** —— 报错文本根本没落盘。只看到一个 glm 的 `cmake_minimum_required` 弃用警告 | 同一轮改了三处（`ErrorActionPreference` Stop→Continue、原生调用把 stderr 并进 stdout、加 `CMAKE_POLICY_VERSION_MINIMUM 3.5`）后就好了。但**事后单独撤销任一处都无法复现失败**，所以不能确定是哪一处修好的 |
| 首次构建 | D1 | 生成器 `Visual Studio 17 2022` 不存在 | 实际装的是 **VS 18（2026）**，我硬编码了 17 | `build.ps1` 改为从 `cmake --help` 自动探测编号最大的 VS 生成器 |
| 首次构建 | D1 | CMake 4.x 对 GLM 报 `cmake_minimum_required` 弃用 | CMake 4.x 移除了对 `VERSION < 3.5` 的兼容 | 在根 `CMakeLists.txt` 设 `CMAKE_POLICY_VERSION_MINIMUM 3.5` |
| 首次构建 | D1 | 拉依赖卡死/失败 | **github.com 在你这台机器上 TCP 连不上**（21 秒超时），gitee.com 正常 | 默认源改成 `gitee.com/mirrors/*`，并把依赖克隆到本地 `third_party/` |
| 首次构建 | D1 | `env_check.cpp` 报 C2039：`name` 不是 `VkExtensionProperties` 的成员 | 我把字段名写成 `ext.name`，正确的是 **`ext.extensionName`** | 修正 3 处 |
| 首次构建 | D1 | 验证层只被"**枚举**"过，从没验证过能否**加载 + 回调** | `vkEnumerateInstanceLayerProperties` 只读 manifest 文件，不加载 layer DLL —— 这两件事完全不同，D2 全靠后者 | 写了一次性探针实测：`VkLayer_khronos_validation.dll` 成功加载 → `vkCreateInstance` = VK_SUCCESS → 故意泄漏 messenger 拿到 `VUID-vkDestroyInstance-instance-00629` → 共收到 **79 条**验证层消息。**结论：D2 的机制是通的**。探针已删除，不留答案在仓库里 |
| 备课 | D2 | 骨架的 10 个函数"能不能按这个签名实现"没验证过 | 万一某个签名有坑，你会以为是自己写错了，白折腾 | 照骨架写了份**临时参考实现**：`src/hello_triangle.cpp` 的签名**一行没改就编译通过**。两条路径实测：正常销毁 → `消息总数=0 ERROR=0`；故意泄漏 messenger → 精确报出 `VUID-vkDestroyInstance-instance-00629`，点名句柄。**结论：你的起点是干净的、可实现的。** 参考实现已删除 |
| 备课 | D3–D5 | 讲义里全是"你自己跑一下看看"，没有你这台机器的具体数值 | 交换链能力 / 呈现模式 / 队列族 / DPI 这些因机器差异极大，没有基准值你就无法判断自己的输出对不对 | 跑了一次性硬件探针，实测值固化成 `docs/your-gpu.md`。**两条会改变预期的发现：① 你的显卡不支持 `MAILBOX`，`chooseSwapPresentMode` 必然回退 `FIFO`；② `currentExtent` 不是 `UINT32_MAX`，`chooseSwapExtent` 的 else 分支在你这台机器上永不执行**。探针已删除 |
| 备课 | D2 | 我文档里写"没实现 initWindow 就跑会崩" | 实际是**挂住**：`glfwWindowShouldClose(nullptr)` 触发 GLFW 内部 assert，Debug 构建下弹模态对话框，进程不退出、无输出 | 给骨架的 `mainLoop()` 加了 `window == nullptr` 前置检查，抛明确异常。实测从 **180 秒超时** 变成 **4.6 秒返回 + 可读报错** |
| | | | | |

---

## 📌 这次排查最大的教训：报错文本要第一时间完整落盘

上面第 2 条（"配置到 glm 处退出"）的根因我**最终没查出来**。原因不是问题太难，而是**我把证据弄丢了**：

我当时这样重定向：

```powershell
& .\scripts\build.ps1 -Clean 2>&1 | Tee-Object -FilePath build.log
```

`Tee-Object` **只捕获 stdout**。而 CMake 的报错走 **stderr** —— 于是日志"干干净净地"停在一行无害的弃用警告上，**真正的错误文本一行都没存下来**。等我想回头查根因时，现场已经被后续修改覆盖了，只能靠猜。

### 正确做法

```powershell
# 让脚本内部就把 stderr 并进 stdout（build.ps1 现在就是这么做的）
& $cmakeExe @cmakeArgs 2>&1 | ForEach-Object { Write-Host $_ }

# 或者在命令行重定向【所有】流
.\scripts\build.ps1 *> build.log
```

### 这条教训直接适用于 Vulkan

**验证层默认往 `stderr` 输出**（教程的 `debugCallback` 就是 `std::cerr << ...`）。
如果你只重定向 stdout 去看日志，你会得到"程序什么都没说"的假象 —— 然后开始怀疑自己的代码。

**D2 起就会遇到这个场景。** 记住：Vulkan 的报错在 stderr。
