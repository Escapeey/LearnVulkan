# D10 讲义：帧缓冲 framebuffer + 图形管线 vkCreateGraphicsPipelines

> 教程章节：[Framebuffers](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Graphics_pipeline_basics/Framebuffers)、[Conclusion](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Graphics_pipeline_basics/Conclusion)
>
> 前置：D9 完成（render pass 已创建）
> 自检题：`docs/ch10-check.md`

---

## 0. 本单元你要亲手写的

| 结构体 / 对象 | 难度 | 对应 OpenGL | 本单元状态 |
|---|---|---|---|
| `VkGraphicsPipelineCreateInfo` + `vkCreateGraphicsPipelines` | ★★★ | `glLinkProgram` + 一堆 `gl*` 状态 | **建出真正的管线对象** |
| `vkDestroyShaderModule` ×2（建完管线后） | ★ | `glDeleteShader`（链接后） | 立刻销毁，D6 埋伏笔兑现 |
| `VkFramebufferCreateInfo` + `vkCreateFramebuffer` | ★★ | `glFramebufferTexture2D` | 每张交换链图像一个 |
| `vkDestroyPipeline` / `vkDestroyFramebuffer` | ★★ | `glDeleteProgram` / `glDeleteFramebuffers` | cleanup 里新增 |

> 本单元是 **D6~D9 的"收获时刻"**：之前填的每一份结构体，都在这里喂给
> `vkCreateGraphicsPipelines`，一次成型。这也是 **ROADMAP 里全教程最关键的一次调用**。
>
> **仍然看不到画面**——管线建好了，但还没有命令缓冲（D11）来"驱动"它。

---

## 1. 全景：本单元两件事

1. **建管线**：把 D6 着色器阶段、D7 固定功能、D8 剩余固定功能 + 管线布局、D9 渲染通道，
   全部"烘烤"成一个不可变的 `VkPipeline` 对象。
2. **建帧缓冲**：为交换链的每张图像各建一个 `VkFramebuffer`，把 D9 的"配方"（renderPass）
   绑到"食材"（图像视图）上。

一句话：

> **D6~D9 一直在"备料、写配方"，D10 是"开火炒菜、装盘"。**

---

## 2. 图形管线 `vkCreateGraphicsPipelines`（本单元核心）

接在 `createGraphicsPipeline()` 末尾（`pipelineLayout` 创建之后）：

```cpp
VkGraphicsPipelineCreateInfo pipelineInfo{};
pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
pipelineInfo.stageCount          = 2;
pipelineInfo.pStages             = shaderStages;           // D6：vert + frag 两个阶段
pipelineInfo.pVertexInputState   = &vertexInputInfo;       // D7
pipelineInfo.pInputAssemblyState = &inputAssembly;         // D7
pipelineInfo.pViewportState      = &viewportState;         // D7
pipelineInfo.pRasterizationState = &rasterizer;            // D7
pipelineInfo.pMultisampleState   = &multisampling;         // D8
pipelineInfo.pDepthStencilState  = &depthStencil;          // D8（Optional，但我们填了）
pipelineInfo.pColorBlendState    = &colorBlending;         // D8
pipelineInfo.pDynamicState       = &dynamicState;          // D8（Optional）
pipelineInfo.layout              = pipelineLayout;         // D8 成员句柄
pipelineInfo.renderPass          = renderPass;             // D9 成员句柄
pipelineInfo.subpass             = 0;                      // 用第 0 号子通道
pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;         // Optional
pipelineInfo.basePipelineIndex   = -1;                     // Optional

if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
}
```

### 逐个字段（重点看这四组）

| 字段 | 值 | 含义 |
|---|---|---|
| `stageCount` / `pStages` | `2` / `shaderStages` | 管线里的着色器阶段（顶点 + 片元） |
| `pVertexInputState` … `pDynamicState` | 指针 | D7~D8 那 8 个固定功能结构体，**传指针**（不是拷贝） |
| `layout` | `pipelineLayout` | 管线布局（成员句柄，非指针） |
| `renderPass` / `subpass` | `renderPass` / `0` | 这条管线要在**哪个渲染通道的哪个子通道**里画 |
| `basePipelineHandle` / `basePipelineIndex` | `nullptr` / `-1` | "从别的管线派生"优化，我们不用 |

### ⭐ 三个最容易被忽略的点

1. **函数名是复数的 `vkCreateGraphicsPipelines`**（不是 `...Pipeline`）。因为它支持**一次批量
   创建多条管线**：`createInfoCount` + `pCreateInfos` 数组。我们一次只建 1 条。
