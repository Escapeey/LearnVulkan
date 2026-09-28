# D7 讲义：固定功能（上）—— 顶点输入 / 输入装配 / 视口 / 光栅化

> 教程章节：[固定功能](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions)（前半）
>
> 前置：D6 完成（两个 `VkPipelineShaderStageCreateInfo` 已就绪）
> 自检题：`docs/ch07-check.md`

---

## 0. 本单元你要亲手写的

| 结构体 | 难度 | 对应 OpenGL | 本单元状态 |
|---|---|---|---|
| `VkPipelineVertexInputStateCreateInfo` | ★ | VAO + `glVertexAttribPointer` | **全空**（D13 才填） |
| `VkPipelineInputAssemblyStateCreateInfo` | ★ | `glDrawElements` 的 mode | `TRIANGLE_LIST` |
| `VkPipelineViewportStateCreateInfo` | ★★ | `glViewport` + `glScissor` | 用 `swapChainExtent` |
| `VkPipelineRasterizationStateCreateInfo` | ★★★ | `glPolygonMode` + `glCullFace` + `glFrontFace` | `cullMode=BACK` + `frontFace=CLOCKWISE` |

> 全部填在 `createGraphicsPipeline()` 里，接在 D6 的两个 shader stage 后面。
> **本单元仍不调用 `vkCreateGraphicsPipelines`** —— 这四个结构体先填好放着，D10 才真正建管线。

---

## 1. 全景：固定功能 = 把 OpenGL 的「全局状态」固化成结构体

这是整个 D7 的核心认知，一句话：

> **你在 OpenGL 里随时可改的那些 `glEnable` / `glViewport` / `glBlendFunc` 全局状态，
> 在 Vulkan 里全部变成 `VkGraphicsPipelineCreateInfo` 里的结构体字段，创建管线时一次性固化。**

### 对照 OpenGL（这一张表就是本单元的地图）

| 管线阶段 | OpenGL 你怎么设（随时可改） | Vulkan 在哪设（创建时固化） |
|---|---|---|
| 顶点格式 | `glVertexAttribPointer` + `glEnableVertexAttribArray` + VAO | `VkPipelineVertexInputStateCreateInfo` |
| 图元装配 | `glDrawElements(GL_TRIANGLES, ...)` 的 mode 参数 | `VkPipelineInputAssemblyStateCreateInfo` |
| 视口 | `glViewport(x, y, w, h)` | `VkPipelineViewportStateCreateInfo` |
| 裁剪 | `glScissor` + `glEnable(GL_SCISSOR_TEST)` | 同一个 viewport state 里的 `pScissors` |
| 光栅化 | `glPolygonMode` / `glEnable(GL_CULL_FACE)` / `glFrontFace` | `VkPipelineRasterizationStateCreateInfo` |
| （D8）多重采样 | `glEnable(GL_MULTISAMPLE)` | `VkPipelineMultisampleStateCreateInfo` |
| （D8）颜色混合 | `glEnable(GL_BLEND)` + `glBlendFunc` | `VkPipelineColorBlendStateCreateInfo` |

### 代价与收益（和 D6 讲的「不可变管线」是同一件事）

- **代价**：改一个状态 = 重建整条管线（或用 D8 要讲的 dynamic state）。
- **收益**：绘制时零状态切换开销；驱动在建管线时就能把「着色器 + 固定功能」一起编译优化。

> 反直觉但重要：**这 4 个结构体在 D7 填完后，真正被「用上」要等到 D10**。现在它们只是一堆
> 还没人读的字段。所以本单元的验收不是「跑出画面」，而是「每个字段都能说出为什么」。

---

## 2. 顶点输入 `VkPipelineVertexInputStateCreateInfo`（本单元全空）

```cpp
VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
vertexInputInfo.vertexBindingDescriptionCount   = 0;
vertexInputInfo.pVertexBindingDescriptions      = nullptr;  // optional
vertexInputInfo.vertexAttributeDescriptionCount = 0;
vertexInputInfo.pVertexAttributeDescriptions    = nullptr;  // optional
```

### 两个数组，两件事

| 数组 | 描述什么 | 例子 |
|---|---|---|
| `pVertexBindingDescriptions` | 数据**在内存里怎么排**（stride 步长、逐顶点还是逐实例） | 一个顶点占多大、隔多远 |
| `pVertexAttributeDescriptions` | 数据**喂给哪个 shader 输入**（location、格式、偏移） | `location=0` 是 `vec2` 位置 |

