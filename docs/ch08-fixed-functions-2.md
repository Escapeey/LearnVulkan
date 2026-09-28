# D8 讲义：固定功能（下）—— 多重采样 / 深度+模板 / 颜色混合 / 动态状态 / 管线布局

> 教程章节：[固定功能](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions)（后半）
>
> 前置：D7 完成（顶点输入 / 输入装配 / 视口+裁剪 / 光栅化 四个结构体已就绪）
> 自检题：`docs/ch08-check.md`

---

## 0. 本单元你要亲手写的

| 结构体 / 对象 | 难度 | 对应 OpenGL | 本单元状态 |
|---|---|---|---|
| `VkPipelineMultisampleStateCreateInfo` | ★ | `glEnable(GL_MULTISAMPLE)` | `SAMPLE_COUNT_1`（无 MSAA） |
| `VkPipelineDepthStencilStateCreateInfo` | ★★ | `glEnable(GL_DEPTH_TEST)` + `glDepthFunc` | 占位：depthTest 开，深度缓冲 D19 才补 |
| `VkPipelineColorBlendStateCreateInfo`（+`AttachmentState`） | ★★★ | `glEnable(GL_BLEND)` + `glBlendFunc` | 不混合（直接覆盖） |
| `VkPipelineDynamicStateCreateInfo` | ★★ | （OpenGL 无对应） | **留空** |
| `VkPipelineLayout` | ★★★ | （OpenGL 无显式对应） | `vkCreatePipelineLayout` 创建 |

> 前 4 个是**纯结构体**（和 D7 一样只填字段、没人用）；最后 1 个 `VkPipelineLayout` 是
> **本单元第一次真正"创建"的 Vulkan 对象**——有句柄、要 `vkDestroyPipelineLayout` 销毁。
>
> 同样**不调用 `vkCreateGraphicsPipelines`**——D10 才把 D6~D8 攒的所有东西一次性建管线。

---

## 1. 全景：D7 填了 4 个结构体，D8 填完剩下的 + 第一次 create

对比一下 D7 和 D8 的性质差异，这是本单元最重要的认知：

| | D7 | D8 |
|---|---|---|
| 干了什么 | 填 4 个纯结构体（数据） | 填 4 个纯结构体 + **创建 1 个对象** |
| 有没有句柄 / 要销毁 | 无 | `VkPipelineLayout` 有，`cleanup()` 里销毁 |
| 什么时候被用上 | D10 | D10（layout 也是 D10 建管线时传进去） |

一句话概括主线：

> **固定功能（下）= 把管线剩余几个阶段的「多重采样 / 深度 / 混合 / 动态状态」填完，
> 再用一个「空的」管线布局，把"着色器将来要访问哪些资源"这件事先占个位。**

那个"空的管线布局"（`setLayoutCount = 0`）就是本单元和第 6 节的真正重点——它是通向
D15 描述符集布局的桥。

---

## 2. 多重采样 `VkPipelineMultisampleStateCreateInfo`

```cpp
VkPipelineMultisampleStateCreateInfo multisampling{};
multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
multisampling.sampleShadingEnable  = VK_FALSE;
multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
multisampling.minSampleShading     = 1.0f;      // Optional
multisampling.pSampleMask          = nullptr;   // Optional
multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
multisampling.alphaToOneEnable     = VK_FALSE;  // Optional
```

### 逐个字段

| 字段 | 值 | 含义 |
|---|---|---|
| `rasterizationSamples` | `VK_SAMPLE_COUNT_1_BIT` | 每像素采样 **1** 次 = **不开 MSAA** |
| `sampleShadingEnable` | `VK_FALSE` | 采样着色（每个采样点都跑一遍片元着色器）关闭。只在 MSAA 下有意义 |
| `minSampleShading` | `1.0f` | 采样着色的最小覆盖率，`sampleShadingEnable=FALSE` 时被忽略 |
| `pSampleMask` | `nullptr` | 按采样点掩码丢弃，`nullptr` = 不丢弃 |
| `alphaToCoverageEnable` / `alphaToOneEnable` | `VK_FALSE` | 透明度的 alpha-to-coverage / alpha-to-one，关 |

