# D7 自检题

> 合上代码和教程先自己答。全部答得出，再把 D7 打勾。

---

### 1. 固定功能 vs 可编程（全景）

1. 图形管线的阶段里，哪几个是**固定功能**阶段（本单元覆盖的 4 个）？
2. OpenGL 里设置它们的调用是「随时可改的全局状态」，Vulkan 里变成了什么？
3. 这个「固化」带来的**代价**和**收益**各是什么？

---

### 2. 顶点输入 `VkPipelineVertexInputStateCreateInfo`

它有两个数组：`pVertexBindingDescriptions` 和 `pVertexAttributeDescriptions`。

1. 这两个数组各描述什么？举一个字段说明区别。
2. 对应 OpenGL 里的哪个概念 / 哪组调用？
3. **为什么本单元两个数组都是空（count=0）？** 数据现在在哪？
4. 那这个空结构体还有必要填吗？D 几会被真正填上？

<details>
<summary>提示</summary>
第 3 问去看 `shader.vert`，数据还在着色器里硬编码。第 4 问对应 D13 顶点缓冲。
</details>

---

### 3. 输入装配 `VkPipelineInputAssemblyStateCreateInfo`

1. `topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST` 的含义是什么（用顶点数说）？
2. 对应 OpenGL 的哪个调用的哪个参数？
3. `primitiveRestartEnable` 是干什么的？为什么我们设 `VK_FALSE`？
4. 说出至少 2 个其它 `topology` 选项。

---

### 4. 视口 vs 裁剪（易混点）

`VkPipelineViewportStateCreateInfo` 里同时有 `pViewports` 和 `pScissors`。

1. 视口（`VkViewport`）和裁剪（`VkRect2D`）各是干什么的？核心区别是什么？
2. 分别对应 OpenGL 的哪个调用？
3. 如果裁剪矩形比视口小一半，画出来的三角形会怎样？谁被丢掉了？

<details>
<summary>提示</summary>
视口 = NDC → 帧缓冲坐标的**映射**；裁剪 = 哪些像素**允许被写入**（越界即丢弃）。
</details>

---

### 5. 深度范围（OpenGL 著名差异）

1. `VkViewport` 的 `minDepth` / `maxDepth` 应填什么？OpenGL 对应默认值是什么？
2. 为什么会有这个差异？本工程用哪个宏让 GLM 直接产出正确的深度范围？

---

### 6. `viewportCount` / `scissorCount`

1. 为什么都填 `1`？
2. 想用多个视口（分屏渲染）需要什么？你的 Intel UHD 630 一定支持吗？

---

### 7. 光栅化字段填空

写出 `VkPipelineRasterizationStateCreateInfo` 里每个字段，并对应到 OpenGL 调用：

| 字段 | 我们填的值 | 对应 OpenGL |
|---|---|---|
| `depthClampEnable` | | |
| `rasterizerDiscardEnable` | | |
| `polygonMode` | | |
| `lineWidth` | | |
| `cullMode` | | |
| `frontFace` | | |
| `depthBiasEnable` | | |

<details>
<summary>提示</summary>
`lineWidth` 只在 `LINE` 模式下有意义；>1 需要 `wideLines` 特性。
</details>

---

### 8. ⭐ 为什么 `frontFace = CLOCKWISE`（本单元核心）

`shader.vert` 里三个顶点按顺序是 `(0,-0.5)` → `(0.5,0.5)` → `(-0.5,0.5)`。

1. 这三个顶点在 NDC（y 轴向上）里是顺时针还是逆时针？
2. Vulkan 的帧缓冲（交换链图像）原点在哪？y 轴朝哪？
3. 视口变换做这个「y 向镜像」时，绕序发生了什么变化？
4. 所以 `frontFace` 要设成什么？`cullMode` 要设成什么？
5. 如果照 OpenGL 直觉写成 `COUNTER_CLOCKWISE`，D11 出图后会看到什么？验证层会报错吗？

<details>
<summary>提示</summary>
第 5 问的答案很反直觉：**合法状态 ≠ 正确画面**，验证层会保持沉默。
</details>

---

### 9. 生命周期 / 依赖

1. 这 4 个结构体填好后，**什么时候才真正被「用上」**？（哪次调用）
2. 它们现在作为局部变量，作用域有什么要求？（提示：和 D3 的 `queuePriority` 是同一类问题）
3. D7 结束时能出画面吗？为什么？

---

### 10. 动手题：两个观察实验

（**需要等到 D11 出图后再做**，做完记录到讲义第 8 节。）

**实验 1：`frontFace` 改成 `COUNTER_CLOCKWISE`**
```
画面变成什么样了？
验证层说了什么？
```

**实验 2：`polygonMode` 改成 `LINE` + `cullMode` 改成 `NONE`**
```
画面变成什么样了？
改回 FILL 之后，你对「改状态 = 重建管线」有什么体会？
```

---

### 11. 延伸思考

1. 视口变换的 Y 镜像，本教程选择在 D16 的投影矩阵里统一翻回来（`proj[1][1] *= -1`）。
   另一种常见做法是 `viewport.height` 取负值。这两种做法各自的**影响范围**是什么？
   为什么「在投影矩阵里翻」对后续的 D19 深度缓冲更省心？
2. 一个游戏要支持「线框模式」调试视图（按 F3 切换）。用本单元学的静态管线，你会怎么实现？
   D8 要讲的 dynamic state 又能怎么简化它？

---

## 答题记录

| 题号 | 第一次是否答对 | 需要复习的点 |
|---|---|---|
| 1 | | |
| 2 | | |
| 3 | | |
| 4 | | |
| 5 | | |
| 6 | | |
| 7 | | |
| 8 | | |
| 9 | | |
| 10 | | |
| 11 | | |
