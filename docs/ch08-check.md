# D8 自检题

> 合上代码和教程先自己答。全部答得出，再把 D8 打勾。

---

### 1. 全景：D8 与 D7 的性质差异

1. D7 填的 4 个结构体和 D8 的 `VkPipelineLayout` 相比，本质区别是什么？
2. D8 有哪些东西是"**纯结构体**"，哪些是"**真正的对象**（有句柄、要销毁）"？
3. 本单元结束后能出画面吗？为什么？

---

### 2. 多重采样 `VkPipelineMultisampleStateCreateInfo`

1. `rasterizationSamples` 本单元填什么？为什么？
2. 真正的 MSAA 要到 D 几才开？届时别的哪些东西也要跟着改（举 1~2 个）？
3. `sampleShadingEnable` 是干什么的？它和 `rasterizationSamples` 是什么关系？
4. 对应 OpenGL 的哪个调用？

---

### 3. 深度 + 模板占位（本单元最绕）

1. `depthTestEnable` 填 `VK_TRUE` 还是 `VK_FALSE`？`depthCompareOp` 填什么？
2. **现在还没有深度缓冲，为什么填 `VK_TRUE` 不会出错、也不会把画面画错？**
3. 深度缓冲这个资源要到 D 几才真正加进来？届时这里需要改吗？
4. `depthWriteEnable` 对应 OpenGL 的哪个调用？

<details>
<summary>提示</summary>
第 2 问：Vulkan 规范里"帧缓冲没有深度附件时，深度测试一律通过、写入一律跳过"。这叫「先声明、后兑现」。
</details>

---

### 4. 颜色混合（两个结构体，易漏）

1. `VkPipelineColorBlendAttachmentState` 和 `VkPipelineColorBlendStateCreateInfo` 各管什么？为什么要两个？
2. 写出（或说清）`blendEnable = VK_TRUE` 时的混合公式，并代入 `ONE / ZERO / ADD` 说明它等于什么。
3. `blendEnable = VK_FALSE` 时，我们仍把 factor 填成 `ONE / ZERO`，为什么？
4. 要实现 OpenGL 的 `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)` 透明混合，`srcColorBlendFactor` / `dstColorBlendFactor` 该填什么？

---

### 5. `logicOpEnable` 与 `colorWriteMask`

1. `logicOpEnable` 是什么？为什么本单元 `VK_FALSE`？
2. `colorWriteMask` 填了哪些位？它对应 OpenGL 的哪个调用？

---

### 6. 动态状态（OpenGL 没有的概念）

1. "静态状态"和"动态状态"的区别是什么？各自的代价 / 收益？
2. 本单元为什么把 `dynamicStateCount` 设为 0？
3. 如果想把 viewport 改成动态，要改哪两处？（提示：一处是 `pDynamicStates` 加 `VK_DYNAMIC_STATE_VIEWPORT`，另一处在 viewportState 里）
4. 为什么说 dynamic state 是 Vulkan 里"最接近 OpenGL 全局状态"的机制？

<details>
<summary>提示</summary>
第 3 问的第二处：声明成 dynamic 的状态就不能再静态设值，viewportState 的 `viewportCount` 要变成 0。
</details>

---

### 7. ⭐ 管线布局 `VkPipelineLayout`（本单元核心）

1. `VkPipelineLayout` 声明的是"什么"？（哪两类资源）
2. `setLayoutCount = 0` 是因为"偷懒"还是"诚实"？三角形着色器现在真的不用描述符吗？
3. 用「类型 / 接线 / 实例」的类比，说清 `VkDescriptorSetLayout`（D15）、`VkPipelineLayout`（D8）、`VkDescriptorSet`（D16）三者的关系。
4. D15 加了 MVP 的 UBO 之后，这里的 `setLayoutCount` / `pSetLayouts` 要改成什么？

<details>
<summary>提示</summary>
第 3 问对应讲义第 6 节里的那张三层表。第 4 问：`setLayoutCount = 1` 并指向新建的 `VkDescriptorSetLayout`。
</details>

---

### 8. 生命周期 / 依赖

1. `pipelineLayout` 为什么必须做成**成员变量**，而不是局部变量？
2. `cleanup()` 里加什么？相对 `vkDestroyDevice` 的顺序是什么？
3. `colorBlendAttachment` 是局部变量，`colorBlending.pAttachments` 指向它——这和 D3 的 `queuePriority`、D7 的 `pViewports` 是同一类什么约束？

---

### 9. 对号入座：固定功能完整顺序

从顶点进来到管线布局，按顺序写出固定功能阶段（D7 的 4 个 + D8 的 5 个），并各说一个关键字段。

（提示：vertex input → input assembly → viewport/scissor → rasterization → multisampling → depth/stencil → color blending → dynamic state → pipeline layout）

---

### 10. 动手题：恒等混合实验

（**等到 D11 出图后做**）把 `colorBlendAttachment.blendEnable` 从 `VK_FALSE` 改成 `VK_TRUE`
（factor 保持 `ONE / ZERO / ADD`），编译运行。

```
画面变了吗？为什么？
改回 VK_FALSE。
```

---

### 11. 延伸思考

1. 为什么 Vulkan 要提供 dynamic state 这样一个"反固化"的机制？如果所有状态都静态，做一个
   "F3 切换线框模式"的功能要付出什么代价？dynamic state 又怎么简化它？
2. Vulkan 把 OpenGL 里"隐式"的 uniform 绑定拆成了 layout → pipeline layout → set 三段显式，
   这样做的代价（代码量）和收益（驱动能提前校验/优化）分别是什么？

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