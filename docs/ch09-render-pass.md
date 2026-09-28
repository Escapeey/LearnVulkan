# D9 讲义：渲染通道 render pass —— attachment / subpass / dependency 三段式

> 教程章节：[Render_passes](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes)
>
> 前置：D8 完成（固定功能·下全部填好，`pipelineLayout` 已创建）
> 自检题：`docs/ch09-check.md`

---

## 0. 本单元你要亲手写的

| 结构体 / 对象 | 难度 | 对应 OpenGL | 本单元状态 |
|---|---|---|---|
| `VkAttachmentDescription` | ★★★ | `glFramebufferTexture2D` 里的附件描述 | 1 个颜色附件 = 交换链图像 |
| `VkAttachmentReference` | ★★ | （无显式对应） | subpass 引用附件 0 |
| `VkSubpassDescription` | ★★ | （无显式对应） | 1 个图形子通道 |
| `VkSubpassDependency` | ★★★ | （OpenGL 隐式同步） | 外部 → 子通道 0 的同步 |
| `VkRenderPass` + `vkCreateRenderPass` | ★★ | `glGenFramebuffers` 的"配方"部分 | 创建 + 句柄 |

> 前 4 个是**纯结构体**（填字段）；最后 `vkCreateRenderPass` 是**第二次真正创建对象**
> （第一次是 D8 的 `vkCreatePipelineLayout`）——返回句柄、要 `vkDestroyRenderPass` 销毁。
>
> **仍然不调用 `vkCreateGraphicsPipelines`**——D10 才做，届时本单元的 `renderPass` 要填进去。

---

## 1. 全景：render pass 是什么、不是什么

先破除一个最容易混的概念：**render pass 里没有图像**。

| | render pass（D9） | framebuffer（D10） |
|---|---|---|
| 描述的是 | 附件**长什么样、怎么用、怎么同步**（格式/采样/loadOp/layout…） | 附件**具体是哪些真实图像视图**（`VkImageView` 数组） |
| 类比 | 一份**配方 / 函数签名** | 一份**食材 / 实参** |
| 持有图像吗 | 不持有 | 持有 |

Vulkan 把 OpenGL 里"一个 FBO 对象"这件事，拆成了**两份**：一份"怎么画"（render pass，D9），
一份"画到哪"（framebuffer，D10）。OpenGL 里 `glGenFramebuffers` + `glFramebufferTexture2D`
一次把"描述"和"绑定"都做了；Vulkan 偏要拆开——因为同一份 render pass（配方）可以配不同的
framebuffer（食材），比如多张交换链图像。

### 三段式

一个 render pass 由三段组成，本单元就是按这个顺序写：

1. **attachment（附件）**：帧缓冲里的一个槽位，先说清楚它"是什么"——格式、采样数、进来/出去时怎么处理。
2. **subpass（子通道）**：一帧里的一"道"绘制，说明"这道绘制用到上面哪些附件、当颜色还是深度"。
3. **dependency（依赖）**：子通道之间（或与外部）的**同步关系**——谁必须等谁先做完。

> 为什么叫 render **pass**（通道）而不是 framebuffer？因为它的语义是"一次渲染的**流程描述**"，
> 不止是"一块内存"。一个 render pass 里可以有好几个 subpass（后处理链：先画场景、再画全屏
> 模糊…），每个 subpass 用不同的附件组合。本教程只画一个三角形，所以 **1 个 subpass、1 个附件**。

---

## 2. 附件描述 `VkAttachmentDescription`

```cpp
VkAttachmentDescription colorAttachment{};
colorAttachment.format         = swapChainImageFormat;           // 像素格式，必须和交换链一致
colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;          // 采样数（D22 MSAA 才改）
colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;    // 开始前清屏
colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;   // 结束后存回
colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;// 模板，不用
colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;      // 进来前什么布局都行
colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;// 出去后要能呈现
```

### 逐个字段（先看 loadOp / storeOp 和 layout 两组）

