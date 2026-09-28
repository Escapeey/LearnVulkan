# D11 自检题

> 合上代码和教程先自己答。全部答得出，再把 D11 打勾。

---

### 1. 全景：一帧的四步

1. `drawFrame()` 里的四步是什么？各对应一句人话。
2. 为什么说 D2~D10 建的所有对象"本身不产生任何像素"？真正让 GPU 干活的是本单元的什么？
3. 两条同步线（GPU↔GPU、GPU↔CPU）分别由谁承担？

---

### 2. 命令池 + 命令缓冲

1. 命令缓冲是怎么来的？为什么说是 **allocate** 而不是 create？
2. `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT` 这个 flag 干什么用？没加会怎样（本单元哪一行会踩到）？
3. 命令池绑定的是哪个队列族？为什么是 graphics 而不是 present？
4. cleanup() 里为什么**没有** `vkDestroyCommandBuffer` 这一行？

---

### 3. ⭐ recordCommandBuffer + VkClearValue

1. `VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};` 为什么是**三重**花括号？每一层各是什么？
2. 这个 clear 色是什么时候、被哪个规则"执行"的？（提示：D9 的哪个字段）
3. `vkCmdDraw(3, 1, 0, 0)` 的 4 个参数各是什么？我们的三角形顶点哪来的（着色器里还是缓冲里）？
4. `vkCmdBindPipeline` 里的 `VK_PIPELINE_BIND_POINT_GRAPHICS` 表示什么？还有别的 bind point 吗？

<details>
<summary>提示</summary>
第 1 问：VkClearValue 是 union（color 字段），VkClearColorValue 也是 union（float32[4] 等）。第 2 问：loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR，在 vkCmdBeginRenderPass 时执行。第 3 问：顶点硬编码在 `shaders/shader.vert`，靠 gl_VertexIndex 取，D13 才换成顶点缓冲。
</details>

---

### 4. ⭐ 信号量 vs 栅栏

1. 信号量和栅栏分别负责哪个方向、哪个"主体"之间的同步？
2. 三个同步对象里，哪两个是信号量、哪个是栅栏？各自的语义是什么？
3. 为什么"等上一帧干完"这个活必须用 fence 而不是信号量来完成？

---

### 5. ⭐ fence 为什么初始 SIGNALED

1. `VK_FENCE_CREATE_SIGNALED_BIT` 是什么含义？不设会怎样？
2. `vkWaitForFences` + `vkResetFences` 这两个调用在本单元的顺序是什么？为什么必须先 wait 再 reset？
3. reset 必须在复用 commandBuffer / 重新 acquire **之前**，如果顺序反了会发生什么？

<details>
<summary>提示</summary>
第 1 问：不设的话第一帧的 vkWaitForFences 会永远等下去——因为还没任何一帧提交过 fence。第 3 问：会覆盖还没执行完的命令缓冲 / 读写未就绪的图像 → 未定义行为。
</details>

---

### 6. drawFrame + submit/present 参数

1. `pWaitDstStageMask` 为什么是**数组**？我们填的 `VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT` 表示"等到什么时候"？
2. `vkQueueSubmit` 的最后一个参数是什么？它和 `vkQueuePresentKHR` 的联系是什么？
3. `vkQueuePresentKHR` 我们用的是哪个队列？教程为什么用的是另一个？（你的机器上两者什么关系）

---

### 7. ⭐ submit 的 wait/signal 语义

把 `vkQueueSubmit` 这一句读成人话：提交了什么、等谁、给谁发信号。

---

### 8. mainLoop + vkDeviceWaitIdle

1. `mainLoop()` 里每帧要做什么？和之前（D10 前）空循环的唯一区别是什么？
2. 循环结束后那句 `vkDeviceWaitIdle(device)` 是干什么的？删掉会怎样？

---

### 9. cleanup 销毁顺序

1. cleanup() 里新增了哪 4 个 destroy？它们相对 `vkDestroyDevice` 的顺序是什么？
2. 为什么没有销毁 commandBuffer 的动作？

---

### 10. 动手题：改 clear 色观察

（**出图后做**）把 `recordCommandBuffer` 里的 clear 色从黑改成别的（比如 `{0.2f, 0.3f, 0.4f, 1.0f}`），
编译运行。

```
背景变了吗？三角形本身变了吗？—— 改回黑色。
```

<details>
<summary>提示</summary>
背景变色、三角形不变——因为 clear 只清"颜色附件"，三角形是 vkCmdDraw 画上去的。这个实验让你确认 clear 色和三角形颜色是两回事（一个来自 VkClearValue，一个来自片元着色器）。
</details>

---

### 11. 延伸思考

1. OpenGL 里 `SwapBuffers` 一行完成"提交 + 等待 + 呈现 + 同步"全部，Vulkan 拆成了
   `vkQueueSubmit` + semaphore + fence + `vkQueuePresentKHR` 一堆显式调用。这些多出来的
   代码换来了什么？（提示：想想 CPU 能不能在 GPU 画上一帧的同时准备下一帧）
2. 现在一次只让 **1 帧** 在飞（CPU 提交完就 `vkWaitForFences` 干等 GPU）。D12 的
   "飞行帧" 要解决什么问题？它大致会怎么改（把什么变成数组）？
3. 为什么 Vulkan 一定要把"同步"这个 OpenGL 里最省心的事，变成最显式、最容易写错的事？

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