> **为什么现在 `SAMPLE_COUNT_1`**：MSAA 需要额外的工作（D22 要建多样本 color buffer、
> resolve 目标、改 render pass），现在先跑通基础管线再说。届时这一行改成
> `VK_SAMPLE_COUNT_4_BIT`（前提是 `vkGetPhysicalDeviceProperties` 里 frameBufferColorSamples 支持）。
> 对照 OpenGL：`glEnable(GL_MULTISAMPLE)`，但 OpenGL 里 MSAA 是"全局开关"，Vulkan 把它固化成
> 每像素采样数、直接写进管线。

---

## 3. 深度 + 模板 `VkPipelineDepthStencilStateCreateInfo`（占位）

```cpp
VkPipelineDepthStencilStateCreateInfo depthStencil{};
depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
depthStencil.depthTestEnable       = VK_TRUE;
depthStencil.depthWriteEnable      = VK_TRUE;
depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
depthStencil.depthBoundsTestEnable = VK_FALSE;
depthStencil.minDepthBounds        = 0.0f;      // Optional
depthStencil.maxDepthBounds        = 1.0f;      // Optional
depthStencil.stencilTestEnable     = VK_FALSE;
depthStencil.front                 = {};        // Optional（模板用，本教程不用）
depthStencil.back                  = {};        // Optional
```

### ⭐ "depthTest 开了，但还没有深度缓冲"是怎么回事

这是本结构体最绕的一点，三步想清楚：

1. **`depthTestEnable = VK_TRUE`**：管线"声明"它要做深度测试，比较规则 `LESS`（新片元深度 < 旧深度才通过）。
2. **但 D9 建的 render pass 此刻没有深度附件**（只有 color 附件）。深度缓冲这个 `VkImage` 是 **D19** 才加的。
3. **Vulkan 规范**：当帧缓冲没有深度附件时，深度测试一律"通过"、深度写入一律跳过。

所以现在的 `VK_TRUE` 是**"先声明、后兑现"**：D11~D18 期间它形同虚设（反正没深度缓冲可测），
D19 加上深度附件后，**这里一个字都不用改，立刻生效**。

> 这就是 OpenGL 里 `glEnable(GL_DEPTH_TEST)` + `glDepthFunc(GL_LESS)` + `glDepthMask(GL_TRUE)`
> 的固化版。差别只在：OpenGL 你随时能开关深度，Vulkan 把它写死在管线里，且深度范围配套是 D7 讲的 `[0,1]`。

### 其余字段（本单元全关）

| 字段 | 值 | 含义 |
|---|---|---|
| `depthBoundsTestEnable` | `VK_FALSE` | 按 `minDepthBounds`/`maxDepthBounds` 区间额外过滤片元，关 |
| `stencilTestEnable` | `VK_FALSE` | 模板测试，关（本教程全程不用模板） |
| `front` / `back` | `{}` | 模板操作的正面/背面参数，不用所以留空 |

---

## 4. 颜色混合 `VkPipelineColorBlendStateCreateInfo`（两个结构体）

本阶段**必须给两个结构体**：一个"每个附件一份"的 `VkPipelineColorBlendAttachmentState`，
一个"全局"的 `VkPipelineColorBlendStateCreateInfo`。

