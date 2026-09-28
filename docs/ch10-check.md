# D10 自检题

> 合上代码和教程先自己答。全部答得出，再把 D10 打勾。

---

### 1. 全景：本单元两件事

1. D10 做的两件事分别是什么？各用一个动词概括。
2. 为什么说 D6~D9 一直在"备料、写配方"，D10 是"开火炒菜"？
3. `vkCreateGraphicsPipelines` 为什么被 ROADMAP 称为"全教程最关键的一次调用"？

---

### 2. ⭐ 图形管线 `vkCreateGraphicsPipelines`

1. 函数名为什么是**复数**的 `vkCreateGraphicsPipelines`？第二个参数 `VK_NULL_HANDLE` 是什么？
2. `VkGraphicsPipelineCreateInfo` 里的 `pStages`、那一串 `p*State` 指针、`layout`、`renderPass`
   分别来自 D 几、是什么类型（指针 or 值）？
3. `subpass = 0` 里的 `0` 指的是什么？为什么填 0？

<details>
<summary>提示</summary>
第 3 问：看 D9 的 `VkRenderPassCreateInfo.subpassCount = 1`，我们只定义了一个子通道，下标 0。
</details>

---

### 3. ⭐ 管线是"烤死的"

1. `vkCreateGraphicsPipelines` 之后，管线的哪些东西不能再改？想改混合模式（比如开透明混合）要付出什么代价？
2. 唯一能让某些状态"运行时可变"的机制是什么？D 几讲过？
3. 对照 OpenGL：`glEnable(GL_BLEND)` 随时能开，Vulkan 为什么偏要"重建管线"？驱动从"烤死"里得到什么好处？

<details>
<summary>提示</summary>
第 2 问：D8 的 dynamic state（`VkPipelineDynamicStateCreateInfo`）。第 3 问：驱动能把整条管线提前编译、做激进优化，不必在每次状态切换时兜底。
</details>

---

### 4. shader module「立刻销毁」

1. 建完管线后为什么要 `vkDestroyShaderModule`？它对应 OpenGL 的哪个调用？
2. D6 为什么当初不能销毁、要留到 D10 才销毁？
3. `cleanup()` 里 D6 加的那两行 `vkDestroyShaderModule` 现在要怎么处理？为什么？

---

### 5. ⭐ renderPass 出现在两个地方

1. `renderPass` 同时被填进哪两个结构体？各表达什么意思？
2. "管线说我要在第 0 号 subpass 里画" vs "framebuffer 说我就是那个 renderPass 的实例"——这两句分别对应哪一行代码？
3. 如果管线填了 renderPass A、framebuffer 用的是 renderPass B，会发生什么？

---

### 6. 帧缓冲 `createFramebuffers()`

1. `swapChainFramebuffers` 为什么要先 `resize`？它 resize 到的长度是什么？
2. `VkFramebufferCreateInfo` 里 `width/height` 填什么？`layers` 为什么是 1？
3. 一个 framebuffer 绑几个附件？这几个附件是哪来的？

---

### 7. ⭐ renderPass vs framebuffer（数量）

1. renderPass 只有 1 个，framebuffer 却每个交换链图像 1 个——为什么数量不同？
2. 用「配方 / 食材」类比：哪个字段是"同一个配方"，哪个字段是"不同的食材"？
3. D11 开始画时，怎么告诉驱动"这一帧画到第几张图"？（提示：`vkCmdBeginRenderPass` 的参数）

---

### 8. 生命周期 / 销毁顺序

1. 写出 cleanup() 里 D10 新增的三处改动（帧缓冲循环、`vkDestroyPipeline`、删 shader module）。
2. 为什么 framebuffer 和 pipeline 要**先**销毁，renderPass / pipelineLayout / imageView 要**后**销毁？用一个词概括这个规则。
3. 如果你把 `vkDestroyPipeline` 放在 `vkDestroyPipelineLayout` **之后**，会发生什么？
4. `graphicsPipeline` 和 `swapChainFramebuffers` 为什么必须是成员变量？

---

### 9. 对号入座：D6~D10 攒了哪些对象

从 D6 到 D10，我们已经"打通"了从着色器到帧缓冲的整条链。按依赖顺序，把这些对象/结构体排一排：
shader module → ? → ? → ? → ?（提示：终点是 framebuffer 和 pipeline 之间那层 renderPass）

<details>
<summary>提示</summary>
shaderModule（D6）→ vertexInput/inputAssembly/viewport/rasterizer 等固定功能（D7~D8）→ pipelineLayout（D8）→ renderPass（D9）→ pipeline + framebuffer（D10）。
</details>

---

### 10. 动手题：立刻销毁的验证

建完管线后，`vkDestroyShaderModule` ×2 那几行你已经写了。实验：**把这些行注释掉**重新编译运行。

```
运行还正常吗？（提示：D11 之前程序只是建对象再销毁，所以看不出差别）
但验证层（validation layer）会不会报「VkShaderModule 泄漏」？观察退出信息。
改回来。
```

<details>
<summary>提示</summary>
程序行为无差别（管线已经把字节码编进去了），但退出时会看到验证层报告 VkShaderModule 资源没有被销毁。这正是 D6 讲义第 10 节"故意泄漏"实验的翻版。
</details>

---

### 11. 延伸思考

1. Vulkan 把 OpenGL 的一个 "program + 全局状态" 拆成 pipeline + 一堆显式结构体，代码量大了
   好几倍。驱动拿到的"确定性"换来什么？用户又要付出什么？（从"重新编译管线"这一动作切入）
2. pipeline cache（`vkCreateGraphicsPipelines` 的第二个参数）能解决上面提到的哪一档代价？
3. 为什么 render pass + framebuffer 拆成两份，就能让"同一份渲染流程"复用到多张交换链图像？
   如果 Vulkan 学 OpenGL 把两者合并成一个对象，交换链多图时你要怎么做？

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