# OpenGL → Vulkan：职责对照与心智模型

> 这份文档是你的"翻译词典"。建议 **打印出来贴在显示器旁**，每学一个 Vulkan 对象就回来对照一次。
>
> 使用方式：不要问"Vulkan 里 `glEnable` 的对应物是什么"（找不到），要问"**做这件事的职责，在 OpenGL 里是谁承担的**"。
> 答案几乎永远是：**OpenGL 的驱动**。Vulkan 把它交还给你了。

---

## 0. 一句话心智模型

> **OpenGL 是"驱动替你做的决定清单"。Vulkan 是同一份清单，但每一项都要你签字。**

| | OpenGL | Vulkan |
|---|---|---|
| 谁选 GPU | 驱动（或在 Windows 上由窗口系统决定） | 你，显式枚举并打分 |
| 谁管内存 | 驱动，你看不到 | 你，手动分配 + 选择内存类型 |
| 谁做同步 | 驱动隐式保证 | 你，semaphore / fence / barrier |
| 谁编译着色器 | 驱动运行时编译 GLSL | 你，离线 `glslc` 编译成 SPIR-V |
| 谁管状态 | 全局状态机，随时 `glEnable` | 不可变的 `VkPipeline`，创建时固化全部状态 |
| 谁管命令提交 | `glDrawElements` 立即执行 | 你录制 `VkCommandBuffer`，再 `vkQueueSubmit` |
| 出错怎么办 | `glGetError` 模糊提示，或直接黑屏 | **验证层**精确指出哪一行哪个参数违规 |

**性能收益来自哪里？** 不是"Vulkan 更快"，而是：驱动不再需要做猜测、状态合并、运行时编译、隐式同步这些工作在每帧重复发生。代价是这些工作变成你的一次性初始化代码。

---

## 1. 主对照表

### 1.1 启动与设备

| 职责 | OpenGL | Vulkan | 关键差异 |
|---|---|---|---|
| 加载 API 函数指针 | GLEW / GLAD，运行时 `wglGetProcAddress` | Loader（`vulkan-1.dll`）+ SDK 的 `vulkan-1.lib` 导入库 | Vulkan 的 loader 在**运行时**从 ICD（驱动）里找真正的实现；你链接的只是 loader |
| 全局上下文 | `wglCreateContext` / GLXContext，全局唯一 | `VkInstance` → `VkPhysicalDevice` → `VkDevice` | 设备是**多例**的：一台机器可枚举多块 GPU，各自独立 |
| 选设备 | 驱动选 | `vkEnumeratePhysicalDevices` + 你写 `isDeviceSuitable()` | 你有完全的掌控权，也要负完全的责任 |
| 查询能力 | `glGetIntegerv(GL_MAX_...)` | `vkGetPhysicalDeviceProperties/Features/QueueFamilyProperties` | 结构体驱动，字段极多 |
| 提交队列 | 隐式（有 shader/image/transfer 之分但 GPU 说了算） | `VkQueueFamilyProperties` 显式查询 + `vkGetDeviceQueue` | 你要自己找"支持图形"和"支持呈现"的队列族 |
| 实例扩展 | 通常不需要（WGL 扩展靠字符串匹配） | 创建 `VkInstance` 时必须显式列出（如 `VK_KHR_win32_surface`） | 不列就没有该功能 |
| 调试 | `glGetError`（只给错误码，不给位置） | `VK_EXT_debug_utils` + 验证层回调 | **这是你学习期最重要的工具**，务必第一时间开 |

### 1.2 显示与帧缓冲

| 职责 | OpenGL | Vulkan | 关键差异 |
|---|---|---|---|
| 窗口 | GLFW/SDL + `glfwCreateWindow`（同时创建 context） | GLFW + `glfwCreateWindowSurface` → `VkSurfaceKHR` | Vulkan 把"窗口"抽象成平台无关的 surface；窗口创建和图形 API 解耦 |
| 默认帧缓冲 | 驱动提供，`glBindFramebuffer(0)` 即可用 | 你从 `VkSwapchainKHR` 取出图像，`vkGetSwapchainImagesKHR` | **交换链是显式的**，图像数量、格式、呈现模式都要你自己谈 |
| 前/后缓冲交换 | `SwapBuffers` 隐式 | `vkAcquireNextImageKHR` + `vkQueuePresentKHR` | 你要自己管理"当前在画哪一张"以及重新获取的时机 |
| 帧缓冲对象 | `glGenFramebuffers` + `glFramebufferTexture2D` | `VkRenderPass` + `VkFramebuffer` | RenderPass 把"渲染时附件怎么用（load/store/layout）"和"附件是谁"分离开了 |
| 双缓冲/三缓冲 | 驱动/`wglSwapIntervalEXT` | `VkPresentModeKHR`（FIFO / MAILBOX / IMMEDIATE）+ 你自己的"飞行帧数" | 飞行帧（frames in flight）是你自己实现的流水线 |
| 窗口缩放 | 驱动自动处理默认帧缓冲 | 你监听回调 + **整个交换链重建**（含 image view、framebuffer、depth、pipeline 里的 viewport） | 这是 Vulkan 最容易踩的坑之一 |