这就是 OpenGL 里 VAO 记录的东西：`glVertexAttribPointer` 同时声明了「绑定」和「属性」两个层面。

### 为什么本单元全是 0 / nullptr

**因为顶点数据现在还硬编码在着色器里**，没走显存：

```glsl
// shader.vert 里
vec2 positions[3] = vec2[](...);   // 数据直接写在代码里
gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);  // 用 gl_VertexIndex 索引
```

没有顶点缓冲 → 没有「绑定」；没有顶点缓冲 → 也谈不上「属性怎么从缓冲读」。所以两个数组都空。

**D13 才会填上它**（顶点缓冲章节），到时候把 `positions`/`colors` 从着色器挪进 `VkBuffer`。

> ⚠️ 但即使全空，**这个结构体本身必须存在**。管线创建时每个阶段都要给一个合法的 struct，
> 空数组也是合法的（count=0 + nullptr 表示「这个阶段不消费外部数据」）。

---

## 3. 输入装配 `VkPipelineInputAssemblyStateCreateInfo`

```cpp
VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
inputAssembly.primitiveRestartEnable = VK_FALSE;
```

### 两个字段

| 字段 | 含义 | 对照 OpenGL |
|---|---|---|
| `topology` | 顶点怎么组装成图元 | `glDrawElements(GL_TRIANGLES, ...)` 的第一个参数 |
| `primitiveRestartEnable` | 是否用特殊索引值重启一条图元带 | `glPrimitiveRestartIndex`（`GL_TRIANGLE_STRIP` 时用） |

`VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST` = **每 3 个顶点拼成一个独立的三角形**，互不共享顶点。
这是最常见的选择。其它常见的有 `TRIANGLE_STRIP`（每多一个顶点多一个三角形，省索引）、
`POINT_LIST` / `LINE_LIST`。

`primitiveRestartEnable = VK_FALSE`：我们不用 strip，也用不到「索引重启」这个特性。

> 这里只声明了「**怎么组装**」，并不画。真正触发绘制的是 D11 的 `vkCmdDraw`。
> 和 OpenGL 一样，`topology` 决定了 `vkCmdDraw(..., vertexCount)` 里那些顶点会被解释成几个三角形。

---

## 4. 视口 + 裁剪 `VkPipelineViewportStateCreateInfo`

```cpp
VkViewport viewport{};
viewport.x        = 0.0f;
viewport.y        = 0.0f;
viewport.width    = (float) swapChainExtent.width;    // 800
viewport.height   = (float) swapChainExtent.height;   // 600
viewport.minDepth = 0.0f;
viewport.maxDepth = 1.0f;

VkRect2D scissor{};
scissor.offset = {0, 0};
scissor.extent = swapChainExtent;                     // 800×600

VkPipelineViewportStateCreateInfo viewportState{};
viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
viewportState.viewportCount = 1;
viewportState.pViewports    = &viewport;
viewportState.scissorCount  = 1;
viewportState.pScissors     = &scissor;
```

### 视口 vs 裁剪：两个概念，别混

| | `VkViewport`（视口） | `VkRect2D`（裁剪） |
|---|---|---|
| 干什么 | NDC → 帧缓冲坐标的**映射** | 哪些像素**允许被写入** |
| 对照 OpenGL | `glViewport` | `glScissor` + `glEnable(GL_SCISSOR_TEST)` |
| 越界结果 | 映射出去（不丢弃） | 直接**丢弃**该像素 |

两者经常设成一样（都是整个 swapChainExtent），但职责不同。裁剪是一个「纯优化 + 正确性」机制：
`vkCmdClearAttachments` 这类操作只清除裁剪矩形内的区域。

### ⚠️ Vulkan 的深度范围是 [0, 1]，不是 OpenGL 的 [-1, 1]

`minDepth = 0.0f`、`maxDepth = 1.0f`。这是 Vulkan 与 OpenGL 的一个著名差异：

| | NDC z 范围 |
|---|---|
| OpenGL | `[-1, 1]`（默认 `glDepthRange(0,1)` 映射过去） |
| **Vulkan** | **[0, 1]** |

所以深度值 0 = 近平面，1 = 远平面。本工程的 `GLM_FORCE_DEPTH_ZERO_TO_ONE`（`CMakeLists.txt` 里）
就是让 GLM 的投影矩阵直接产出 `[0,1]` 的深度，省去手动换算。这个宏 D16 用投影矩阵时会真正起作用。