| 字段 | 值 | 含义 |
|---|---|---|
| `format` | `swapChainImageFormat` | 附件像素格式。**必须**和交换链一致，否则画出来是垃圾 / 验证层报错 |
| `samples` | `VK_SAMPLE_COUNT_1_BIT` | 每像素 1 采样。**必须**和 D8 管线里的 `rasterizationSamples` 一致 |
| `loadOp` | `CLEAR` | 渲染通道**开始前**对附件做什么：`CLEAR` 清成 clear color |
| `storeOp` | `STORE` | 渲染通道**结束后**对附件做什么：`STORE` 存回（要呈现给屏幕，必须存） |
| `stencilLoadOp` / `stencilStoreOp` | `DONT_CARE` | 模板附件的 load/store，本教程不用模板，无所谓 |
| `initialLayout` | `UNDEFINED` | 图像**进来时**的布局。`UNDEFINED` = 不在乎它之前是什么 |
| `finalLayout` | `PRESENT_SRC_KHR` | 图像**出去时**的布局。要交给 present，必须是这个 |

### ⭐ loadOp / storeOp 讲的是"附件内容的命运"

这是 OpenGL 里 `glClear` 的固化版：

| `loadOp` | 含义 | 对照 OpenGL |
|---|---|---|
| `LOAD` | 保留上一帧的内容继续画 | 不 clear，直接画 |
| `CLEAR` | 开始前清成 clear color | `glClearColor` + `glClear(GL_COLOR_BUFFER_BIT)` |
| `DONT_CARE` | 内容无所谓（会被完全覆盖） | 无（OpenGL 总会读旧值） |

| `storeOp` | 含义 |
|---|---|
| `STORE` | 结束后把结果写回附件（要呈现，选它） |
| `DONT_CARE` | 结果不需要保留 |

我们画满整屏、要呈现，所以 `CLEAR + STORE`。`CLEAR` 的"清成什么颜色"不是在这里写的——
那要到 D11 `vkCmdBeginRenderPass` 传 `VkClearValue` 时才定。

### ⭐ initialLayout / finalLayout 讲的是"图像内存怎么摆"

Vulkan 的图像（`VkImage`）同一块内存可以用不同"布局"访问。布局 = 内存被按什么方式组织，
优化给谁用：

| 布局 | 用途 |
|---|---|
| `VK_IMAGE_LAYOUT_UNDEFINED` | 不关心（刚换到的交换链图像，内容无所谓） |
| `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL` | 当"颜色附件"被绘制时的最优布局 |
| `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` | 当"呈现源"交给屏幕时的布局 |

> 布局切换（transition）是显式的，**D17** 会专门用 `vkCmdPipelineBarrier` 做。
> 现在 render pass 里声明的 `initialLayout → finalLayout` 就是**约定好**这两次切换
> 会自动发生：`UNDEFINED` 进来 →（渲染时变 COLOR_ATTACHMENT_OPTIMAL）→ `PRESENT_SRC_KHR` 出去。
> 这也是本单元最需要"先背下来、D17 再懂透"的一对字段。

---

## 3. 附件引用 `VkAttachmentReference` + 子通道 `VkSubpassDescription`

subpass 不直接拿附件，而是拿**引用**（下标 + 期望布局），这样同一附件可以在不同 subpass 里
以不同布局被引用。

```cpp
// 附件引用：subpass 用「下标 + 期望 layout」指向一个附件
VkAttachmentReference colorAttachmentRef{};
colorAttachmentRef.attachment = 0;                                 // 引用第 0 个附件（colorAttachment）
colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 绘制时的最优布局

// 子通道：一道图形绘制，绑上颜色附件
VkSubpassDescription subpass{};
subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
subpass.colorAttachmentCount = 1;
subpass.pColorAttachments    = &colorAttachmentRef;
```

### 逐个字段

| 字段 | 值 | 含义 |
|---|---|---|
| `attachment` | `0` | 引用 `pAttachments` 数组里的**下标**（不是句柄、不是指针） |
| `layout` | `COLOR_ATTACHMENT_OPTIMAL` | subpass 执行期间，这个附件应处于的布局 |
| `pipelineBindPoint` | `GRAPHICS` | 这个 subpass 跑图形管线（也有 COMPUTE 子通道） |
| `colorAttachmentCount` / `pColorAttachments` | `1` / 指针 | 颜色附件列表 |