### 1.3 管线与状态

| 职责 | OpenGL | Vulkan | 关键差异 |
|---|---|---|---|
| 状态设置 | `glEnable`/`glBlendFunc`/`glDepthFunc`… 全局可变状态机 | `VkPipeline`：所有状态在创建时**固化，之后不可变** | 切换管线比切换状态便宜；但换一个状态就要换一条管线（或用 dynamic state） |
| 顶点格式 | `glVertexAttribPointer` + `glEnableVertexAttribArray` | `VkVertexInputBindingDescription` + `VkVertexInputAttributeDescription` | 在**创建管线时**声明，且要显式说明数据来自哪个 binding |
| 着色器 | `glCreateShader` + `glCompileShader`（运行时编译 GLSL） | `glslc` **离线**编译 GLSL → SPIR-V，`vkCreateShaderModule` 加载 | 驱动不做编译，消除运行期卡顿；GLSL 需要写的更"显式"（如 `layout(location=)`） |
| 着色器阶段组合 | `glAttachShader` + `glLinkProgram` | `VkPipelineShaderStageCreateInfo` 数组 + `vkCreateGraphicsPipelines` | 没有独立的 program 对象，管线一次性建好 |
| Uniform 变量 | `glGetUniformLocation` + `glUniformMatrix4fv`（按名字） | `VkBuffer`（UBO）+ `VkDescriptorSetLayout` + `VkDescriptorSet` + `VkPipelineLayout` | 从"按名字传值"变成"按槽位传内存"。**这是 OpenGL 转 Vulkan 最大的一次思维跳跃** |
| 纹理 | `glGenTextures` + `glTexImage2D` + `glBindTexture` + `glTexParameteri` | `VkImage`（数据）+ `VkImageView`（怎么解释）+ `VkSampler`（怎么采样）+ descriptor | 三者解耦；且需要手动做 **layout 转换**（`UNDEFINED→TRANSFER_DST→SHADER_READ_ONLY`） |
| 顶点数据 | `glBufferData` 一把梭 | `createBuffer` 分配 `VkBuffer` + `vkAllocateMemory` + `vkBindBufferMemory` + 拷贝 | 显式两段式：缓冲对象和它的内存是分开的 |
| 索引缓冲 | `glDrawElements` + `GL_ELEMENT_ARRAY_BUFFER` | `vkCmdBindIndexBuffer` + `vkCmdDrawIndexed` + `VK_BUFFER_USAGE_INDEX_BUFFER_BIT` | usage flag 必须创建时声明 |
| 深度测试 | `glEnable(GL_DEPTH_TEST)` + 深度附件 | 管线里的 `VkPipelineDepthStencilStateCreateInfo` + 你创建的深度 `VkImage` | 深度缓冲要你自己分配和转换 layout |

### 1.4 命令提交与同步

| 职责 | OpenGL | Vulkan | 关键差异 |
|---|---|---|---|
| 绘制调用 | `glDrawElements` 立即入队 | `vkCmdDrawIndexed` **录制**到 `VkCommandBuffer`，再 `vkQueueSubmit` | 命令缓冲可预录制、复用、多线程并行录制 |
| 命令内存 | 无概念 | `VkCommandPool` + `VkCommandBuffer`，且每个飞行帧一份 | 生命周期管理是新手最大崩溃源 |
| 同步 | 驱动隐式；`glFinish`/`glFlush` 兜底 | `VkSemaphore`（GPU↔GPU）+ `VkFence`（GPU↔CPU）+ `vkCmdPipelineBarrier`（GPU 内部） | **全显式**。忘记同步 = 画面闪烁/崩溃/验证层报错 |
| 资源复用保护 | 驱动保证安全 | `vkWaitForFences` + `vkResetFences` + 每飞行帧独立资源 | 你要自己保证"上一帧用完之前不覆写" |
| 内存类型 | 驱动选 | `vkGetPhysicalDeviceMemoryProperties` + `findMemoryType()` 查 `DEVICE_LOCAL` / `HOST_VISIBLE` 等 | 显存 vs 可映射内存的分层要你自己安排（staging buffer 就是为此而生） |

### 1.5 进阶

