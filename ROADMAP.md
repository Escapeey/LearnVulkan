# Vulkan 学习路线图

> 教程：https://tutorial.vulkan.net.cn/ （Overv VulkanTutorial 中文版）
> 学员背景：已入门 OpenGL，熟悉图形管线概念，C++ 可用
> 节奏：3–4 小时/天，集中攻关
> 终点：走完全部教程主体，最终拥有一个可加载 OBJ 模型、带纹理/深度/Mipmap/MSAA 的 Vulkan 渲染器

---

## 一、先明确一件事：你比零基础的人快在哪、慢在哪

**你的优势（要主动利用）**
你脑子里已经有一份"渲染一帧需要什么"的完整清单：顶点、着色器、纹理、矩阵、深度、帧缓冲。
Vulkan 并没有换掉这份清单，它只是把**过去由 OpenGL 驱动替你做的每一个决定，全部搬到你的代码里让你签字**。
所以对你来说，学习任务不是"学新概念"，而是"**把 OpenGL 隐式的部分显式化**"。
每学一个 Vulkan 对象，你都要能回答：*"这一步在 OpenGL 里是谁做的？"* —— 答得出来，你就真懂了。

**你的风险（要主动防）**
1. 想用 OpenGL 的函数名去找 Vulkan 的对应物 → 会迷路。要按**职责**对照，不是按名字。见 `docs/opengl-to-vulkan.md`。
2. 卡在环境/CMake/链接错误上，一天就没了 → 所以第一步我先把工程骨架搭好，你只管写 Vulkan 逻辑。
3. 抄教程答案 → 抄完能跑，但下周全忘。所以我们用"我给你骨架，你填核心"的方式强制你动手。

---

## 二、总览：5 个阶段 / 24 个学习单元

每个单元 ≈ 3–4 小时。**总计约 3.5–4 周**（允许 ±30% 浮动，卡住很正常）。

| 阶段 | 单元 | 内容 | 里程碑 |
|---|---|---|---|
| **0 环境** | D1 | 工具链 + 冒烟测试 + 读 Overview 建立全局图景 | M0：能编译运行并打印扩展数 |
| **1 第一个三角形** | D2–D12 | 教程最陡的一段，也是最重要的一段 | **M1：彩色三角形** |
| **2 让画面动起来** | D13–D16 | 顶点/索引/暂存缓冲 → UBO/描述符 → MVP 旋转 | **M2：旋转的 MVP 四边形** |
| **3 纹理与 3D** | D17–D20 | 图像/采样器 → 深度缓冲 → 加载模型 | **M3 / M4：带纹理的 3D 模型** |
| **4 质量与进阶** | D21–D24 | Mipmap → MSAA → 计算着色器 | **M5：教程完成** |
| **收尾** | D25 | 总复盘 + 下一步方向 | — |

### 阶段 0：环境（D1）

| 单元 | 任务 | 教程章节 | 你要亲手写的核心逻辑 |
|---|---|---|---|
| D1 | 装 VS2022 + Vulkan SDK；编译冒烟测试；读 Overview/简介 | 开发环境、概述 | 无（跑通即可）；但必须回答：为什么 Vulkan 需要 loader？对照你的 GLEW/GLAD 经验 |

### 阶段 1：第一个三角形（D2–D12）

