# D9 自检题

> 合上代码和教程先自己答。全部答得出，再把 D9 打勾。

---

### 1. 全景：render pass 是什么、不是什么

1. `VkRenderPass` 里到底"持有"什么？它和 D10 的 framebuffer 各负责哪一半？
2. 用「配方 / 食材」或「签名 / 实参」的类比，说清为什么交换链有几张图、就要建几个 framebuffer，但 render pass 只要 1 个。
3. "三段式"指的是哪三段？

---

### 2. 附件描述 `VkAttachmentDescription`

1. `format` 填什么？为什么必须和交换链一致？
2. `samples` 填什么？它必须和哪个别的值一致？
3. 写出我们这 1 个附件在 `format` / `samples` 两栏的取值。

---

### 3. ⭐ loadOp / storeOp：附件内容的命运

1. `loadOp` 有哪三种取值？各是什么含义？
2. 我们为什么选 `CLEAR`？它对应 OpenGL 的哪个调用？
3. `storeOp` 为什么必须 `STORE`？如果写成 `DONT_CARE` 会发生什么？
4. `stencilLoadOp` / `stencilStoreOp` 为什么可以 `DONT_CARE`？

<details>
<summary>提示</summary>
第 3 问：`DONT_CARE` 意味着结果不被保留，屏幕可能不更新或内容未定义。第 4 问：本教程全程不用模板附件。
</details>

---

### 4. ⭐ initialLayout / finalLayout：图像内存怎么摆

1. "图像布局"（image layout）到底是什么？举出本单元涉及的三个布局。
2. `initialLayout` 为什么填 `UNDEFINED`？它表示"不在乎之前是什么"，还是"把图像清成未定义"？
3. `finalLayout` 为什么必须 `PRESENT_SRC_KHR`？如果填成 `COLOR_ATTACHMENT_OPTIMAL` 会怎样？
4. 从 `UNDEFINED` 到 `PRESENT_SRC_KHR` 中间要经过 `COLOR_ATTACHMENT_OPTIMAL`，这几次布局切换是谁在什么时机做的？到 D 几才需要我们自己显式写 barrier？

<details>
<summary>提示</summary>
第 4 问对应讲义第 2 节的"布局切换是显式的"那段：现在由 render pass 的 initial/final 声明"约定"了切换，真正自己写 pipeline barrier 是 D17。
</details>

---

### 5. 附件引用 `VkAttachmentReference` + 子通道 `VkSubpassDescription`

1. `colorAttachmentRef.attachment = 0` 里的 `0` 是什么？它和 `colorAttachmentRef.layout` 是同一回事吗？
2. 为什么 subpass 不直接拿附件、而要经过一层"引用"？
3. `pipelineBindPoint = GRAPHICS` 表示什么？还有别的取值吗？
4. `pColorAttachments` 指向的数组长度由哪个字段声明？

---

### 6. ⭐ 子通道依赖 `VkSubpassDependency`

1. `srcSubpass = VK_SUBPASS_EXTERNAL` 里的 "EXTERNAL" 指什么时刻？
2. 把这条依赖读成一句人话（谁等谁、在哪两个阶段、写/读什么内存）。
3. `srcAccessMask = 0` 而 `dstAccessMask = COLOR_ATTACHMENT_WRITE`，为什么一个 0 一个非 0？
4. 如果完全删掉这个 dependency，现在（D9~D10）会不会崩？D11 出图后可能会怎样？

<details>
<summary>提示</summary>
第 4 问：现在还没 begin/end render pass，依赖没被执行，所以不崩；但 D11 真跑起来后，缺依赖 = 少了"等交换链图像可写"的同步，属于未定义行为 / 花屏。这也是 D9 讲义第 8 节坑 #7。
</details>

---

### 7. 汇总 + `vkCreateRenderPass`

1. `VkRenderPassCreateInfo` 里三个 `Count` 字段分别对应哪三个 `p` 指针？
2. `vkCreateRenderPass` 成功后会做哪些"校验"？（说 1~2 个它要检查的自洽性）

---

### 8. 生命周期 / 依赖

1. `renderPass` 为什么必须做成成员变量，而不是局部变量？
2. `cleanup()` 里加什么？相对 `vkDestroyDevice` 的顺序是什么？
3. D10 建管线时，`renderPass` 会被填进哪个结构体的哪个字段？这对它的生命周期提出了什么要求？
4. 本单元是第几次"真正创建对象"（返回句柄、要销毁）？第一次是 D 几的什么？

<details>
<summary>提示</summary>
第 4 问：第二次。第一次是 D8 的 `vkCreatePipelineLayout`。
</details>

---

### 9. 对号入座：三段式完整顺序

按顺序写出 `createRenderPass()` 里的 5 个结构体 + 1 次创建调用，并各说一个关键字段。

（提示：AttachmentDescription → AttachmentReference → SubpassDescription → SubpassDependency → RenderPassCreateInfo → vkCreateRenderPass）

---

### 10. 动手题：loadOp 观察实验

（**等到 D11 出图后做**）把 `colorAttachment.loadOp` 从 `VK_ATTACHMENT_LOAD_OP_CLEAR` 改成
`VK_ATTACHMENT_LOAD_OP_LOAD`，编译运行。

```
画面变了吗？为什么？
（再想想）改成 DONT_CARE 又会怎样？—— 改回 CLEAR。
```

<details>
<summary>提示</summary>
`LOAD` 保留上一帧内容，但每一帧开始的内容其实是上一帧画完存下的结果（因为 storeOp=STORE），所以三角形本身可能看不出变化；真正有区别的是"残留帧"。这个实验的价值在于让你确认 loadOp 确实在"读/清"附件内容。
</details>

---

### 11. 延伸思考

1. Vulkan 为什么非要把 OpenGL 的一个 FBO 拆成 render pass + framebuffer 两份？多出来的这份"配方"
   带来什么优化机会（想想驱动能不能提前知道整帧的附件用法）？
2. `VkSubpassDependency` 和 OpenGL 的隐式同步相比，好在哪里、难在哪里？
3. 如果以后要做一个"先渲染场景、再对颜色附件做一次后处理"的效果，render pass 的哪个概念能派上用场（subpass 还是 dependency）？

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