```cpp
// (a) 每个附件一份 —— 我们只有 1 个 color 附件（交换链图像）
VkPipelineColorBlendAttachmentState colorBlendAttachment{};
colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                    | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
colorBlendAttachment.blendEnable         = VK_FALSE;
colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;       // Optional
colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;       // Optional

// (b) 全局
VkPipelineColorBlendStateCreateInfo colorBlending{};
colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
colorBlending.logicOpEnable   = VK_FALSE;
colorBlending.logicOp         = VK_LOGIC_OP_COPY;  // Optional
colorBlending.attachmentCount = 1;
colorBlending.pAttachments    = &colorBlendAttachment;
colorBlending.blendConstants[0] = 0.0f;  // Optional
colorBlending.blendConstants[1] = 0.0f;
colorBlending.blendConstants[2] = 0.0f;
colorBlending.blendConstants[3] = 0.0f;
```

### 混合公式（`blendEnable = VK_TRUE` 时才用，本单元 FALSE）

```
finalColor.rgb = srcFactor * srcColor.rgb  ⊕  dstFactor * dstColor.rgb
finalColor.a   = srcAlphaFactor * srcAlpha ⊕  dstAlphaFactor * dstAlpha
                    （⊕ 就是 colorBlendOp / alphaBlendOp，通常都是 ADD）
```

现在填的 `ONE / ZERO / ADD` 代入公式 = `src * 1 + dst * 0 = src`，也就是"**恒等混合**"——
万一以后把 `blendEnable` 翻成 `VK_TRUE`，这个组合等于不混合。所以现在虽然 `blendEnable = VK_FALSE`
（三角形直接覆盖旧颜色），这些 Optional 字段也填成了恒等值，**为将来开混合预留好**。

### 对照 OpenGL

| OpenGL | Vulkan 字段 |
|---|---|
| `glEnable(GL_BLEND)` | `blendEnable = VK_TRUE` |
| `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)` | `srcColorBlendFactor = SRC_ALPHA` + `dstColorBlendFactor = ONE_MINUS_SRC_ALPHA` |
| `glBlendFuncSeparate(...)`（rgb 和 a 分开） | `srcColorBlendFactor`/`srcAlphaBlendFactor` 两组 |
| `glBlendEquation(GL_FUNC_ADD)` | `colorBlendOp = ADD` |
| `glColorMask(...)` | `colorWriteMask`（按通道开关写） |

> 本工程当前**不做透明混合**（三角形是实心色，直接写覆盖）。D18 贴图后也保持关闭，
> 教程只在讲到 alpha blending 概念时提及，最终渲染器若做透明物体会用到。

### `logicOpEnable` 是什么

一种"不用混合公式、改用按位逻辑运算（AND/OR/XOR/COPY…）"的替代混合方式。几乎不用，
且需要 `logicOp` 特性 + 帧缓冲格式支持。我们 `VK_FALSE`，`logicOp = COPY` 只是占位。

---

## 5. 动态状态 `VkPipelineDynamicStateCreateInfo`（本单元留空）

```cpp
VkPipelineDynamicStateCreateInfo dynamicState{};
dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
dynamicState.dynamicStateCount = 0;
dynamicState.pDynamicStates    = nullptr;
```

### 静态 vs 动态

D7 把 viewport / scissor 的值**固化**进了管线（静态状态）。`dynamicState` 则声明：
"这几种状态我**不**在管线里写死，而是绘制时用 `vkCmdSetViewport` / `vkCmdSetScissor`
临时改"。

| | 静态（D7 的做法） | 动态 |
|---|---|---|
| 改状态代价 | 重建整条管线 | 一条 `vkCmdSet*` 命令 |
| 驱动优化 | 更充分 | 稍弱 |
| 对照 OpenGL | Vulkan 独有 | **最接近 OpenGL 的"随时可改"全局状态** |

> OpenGL 没有 dynamic state 这个概念，因为它的状态本来就随时可变。dynamic state 是 Vulkan
> 为了"部分状态想要 OpenGL 式的灵活性"而留的口子——你想灵活哪个，就把哪个声明成 dynamic。

### 本单元为什么留空

教程选择**完全静态**：viewport/scissor 都固定成整个 `swapChainExtent`，够用了，不需要运行时改。
所以 `dynamicStateCount = 0`。**但结构体本身仍要传给管线创建**（D10），空数组是合法的。