### `viewportCount` / `scissorCount` 为什么是 1

多视口（`viewportCount > 1`，比如分屏渲染）需要 `multiViewport` 特性，**你的 Intel UHD 630 不一定支持**。
本教程只用 1 个视口 + 1 个裁剪，都设 1。

> D8 会讲「dynamic state」：把 viewport / scissor 设成 `VK_DYNAMIC_STATE_VIEWPORT` /
> `VK_DYNAMIC_STATE_SCISSOR`，就能在绘制时动态改、不用重建管线。本单元先做**静态**版本
> （值直接固化在管线里），这是理解 dynamic state 的前提。

---

## 5. 光栅化 `VkPipelineRasterizationStateCreateInfo`（本单元最容易错）

```cpp
VkPipelineRasterizationStateCreateInfo rasterizer{};
rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
rasterizer.depthClampEnable        = VK_FALSE;
rasterizer.rasterizerDiscardEnable = VK_FALSE;
rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
rasterizer.lineWidth               = 1.0f;
rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
rasterizer.depthBiasEnable         = VK_FALSE;
```

### 逐个字段

| 字段 | 值 | 含义 | 对照 OpenGL |
|---|---|---|---|
| `depthClampEnable` | `VK_FALSE` | 超出近/远平面的片元是丢弃还是夹紧（clamp）。TRUE 用于阴影贴图，避免近平面裁掉 | `glEnable(GL_DEPTH_CLAMP)` |
| `rasterizerDiscardEnable` | `VK_FALSE` | TRUE = 光栅化阶段**整个跳过**（不产出片元）。用于只算不画（transform feedback） | 无直接对应 |
| `polygonMode` | `VK_POLYGON_MODE_FILL` | 三角形内部填充 | `glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)` |
| `lineWidth` | `1.0f` | 线宽，只对 `LINE` 有效；>1 需要 `wideLines` 特性 | `glLineWidth` |
| `cullMode` | `VK_CULL_MODE_BACK_BIT` | 剔除背面 | `glEnable(GL_CULL_FACE)` + `glCullFace(GL_BACK)` |
| `frontFace` | `VK_FRONT_FACE_CLOCKWISE` | 顺时针的面算正面 | `glFrontFace(GL_CW)` |
| `depthBiasEnable` | `VK_FALSE` | 深度偏移，阴影贴图防 z-fighting 用 | `glPolygonOffset` |

### ⭐ 为什么 `frontFace = CLOCKWISE`（本单元最反直觉的一点）

`shader.vert` 里三个顶点按顺序是：

```glsl
vec2( 0.0, -0.5),   // 顶点 0：下
vec2( 0.5,  0.5),   // 顶点 1：右上
vec2(-0.5,  0.5)    // 顶点 2：左上
```

在 NDC（y 轴向上）里，这个顺序是**逆时针**（CCW）的。按 OpenGL 的习惯，你会写 `frontFace = COUNTER_CLOCKWISE`。**但 Vulkan 要写 CLOCKWISE**。原因分三步：

1. Vulkan 的 NDC 和 OpenGL 一样是 **y 轴向上**；
2. 但 Vulkan 的**帧缓冲（交换链图像）原点在左上角，y 轴向下**（第 0 行扫描线在最上面）；
3. 视口变换把「y 向上的 NDC」映射到「y 向下的帧缓冲」时，**图像在竖直方向上被镜像了一次**。

这个镜像会把绕序一起反转：NDC 里的逆时针 → 屏幕上的**顺时针**。所以：

> **cullMode 剔背面 + frontFace 设顺时针，才和「NDC 里逆时针」的顶点对上。**

如果你写成 `frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE`（照 OpenGL 直觉抄），背面剔除会把
三角形当「背面」**整个剔掉 → 画面全黑，且验证层不报错**（这是合法状态，只是渲染结果不对）。

> 顺带：有些教程用 `viewport.height = -(float)extent.height`（负高度）在视口处就把 Y 翻回来，
> 这样 frontFace 就能写 `COUNTER_CLOCKWISE`。**经典 Vulkan 教程不用这招**——它保持视口高度为正，
> 在 D16 的投影矩阵里统一翻 Y（`proj[1][1] *= -1`）。两条路殊途同归，本工程跟经典教程：**正高度 + 投影矩阵翻转**。
> 所以 D7 这里记住：**视口高度为正，frontFace 写 CLOCKWISE**。

---