| 单元 | 任务 | 教程章节 | 你要亲手写的核心逻辑 |
|---|---|---|---|
| D2 | 基础代码 + 实例 + 验证层 | Setup/Base_code, Instance, Validation_layers | `createInstance()`、扩展列表、`VkApplicationInfo`、debug messenger 回调 |
| D3 | 物理设备 + 队列族 + 逻辑设备 | Physical_devices_and_queue_families, Logical_device_and_queues | `pickPhysicalDevice()`、`findQueueFamilies()`、`createLogicalDevice()` |
| D4 | 窗口表面 + 交换链（探测） | Presentation/Window_surface, Swap_chain（前半） | `createSurface()`、capabilities/format/present mode 选择算法 |
| D5 | 交换链（创建）+ 图像视图 | Swap_chain（后半）, Image_views | `createSwapChain()`、`createImageViews()` |
| D6 | 着色器模块 | Graphics_pipeline_basics/Shader_modules, Introduction | GLSL→SPIR-V 流程、`createShaderModule()`、读懂 `VkPipelineShaderStageCreateInfo` |
| D7 | 固定功能（上） | Fixed_functions（前半） | vertex input / input assembly / viewport / rasterizer 各结构体的字段含义 |
| D8 | 固定功能（下） | Fixed_functions（后半） | multisampling / color blend / dynamic state / **pipeline layout 与 descriptor set layout 的关系** |
| D9 | 渲染通道 + 图形管线 | Render_passes, Conclusion | `createRenderPass()` 的 attachment/subpass/dependency 三段式 |
| D10 | 帧缓冲 + 创建图形管线 | Framebuffers, Conclusion | `createFramebuffers()`、`vkCreateGraphicsPipelines()` ← **全教程最关键的一次调用** |
| D11 | 命令缓冲 + 渲染与呈现 | Drawing/Command_buffers, Rendering_and_presentation | `createCommandPool/Buffers()`、`recordCommandBuffer()`、`drawFrame()` 的 submit/present |
| D12 | 飞行帧 + 交换链重建 + 收尾 | Frames_in_flight, Swap_chain_recreation | 信号量/栅栏配对、`recreateSwapChain()`、`cleanup()` 逆序销毁 | 

**M1 验收标准**：屏幕上出现彩色三角形；窗口缩放/最小化不崩溃；**开启验证层后日志零 error**。

### 阶段 2：让画面动起来（D13–D16）

| 单元 | 任务 | 教程章节 | 你要亲手写的核心逻辑 |
|---|---|---|---|
| D13 | 顶点输入描述 + 顶点缓冲 | Vertex_buffers/Vertex_input_description, Vertex_buffer_creation | `bindingDescription`/`attributeDescriptions`、`findMemoryType()` |
| D14 | 暂存缓冲 + 索引缓冲 | Staging_buffer, Index_buffer | `createBuffer()` 通用化、`copyBuffer()`、`createIndexBuffer()` |
| D15 | 描述符布局 + UBO | Uniform_buffers/Descriptor_layout_and_buffer | `createDescriptorSetLayout()`、`createUniformBuffers()`、`updateUniformBuffer()` |
| D16 | 描述符池与集合 + MVP | Descriptor_pool_and_sets | `createDescriptorPool/Sets()`、`vkUpdateDescriptorSets()`、MVP 矩阵与旋转 | 

**M2 验收标准**：四边形绕轴旋转，透视正确，窗口缩放时画面不变形。

### 阶段 3：纹理与 3D（D17–D20）

| 单元 | 任务 | 教程章节 | 你要亲手写的核心逻辑 |
|---|---|---|---|
| D17 | 图像：贴图加载 + 布局转换 | Texture_mapping/Images | stb_image 加载、staging→device local 两段拷贝、`transitionImageLayout()`、`copyBufferToImage()` |
| D18 | 图像视图/采样器 + 组合图像采样器 | Image_view_and_sampler, Combined_image_sampler | `createTextureImageView/Sampler()`、采样器寻址与过滤模式、着色器里 `texture()` |
| D19 | 深度缓冲 | Depth_buffering | `createDepthResources()`、深度格式选择、`VkImageMemoryBarrier` 用于深度附件 |
| D20 | 加载模型 | Loading_models | tinyobjloader 集成、`loadModel()` 去重顶点、顶点结构扩展 | 

**M4 验收标准**：带纹理的 OBJ 模型正确渲染，前后遮挡关系正确（深度生效）。

### 阶段 4：质量与进阶（D21–D24）

| 单元 | 任务 | 教程章节 | 你要亲手写的核心逻辑 |
|---|---|---|---|
| D21 | 生成 Mipmaps | Generating_Mipmaps | `generateMipmaps()` 的逐级 blit 循环 |
| D22 | 多重采样 MSAA | Multisampling | `getMaxUsableSampleCount()`、颜色附件改为 resolve 模式 |
| D23–D24 | 计算着色器 | Compute_Shader | 独立于图形管线的 compute queue/pipeline/descriptor、`vkCmdDispatch()`、SSBO | 