2. **第二个参数是 `VK_NULL_HANDLE`**——pipeline cache。可以把"编译好的管线"存盘/复用，
   下次启动省编译时间。教程用 `VK_NULL_HANDLE`（不用 cache）。
3. **`renderPass` 出现在两个地方**：既在管线的 `renderPass + subpass` 里，又在 framebuffer
   的 `renderPass` 里。**两者必须匹配**——管线说"我要在第 0 号 subpass 里画"，framebuffer
   说"我就是那个 renderPass 的一个具体实例"。

### ⭐ 管线是"烤死的"，这是和 OpenGL 最大的分野

`vkCreateGraphicsPipelines` 之后，这条管线的**所有固定状态都不再可变**（除非用了 D8 的
dynamic state）。想改混合模式、改深度比较规则、换顶点输入格式——**都得重建一条管线**。

对照 OpenGL：`glEnable(GL_BLEND)` / `glDepthFunc(...)` 随时能改，驱动在后台替你重新编译。
Vulkan 把"重新编译"显式化：状态变了 = 重新 `vkCreateGraphicsPipelines`。

> 这就是为什么叫 **graphics pipeline** 而不叫 "shader program"：OpenGL 的 program 只装
> 着色器，状态还是全局的；Vulkan 的 pipeline 把"着色器 + 固定功能 + 布局 + 渲染通道"打包成
> 一个整体，驱动能据此做最激进的优化。

---

## 3. ⭐ shader module「立刻销毁」——D6 埋伏笔兑现

建完管线后，紧接着：

```cpp
vkDestroyShaderModule(device, fragShaderModule, nullptr);
vkDestroyShaderModule(device, vertShaderModule, nullptr);
```

D6 讲义第 4 节说过「可以立刻销毁」，当时不能做是因为管线还没建。现在管线已经**把字节码
编译进去了**，`VkShaderModule` 只是那段字节码的"容器"，用完即可销毁。

> 对照 OpenGL：`glLinkProgram` 之后 `glDeleteShader`——shader 源码对象使命完成，program
> 里已经编好了。`VkShaderModule` ≈ `glCreateShader` 出来的对象，`VkPipeline` ≈ `glCreateProgram`。
>
> ⚠️ 因此 `cleanup()` 里 D6 加的那两行 `vkDestroyShaderModule` 要**删掉**，否则二次释放。

---

## 4. 帧缓冲 `createFramebuffers()`

```cpp
void createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = { swapChainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = renderPass;          // 用 D9 的"配方"
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments    = attachments;         // 具体绑哪张图
        framebufferInfo.width           = swapChainExtent.width;
        framebufferInfo.height          = swapChainExtent.height;
        framebufferInfo.layers          = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}
```

### 逐个字段

| 字段 | 值 | 含义 |
|---|---|---|
| `renderPass` | `renderPass` | 这个 framebuffer 是**哪个 renderPass 的实例** |
| `attachmentCount` / `pAttachments` | `1` / `&swapChainImageViews[i]` | 绑定哪个图像视图当颜色附件 |
| `width` / `height` | `swapChainExtent` | 帧缓冲尺寸（不能超过附件图像的实际尺寸） |
| `layers` | `1` | 图层数（不是立体/数组图像就是 1） |

### ⭐ renderPass vs framebuffer 的最终落地

- **renderPass 只有 1 个**：它描述"附件长什么样、怎么用"——这套规则所有帧都一样，一份就够。
- **framebuffer 有 N 个**（N = 交换链图像数）：因为每次呈现用的是**不同的那张图**，每个
  framebuffer 要把"配方"绑到**具体的那张图**上。

> D9 那句「配方 / 食材」在这里变成代码：`VkFramebufferCreateInfo.renderPass` 填同一个配方，
> `pAttachments` 填不同的食材。D11 开始画时会用 `vkCmdBeginRenderPass` + 某个 framebuffer
> 告诉驱动"这一帧画到第几张图"。

---

## 5. 生命周期 / 清理顺序

D10 起，`graphicsPipeline` 和 `swapChainFramebuffers` 也是要销毁的对象。关键的**引用关系**
决定了销毁顺序：

```
VkFramebuffer  ──引用──►  VkImageView + VkRenderPass
VkPipeline     ──引用──►  VkPipelineLayout + VkRenderPass
```

所以清理顺序（从"最靠外"到"最靠内"）：

```cpp
// cleanup() 里：
for (auto framebuffer : swapChainFramebuffers)   // ① 帧缓冲先（引用 imageView + renderPass）
    vkDestroyFramebuffer(device, framebuffer, nullptr);
...
vkDestroyPipeline(device, graphicsPipeline, nullptr);   // ② 管线（引用 pipelineLayout + renderPass）
vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
vkDestroyRenderPass(device, renderPass, nullptr);
// 然后是 imageView 循环 → swapchain → ...（已有的顺序不变）
```