> `colorAttachmentRef.attachment = 0` 里的 `0`，和 `colorAttachmentRef.layout` 是两码事：
> 前者是"哪个附件"（下标），后者是"那个附件此刻处于什么布局"。这是本结构体最容易被
> 当成一回事的两行。

---

## 4. 子通道依赖 `VkSubpassDependency`

```cpp
VkSubpassDependency dependency{};
dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;                    // 外部 = 渲染通道开始之前
dependency.dstSubpass    = 0;                                      // 我们的第 0 号子通道
dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.srcAccessMask = 0;                                      // 等「颜色输出阶段」就绪
dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;   // 我们要写颜色附件
```

### 它是干什么的：给"谁先谁后"上保险

两个 subpass（或 subpass 与外部）之间如果**有内存读写依赖**，驱动需要知道"后者必须等前者
把内存写完"，否则可能读到半成品或画花。`VkSubpassDependency` 就是声明这条"先后 + 内存可见性"。

| 字段 | 值 | 含义 |
|---|---|---|
| `srcSubpass` | `VK_SUBPASS_EXTERNAL` | 依赖的"上一方"。`EXTERNAL` = 渲染通道之外（开始之前的那个时刻） |
| `dstSubpass` | `0` | 依赖的"下一方" = 我们的第 0 号 subpass |
| `srcStageMask` | `COLOR_ATTACHMENT_OUTPUT` | 上一方在哪个**管线阶段**产生我们要等的动作 |
| `srcAccessMask` | `0` | 上一方那次访问是"写"了什么内存。这里没有写，故 0 |
| `dstStageMask` | `COLOR_ATTACHMENT_OUTPUT` | 下一方在哪个阶段执行依赖动作 |
| `dstAccessMask` | `COLOR_ATTACHMENT_WRITE` | 下一方要"写颜色附件" |

读成一句话：**"在渲染通道开始（EXTERNAL）到第 0 号 subpass 之间，要等交换链图像在
颜色输出阶段就绪（srcStageMask），之后才允许 subpass 写颜色附件（dstAccessMask）。"**

> 为什么现在需要它：交换链图像在拿给我们画之前，可能正被上一帧的 present 占用。这条依赖
> 保证"等图像可用，才开始画"，避免画到正在被屏幕读的图像上。**本单元的依赖要到 D11
> 真正 begin/end render pass 时才生效**，现在只是"把规矩先写好"。

---

## 5. 汇总 + `vkCreateRenderPass`

```cpp
VkRenderPassCreateInfo renderPassInfo{};
renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
renderPassInfo.attachmentCount = 1;
renderPassInfo.pAttachments    = &colorAttachment;
renderPassInfo.subpassCount    = 1;
renderPassInfo.pSubpasses      = &subpass;
renderPassInfo.dependencyCount = 1;
renderPassInfo.pDependencies   = &dependency;

if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
}
```

三段式在这里被"组装"进一个 `VkRenderPassCreateInfo`：附件数组、子通道数组、依赖数组。
`vkCreateRenderPass` 校验这三段自洽后返回句柄。

---

## 6. 生命周期

- `renderPass` 是成员变量（`VkRenderPass renderPass = VK_NULL_HANDLE;`），从 D9 活到程序结束。
- `cleanup()` 里 `vkDestroyRenderPass(device, renderPass, nullptr);`，放在 `vkDestroyDevice` **之前**。
- D10 建管线时，`VkGraphicsPipelineCreateInfo::renderPass` 要填它——所以它得活过 D10。

> 为什么不做成局部变量？和 D8 的 `pipelineLayout` 同一个道理：它是被"创建"出来的句柄，
> 后面还有别的对象引用它。局部变量一出函数作用域句柄就丢了，也销毁不了。

---

## 7. OpenGL 对照总表（速查）