**M5 验收标准**：全部章节跑通，验证层零 error，能自己解释每一章"OpenGL 里是谁做的这一步"。

---

## 三、每个学习单元的固定工作流（6 步）

这是**学习方式**的核心，请严格照做：

1. **我交付讲义**：`docs/chNN-*.md` —— 概念全景图 + OpenGL 对照 + 关键 API 的参数为什么长这样 + 常见坑。
2. **你读原文**：读教程对应章节，**先别看代码清单**。重点读"为什么需要这个东西"。
3. **你填骨架**：我在 `src/` 里留好结构体、成员、函数签名，核心逻辑是 `// TODO(chNN)`。骨架能编译，但行为是空的/错的。
4. **你编译运行**：`.\scripts\build.ps1` 然后 `.\scripts\run.ps1`。改一次跑一次。
5. **卡住 → 贴给我**：把**验证层原文**（validation layer output）连同你的代码一起贴过来。不要只贴"报错了"。
6. **通过 → 我 review**：我读你的实现，指出 Vulkan 特有的坑（生命周期、同步、内存类型、layout）。然后你在 `PROGRESS.md` 写 3 行总结，`git tag` 存档。

> **铁律**：每单元结束前，先自己回答 `docs/chNN-check.md` 的自检问题。
> 答不上来就别往下走 —— Vulkan 后面的章节全部建立在前面的对象上，欠债利息很高。

---

## 四、学习方式：7 条针对"OpenGL 转 Vulkan"的原则

1. **按职责对照，不按函数名对照。** 详细对照表见 `docs/opengl-to-vulkan.md`，建议打印出来贴显示器边。
2. **验证层是你的老师，不是敌人。** 每写一个对象就立刻开验证层跑一次。Vulkan 不报错不代表对，验证层报错几乎总是真问题。
3. **先画对象依赖图再写代码。** 每个单元开始前，用纸画出"谁依赖谁"（如：Pipeline ← PipelineLayout ← DescriptorSetLayout；Framebuffer ← RenderPass + ImageView）。Vulkan 的复杂度 90% 在依赖顺序和生命周期。
4. **理解"显式"的代价与收益。** 每当一个操作在 Vulkan 里要写 30 行，停下来问：OpenGL 那 1 行背后驱动做了什么？这就是性能来源。
5. **同步是独立课题。** OpenGL 帮你隐式同步；Vulkan 里 semaphore/fence/barrier 你要自己保证正确。D11–D12 会专门啃这块，别跳过。
6. **不要引入抽象层。** 教程后期会建议把资源封成类（RAII）。**但前 20 个单元请全部写在一个 `main.cpp` 的 `HelloTriangleApplication` 类里**，跟教程一致。过早抽象会让你看不清 Vulkan 的真实结构。
7. **每个里程碑做一次"从零复述"。** 合上代码，白纸画出从 `vkCreateInstance` 到画面出现的完整调用链。画不出来说明还没掌握。

---

## 五、卡住时的排查顺序（省时间用）

1. **看验证层**，不要看黑屏猜。90% 的问题验证层直接告诉你。
2. 有画面但全黑 → 先确认是不是 `recordCommandBuffer` 里没调用 `vkCmdDraw`，或渲染通道的 `loadOp` 写成了 `DONT_CARE`。
3. 崩溃在 `vkCreateDevice` → 队列族索引搞错，或忘了把 device extension 加进 `enabledExtensionNames`。
4. 链接错误 `unresolved external symbol vkXxx` → 没链接 `vulkan-1.lib`，或 SDK 没装好。
5. `glslc` 找不到 → 没重启终端导致 `VULKAN_SDK` 环境变量没生效。
6. 画面撕裂/闪烁/退出时报错 → 同步对象复用错误，回到 D12 复习。
7. **换一台机器/升级驱动后行为变了** → 先跑 `vulkaninfo --summary` 看设备能力，别假设。

---

## 六、D1 的唯一任务

见 `SETUP.md`。**在你跑通冒烟测试之前，不要开始 D2。**

进度打卡见 `PROGRESS.md`。