> 一句话记忆：**谁被引用，谁后销毁**。framebuffer / pipeline 是"使用者"，先销毁；
> 它们引用的 renderPass / pipelineLayout / imageView 是"被使用者"，后销毁。

---

## 6. OpenGL 对照总表（速查）

| OpenGL 调用 | Vulkan 调用 | 说明 |
|---|---|---|
| `glCreateProgram` + `glAttachShader` + `glLinkProgram` | `vkCreateGraphicsPipelines` | 但 Vulkan 把固定功能状态也**一起打包**进去了 |
| `glUseProgram` + 运行时改一堆 `gl*` 状态 | pipeline 创建时一次性固化 | 改状态 = 重建管线 |
| `glDeleteShader`（链接后） | `vkDestroyShaderModule`（建管线后） | 源码容器用完即弃 |
| `glGenFramebuffers` + `glFramebufferTexture2D` | `vkCreateFramebuffer` | 把图像视图绑进 renderPass |
| `glDeleteProgram` | `vkDestroyPipeline` | |
| `glDeleteFramebuffers` | `vkDestroyFramebuffer` | |

---

## 7. 本单元最容易踩的坑

| # | 现象 | 根因 | 处理 |
|---|---|---|---|
| 1 | 编译报 `graphicsPipeline` / `swapChainFramebuffers` 未声明 | 忘了加成员变量 | 加 `VkPipeline graphicsPipeline` + `std::vector<VkFramebuffer> swapChainFramebuffers` |
| 2 | 函数名抄成单数 `vkCreateGraphicsPipeline` | 记错名字 | 是**复数** `vkCreateGraphicsPipelines` |
| 3 | 忘了第二个参数 `VK_NULL_HANDLE`（pipeline cache） | 漏看教程 | `vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, …)` |
| 4 | `renderPass` 和 `subpass` 忘了填 | 以为管线不用绑定渲染通道 | 两个都填：`renderPass` + `subpass = 0` |
| 5 | 建完管线没销毁 shader module，或又在 cleanup 里再销毁一次 | 忘了"立刻销毁"或忘了"删旧代码" | 建管线后销毁一次，**cleanup 里的两行删掉** |
| 6 | framebuffer 的 `width/height` 抄成别值 | 想当然 | `= swapChainExtent.width/height`，`layers = 1` |
| 7 | 销毁顺序反了（先 renderPass 后 pipeline） | 没理清引用关系 | 见第 5 节：framebuffer/pipeline 先，被引用者后 |
| 8 | 退出时报 `VkFramebuffer` / `VkPipeline` 泄漏 | cleanup 漏了对应的 destroy | 帧缓冲循环 + `vkDestroyPipeline` |

---

## 8. 本单元完成标志

- [ ] `createGraphicsPipeline()` 末尾补 `VkGraphicsPipelineCreateInfo` + `vkCreateGraphicsPipelines`
- [ ] 建完管线后立刻 `vkDestroyShaderModule` ×2
- [ ] 新建 `createFramebuffers()`：`resize` + 循环 `vkCreateFramebuffer`
- [ ] `initVulkan()` 里 `createRenderPass()` 之后调用 `createGraphicsPipeline()` 再 `createFramebuffers()`
- [ ] `cleanup()`：加了帧缓冲循环 + `vkDestroyPipeline`，并**删掉**了两行 `vkDestroyShaderModule`
- [ ] 能说清 `vkCreateGraphicsPipelines` 为什么是复数、第二个参数是什么
- [ ] 能说清 `renderPass` 为什么会同时出现在管线和 framebuffer 里
- [ ] 能说清"管线烤死"和 OpenGL 全局可变状态的区别
- [ ] 能说清 framebuffer 为什么每张图像一个、renderPass 却只要一个
- [ ] 能回答 `docs/ch10-check.md` 全部问题
- [ ] `PROGRESS.md` 里 D10 打勾 + 3 行总结

---

## 9. 前瞻

- **D11** 第一次出图：命令缓冲（command pool / command buffer）+ `vkCmdBeginRenderPass`
  （用上本单元的 renderPass + framebuffer + `VkClearValue`）+ 绘制命令 + 信号量 + fence +
  present。M1（彩色三角形）的画面临门一脚。
- 出图后建议**回头重做** D7 讲义第 8 节的观察实验（改 frontFace / 改 line 模式），
  以及 D8 自检题第 10 的"恒等混合"实验、D9 自检题第 10 的"loadOp"实验——都等到有画面才有意义。