> ⚠️ 一个隐藏规则：**声明成 dynamic 的状态，就不能再静态设值**。例如把
> `VK_DYNAMIC_STATE_VIEWPORT` 放进 `pDynamicStates`，那么 viewportState 的 `viewportCount`
> 必须为 0（`pViewports = nullptr`），改由 `vkCmdSetViewport` 在命令缓冲里设。两者互斥。

---

## 6. 管线布局 `VkPipelineLayout`（本单元核心）

```cpp
VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount         = 0;         // Optional
pipelineLayoutInfo.pSetLayouts            = nullptr;   // Optional
pipelineLayoutInfo.pushConstantRangeCount = 0;         // Optional
pipelineLayoutInfo.pPushConstantRanges    = nullptr;   // Optional

if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
}
```

### 这是本单元第一次"创建"对象

D6~D8 前面攒的一堆 `Vk*CreateInfo` 都是**纯数据结构**（填完放在栈上，没人读）。
`vkCreatePipelineLayout` 是第一个**真正申请资源、返回句柄**的调用。所以：

- `pipelineLayout` 必须做成**成员变量**（`VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;`），
  从 D8 活到管线销毁；
- `cleanup()` 里 `vkDestroyPipelineLayout(device, pipelineLayout, nullptr)`。

### 它声明什么

`VkPipelineLayout` 回答一个问题：**"这条管线的着色器，能访问哪些资源？"** 具体两类：

1. **描述符集**（`pSetLayouts`）：uniform 缓冲、纹理采样器等，按 set 槽位排。
2. **推送常量**（`pPushConstantRanges`）：直接内嵌在命令流里的小块常量。

现在三角形着色器**不读任何 uniform / 纹理**，所以 `setLayoutCount = 0`、`pushConstantRangeCount = 0`。

### ⭐ 与 D15 描述符集布局的关系（本单元的认知重点）

这是 ROADMAP 明确要求想清楚的一段。三层，从上到下：

| 对象 | 出现的单元 | 角色 | 类比 |
|---|---|---|---|
| `VkDescriptorSetLayout` | D15 | 描述"**一个描述符集长什么样**"（binding 0 = UBO，binding 1 = 采样器…） | 一份**类型 / 签名** |
| `VkPipelineLayout` | **D8（本单元）** | 声明"**这条管线在哪些 set 槽位用哪些 layout**" + push constant | **接线图**：函数调用点接了哪些签名 |
| `VkDescriptorSet` | D16 | 真正绑上 `VkBuffer` / `VkImageView` 的**实例** | 一份**变量 / 实例** |

对应到 OpenGL：uniform 的"布局"是**隐式**的——`glGetUniformLocation` + `glBindBufferBase` +
`glActiveTexture` 在运行时拼起来。Vulkan 把它拆成**显式三段**：先声明 layout（D15），
管线创建时声明"用哪些 layout"（D8 的 `pSetLayouts`，现在空），绘制时绑定真正的 set（D16）。

> 所以现在 `setLayoutCount = 0` 不是"偷懒"，而是"诚实"：三角形着色器此刻确实不用任何描述符。
> D15 一加 UBO（MVP 矩阵），这里就得改成 `setLayoutCount = 1` 并指向那个 `VkDescriptorSetLayout`。
> 届时你会体会到：**管线布局是连接"着色器声明"和"应用供给"的那根线**，D8 先把它空着接好。

---

## 7. OpenGL 对照总表（速查）