| 主题 | OpenGL | Vulkan |
|---|---|---|
| 计算 | GL 4.3 `glDispatchCompute` + SSBO | 独立于图形管线的 compute queue / pipeline / descriptor + `vkCmdDispatch` |
| 多重采样 | `glEnable(GL_MULTISAMPLE)` + 窗口 hint | 你自己建 MSAA 图像 + `VkPipelineMultisampleStateCreateInfo` + resolve 附件 |
| Mipmap | `glGenerateMipmap` 一行 | 逐级 `vkCmdBlitImage` 循环，每级之间插 barrier |
| 模型加载 | 与图形 API 无关 | 一样（tinyobjloader），但顶点数据要按你声明的 binding 重排 |

---

## 2. 三张必须画出来的图

每进入一个新阶段，先画这些（用纸和笔，比看文档有效 10 倍）：

### 图 A：对象依赖图（D2–D12 期间每天看）

```
VkInstance ──┬── VkSurfaceKHR (来自 GLFW)
             └── VkPhysicalDevice
                    └── VkDevice ──┬── VkQueue (graphics)
                                   └── VkQueue (present)
VkDevice ──┬── VkSwapchainKHR ── VkImage[] ── VkImageView[]
           ├── VkRenderPass ──┬── VkFramebuffer[] (需要 ImageView)
           │                  └── VkPipeline
           ├── VkShaderModule
           ├── VkCommandPool ── VkCommandBuffer[]
           ├── VkSemaphore[] / VkFence[] / VkBuffer (UBO) / VkImage (纹理/深度)
           └── VkDescriptorSetLayout ── VkPipelineLayout ── VkPipeline
```

**销毁时必须严格逆序。** 这是新手最常见的崩溃原因。

### 图 B：一帧的生命周期（D11–D12）

```
drawFrame():
  1. vkWaitForFences       ← 等上一轮这组飞行帧资源空闲
  2. vkAcquireNextImageKHR ← 问交换链要一张图（信号：imageAvailable）
  3. vkResetFences
  4. vkResetCommandBuffer
  5. recordCommandBuffer   ← 录制：begin → 清屏 → bind pipeline → draw → end
  6. vkQueueSubmit         ← 等 imageAvailable，完成时信号 renderFinished，fence 通知 CPU
  7. vkQueuePresentKHR     ← 等 renderFinished
```

### 图 C：内存流向（D13–D21）

```
CPU 端数组 (std::vector<Vertex>)
   │  memcpy（映射内存）
   ▼
Staging VkBuffer  [HOST_VISIBLE | HOST_COHERENT]
   │  vkCmdCopyBuffer / vkCmdCopyBufferToImage
   ▼
Device-local VkBuffer / VkImage  [DEVICE_LOCAL]  ← GPU 从这里读，最快
   │  layout 转换 + barrier
   ▼
着色器通过 descriptor set 访问
```

**为什么不能直接把 CPU 数组给 GPU 用？** 因为 GPU 显存（DEVICE_LOCAL）通常 CPU 不可见。staging buffer 就是这座桥。这是 OpenGL 里驱动偷偷帮你做的、Vulkan 里要你亲手做的事。

---

## 3. 每章必答的问题

学完每一章，用一句话回答：**"这一步在 OpenGL 里是谁做的？"**

| 章节 | 该回答的问题 |
|---|---|
| Instance / 验证层 | OpenGL 里有没有"创建实例"？如果没有，等价的工作是谁在哪一刻做的？ |
| 物理设备 / 队列族 | OpenGL 里怎么知道有几块 GPU、用哪条队列？为什么 OpenGL 你从没关心过？ |
| 交换链 | `SwapBuffers` 背后是什么？为什么显示模式、图像数量、格式在 OpenGL 里不可见？ |
| 固定功能 / 管线 | `glEnable(GL_CULL_FACE)` 之后驱动做了什么？为什么 Vulkan 要把它变成管线的字段？ |
| 命令缓冲 | `glDrawElements` 返回时，GPU 真的画完了吗？谁在保证顺序？ |
| 描述符 | `glUniform*` 是怎么把数据送到着色器的？为什么 Vulkan 要用"槽位 + 内存"代替"名字 + 值"？ |
| 纹理与 layout | 为什么 OpenGL 不需要你写 `transitionImageLayout`？谁在替你转？ |
| 深度缓冲 | `GL_DEPTH_TEST` 打开时，OpenGL 偷偷创建了什么？ |
| Mipmap | `glGenerateMipmap` 一行背后是什么循环？ |
| MSAA | `glEnable(GL_MULTISAMPLE)` 之后谁在 resolve？ |
| 计算着色器 | 图形管线和计算管线为什么必须是分开的对象？ |