| OpenGL 调用 | Vulkan 字段 | 所在结构体 |
|---|---|---|
| `glGenFramebuffers` + `glFramebufferTexture2D`（描述 + 绑定一次完成） | render pass（描述，D9）+ framebuffer（绑定，D10） | 拆成两份 |
| `glClearColor` + `glClear(GL_COLOR_BUFFER_BIT)` | `loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR` | `VkAttachmentDescription` |
| （无 —— OpenGL 总是可读旧值） | `loadOp = LOAD / DONT_CARE` | `VkAttachmentDescription` |
| `glDrawBuffer(GL_COLOR_ATTACHMENT0)` | `pColorAttachments` + `attachment = 0` | `VkSubpassDescription` / `VkAttachmentReference` |
| （无 —— OpenGL 隐式同步） | `src/dstStageMask` + `src/dstAccessMask` | `VkSubpassDependency` |
| （无 —— OpenGL 内部管理） | `initialLayout` / `finalLayout` | `VkAttachmentDescription` |

> 最大的心智差异：OpenGL 的 FBO 是"一个对象，描述和绑定都在一起"；Vulkan 拆成
> render pass（描述，一份可复用）+ framebuffer（绑定，每个实际图像一份）。这就是为什么
> 交换链有几张图，D10 就要建几个 framebuffer，但 render pass 只要 1 个。

---

## 8. 本单元最容易踩的坑

| # | 现象 | 根因 | 处理 |
|---|---|---|---|
| 1 | 编译报 `renderPass` 未声明 | 忘了加成员变量 `VkRenderPass renderPass` | 加成员 + `= VK_NULL_HANDLE` |
| 2 | 退出时报 `VkRenderPass` 泄漏 | 忘了 `cleanup()` 里 `vkDestroyRenderPass` | 在 `vkDestroyDevice` 之前加 |
| 3 | `format` 抄成别的东西（如 `B8G8R8` 写反） | 没复用 `swapChainImageFormat` | 直接 `= swapChainImageFormat` |
| 4 | `samples` 和管线 `rasterizationSamples` 不一致 | 一边 `SAMPLE_COUNT_1` 一边改了 | 现在两者都 `SAMPLE_COUNT_1_BIT` |
| 5 | `storeOp` 写成 `DONT_CARE` | 以为"不用存" | 要呈现必须 `STORE`，否则画面不更新 |
| 6 | `finalLayout` 写成 `COLOR_ATTACHMENT_OPTIMAL` | 混淆"绘制中布局"和"出去后布局" | `finalLayout` = `PRESENT_SRC_KHR` |
| 7 | 漏了 `VkSubpassDependency` | 以为三段只要 attachment + subpass | 三段都要；少了依赖 D11 会有同步问题 |
| 8 | `pDependencies` 没赋值但 `dependencyCount = 1` | 忘了汇总进 `renderPassInfo` | 三个 `Count` + 三个 `p` 指针都要配对 |

---

## 9. 本单元完成标志

- [ ] 新建 `createRenderPass()`：colorAttachment → colorAttachmentRef → subpass → dependency → renderPassInfo → `vkCreateRenderPass`
- [ ] `initVulkan()` 里 `createImageViews()` 之后调用 `createRenderPass()`
- [ ] `renderPass` 是成员变量，`cleanup()` 里 `vkDestroyRenderPass`（在 `vkDestroyDevice` 之前）
- [ ] 能说清 render pass（描述）和 framebuffer（绑定，D10）的区别
- [ ] 能说出 `loadOp` / `storeOp` 各三种取值里我们为什么选 `CLEAR` / `STORE`
- [ ] 能说出 `initialLayout` / `finalLayout` 为什么是 `UNDEFINED` / `PRESENT_SRC_KHR`
- [ ] 能说清 `VkSubpassDependency` 里那两对 stage/access mask 分别"等谁、写谁"
- [ ] 能回答 `docs/ch09-check.md` 全部问题
- [ ] `PROGRESS.md` 里 D9 打勾 + 3 行总结

---

## 10. 前瞻

- **D10** 建帧缓冲 `createFramebuffers()` + **`vkCreateGraphicsPipelines`** —— 把 D6~D9 攒的所有
  结构体（含本单元的 `renderPass`）一次性建出真正的图形管线，全教程最关键的一次调用。
- M1（彩色三角形）在 **D11** 第一次出图：命令缓冲 + `vkCmdBeginRenderPass`（用上本单元的
  `renderPass` 和 `VkClearValue`）+ 绘制命令 + present。届时建议重做 D7 讲义第 8 节的观察实验。