| OpenGL 调用 | Vulkan 字段 | 所在结构体 |
|---|---|---|
| `glEnable(GL_MULTISAMPLE)` | `rasterizationSamples` | `VkPipelineMultisampleStateCreateInfo` |
| `glEnable(GL_DEPTH_TEST)` | `depthTestEnable` | `VkPipelineDepthStencilStateCreateInfo` |
| `glDepthFunc(GL_LESS)` | `depthCompareOp = VK_COMPARE_OP_LESS` | `VkPipelineDepthStencilStateCreateInfo` |
| `glDepthMask(GL_TRUE)` | `depthWriteEnable` | `VkPipelineDepthStencilStateCreateInfo` |
| `glEnable(GL_BLEND)` | `blendEnable` | `VkPipelineColorBlendAttachmentState` |
| `glBlendFunc(SRC_ALPHA, ONE_MINUS_SRC_ALPHA)` | `src/dstColorBlendFactor` | `VkPipelineColorBlendAttachmentState` |
| `glBlendEquation(ADD)` | `colorBlendOp` / `alphaBlendOp` | `VkPipelineColorBlendAttachmentState` |
| `glColorMask(...)` | `colorWriteMask` | `VkPipelineColorBlendAttachmentState` |
| （无 —— 状态本就可变） | `pDynamicStates` | `VkPipelineDynamicStateCreateInfo` |
| （无 —— 隐式链接） | `pSetLayouts` / `pPushConstantRanges` | `VkPipelineLayout` |

---

## 8. 本单元最容易踩的坑

| # | 现象 | 根因 | 处理 |
|---|---|---|---|
| 1 | 编译报 `pipelineLayout` 未声明 | 忘了加成员变量 `VkPipelineLayout pipelineLayout` | 加成员 + `= VK_NULL_HANDLE` |
| 2 | 退出时报 `VkPipelineLayout` 泄漏 | 忘了在 `cleanup()` 里 `vkDestroyPipelineLayout` | 在 `vkDestroyDevice` 之前加 |
| 3 | `pAttachments` 悬垂 / count 写 0 | `colorBlendAttachment` 是局部变量，或忘了 `attachmentCount = 1` | `attachmentCount = 1`，指针指向仍在作用域内的局部变量 |
| 4 | 把 `depthTestEnable` 抄成 `VK_FALSE` | 想当然"现在没深度缓冲所以关掉" | 照教程填 `VK_TRUE`，见第 3 节的"先声明后兑现" |
| 5 | dynamic state 和静态值**同时设** | 把 viewport 声明成 dynamic 却又在 viewportState 里填了值 | 二选一：要么静态、要么 dynamic（count=0） |
| 6 | 漏了 `VkPipelineColorBlendAttachmentState` | 只建了全局 `colorBlending` | 两个结构体都要，全局的 `pAttachments` 指向附件数组 |

---

## 9. 本单元完成标志

- [ ] 五个部分在 `createGraphicsPipeline()` 里接在 rasterizer 后：multisampling → depthStencil → colorBlending → dynamicState → pipelineLayout
- [ ] `vkCreatePipelineLayout` 调用成功，`pipelineLayout` 是成员变量
- [ ] `cleanup()` 里 `vkDestroyPipelineLayout` 已加（在 `vkDestroyDevice` 之前）
- [ ] 能说清 `depthTestEnable = VK_TRUE` 但现在没有深度缓冲为什么不出问题
- [ ] 能写出颜色混合公式，并说明 `ONE/ZERO/ADD` 是"恒等混合"
- [ ] 能说清 `VkPipelineLayout` 与 D15 `VkDescriptorSetLayout` 的关系（类型 / 接线 / 实例）
- [ ] 能回答 `docs/ch08-check.md` 全部问题
- [ ] `PROGRESS.md` 里 D8 打勾 + 3 行总结

---

## 10. 前瞻

- **D9** 建渲染通道 `vkCreateRenderPass`（subpass / attachment / dependency 三段式）。
- **D10** 终于把 D6~D8 攒的所有结构体（含本单元的 `pipelineLayout`）喂给 **`vkCreateGraphicsPipelines`** —— 全教程最关键的一次调用。
- M1（彩色三角形）在 **D11** 第一次出图。届时建议重做 D7 讲义第 8 节的两个观察实验。