## 6. OpenGL 对照总表（速查）

| OpenGL 调用 | Vulkan 字段 | 所在结构体 |
|---|---|---|
| `glVertexAttribPointer` | `pVertexAttributeDescriptions` | `VkPipelineVertexInputStateCreateInfo` |
| VAO 绑定 | `pVertexBindingDescriptions` | `VkPipelineVertexInputStateCreateInfo` |
| `glDrawElements(mode, ...)` | `topology` | `VkPipelineInputAssemblyStateCreateInfo` |
| `glViewport` | `viewport`（`VkViewport`） | `VkPipelineViewportStateCreateInfo` |
| `glScissor` + `glEnable(GL_SCISSOR_TEST)` | `scissor`（`VkRect2D`） | `VkPipelineViewportStateCreateInfo` |
| `glDepthRange(0,1)` vs `[0,1]` | `minDepth` / `maxDepth` | `VkViewport` |
| `glPolygonMode(GL_FILL)` | `polygonMode` | `VkPipelineRasterizationStateCreateInfo` |
| `glEnable(GL_CULL_FACE)` + `glCullFace(GL_BACK)` | `cullMode` | `VkPipelineRasterizationStateCreateInfo` |
| `glFrontFace(GL_CW)` | `frontFace` | `VkPipelineRasterizationStateCreateInfo` |

---

## 7. 本单元最容易踩的坑

| # | 现象 | 根因 | 处理 |
|---|---|---|---|
| 1 | D11 出图后**全黑**，验证层零报错 | `frontFace` 抄成了 `COUNTER_CLOCKWISE`，背面剔除把三角形剔光 | 改成 `VK_FRONT_FACE_CLOCKWISE` |
| 2 | 编译报「未声明的标识符」 | 类型名写成小写 `vk...`（应为 `Vk...`） | 见 D6 的老坑，`Vk` 大写 |
| 3 | 视口/裁剪设置被忽略 | `viewportCount`/`scissorCount` 写 0，或 `pViewports`/`pScissors` 悬垂 | 都写 1，指针指向局部变量（作用域覆盖创建调用） |
| 4 | `minDepth`/`maxDepth` 写成 `-1`/`1` | 沿用 OpenGL 的 z 范围 | Vulkan 是 `0.0f` / `1.0f` |
| 5 | 线框模式不生效 | `polygonMode` 设 `LINE` 但忘了 `lineWidth` 受 `wideLines` 限制（>1 需特性） | 线宽保持 `1.0f` |

---

## 8. 🔬 建议你自己做的两个观察实验

1. **把 `frontFace` 改成 `VK_FRONT_FACE_COUNTER_CLOCKWISE`**，编译运行（D11 出图后再做）。
   预期：画面全黑，验证层不报错。改回来。
   —— 亲身体会「Vulkan 的合法状态 ≠ 正确画面」。
2. **把 `polygonMode` 改成 `VK_POLYGON_MODE_LINE`**（暂时关掉 culling：`cullMode = VK_CULL_MODE_NONE`），
   看线框三角。再改回 `FILL`。
   —— 体会「改一个渲染状态 = 重建管线」意味着什么（这俩值都是管线固化状态，改了必须重编译管线）。

---

## 9. 本单元完成标志

- [ ] 四个结构体在 `createGraphicsPipeline()` 里填好，顺序：vertexInput → inputAssembly → viewportState → rasterizer
- [ ] `viewport`/`scissor` 用的是 `swapChainExtent`，深度范围 `[0,1]`
- [ ] `frontFace = CLOCKWISE`、`cullMode = BACK`，能说清为什么
- [ ] 能回答：这四个结构体分别替代了 OpenGL 里的哪些调用
- [ ] 能说出视口和裁剪的区别
- [ ] 完成第 8 节的两个观察实验并记录
- [ ] 能回答 `docs/ch07-check.md` 全部问题
- [ ] `PROGRESS.md` 里 D7 打勾 + 3 行总结

---

## 10. 前瞻

- **D8** 补其余固定功能：multisampling、color blending、depth-stencil（占位）、dynamic state、以及 `VkPipelineLayout`（它和 D15 的描述符集布局的关系是重点）。
- **D9** 建渲染通道 `vkCreateRenderPass`。
- **D10** 终于把 D6~D8 攒的所有结构体喂给 **`vkCreateGraphicsPipelines`** —— 全教程最关键的一次调用。
- M1（彩色三角形）在 **D11** 第一次出图。
