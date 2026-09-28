# D11 讲义：命令缓冲 + 渲染与呈现（第一次出图 🎉）

> 教程章节：[Command_buffers](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Drawing/Command_buffers)、[Rendering_and_presentation](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Drawing/Rendering_and_presentation)
>
> 前置：D10 完成（管线 + 帧缓冲已就绪）
> 自检题：`docs/ch11-check.md`

---

## 0. 本单元你要亲手写的

| 对象 / 函数 | 难度 | 对应 OpenGL | 本单元状态 |
|---|---|---|---|
| `VkCommandPool` + `createCommandPool` | ★★ | （无直接对应，见下） | 命令的"分配器" |
| `VkCommandBuffer` + `createCommandBuffer` | ★★ | `glDraw*` 背后驱动偷偷做的 | 命令的"载体" |
| `recordCommandBuffer`（begin render pass / draw / end） | ★★★ | 一整帧 `glClear` + `glDraw*` 序列 | 把命令录下来 |
| `VkSemaphore` ×2 + `VkFence` ×1 | ★★★ | 无（OpenGL 全自动同步） | GPU↔GPU / GPU↔CPU 同步 |
| `drawFrame`（acquire / submit / present） | ★★★ | `SwapBuffers` + 驱动隐式同步 | 每帧的主干 |

> **本单元结束，彩色三角形第一次出现在屏幕上。** 这是 M1 里程碑（D2~D12）最关键的一步。
> 你的着色器已经输出红/绿/蓝三色三角形（`shaders/shader.vert` 里硬编码的 3 个顶点 + 3 个颜色），
> clear 色填黑 `{0,0,0,1}`，所以你会看到：**黑色背景上一个 RGB 渐变的三角形**。

---

## 1. 全景：一帧是怎么被"提交"的

前面 D2~D10 一直在"备料"：建实例、设备、交换链、管线、帧缓冲。**但这些对象本身不产生任何
像素**——它们只是"材料"和"配方"。真正让 GPU 干活的是本单元。

一帧的完整流程（`drawFrame()` 里的四步）：

```
① acquire   从交换链拿一张可画的图（第 imageIndex 张）
② record    把"画这张图"要做的 GPU 命令录进 commandBuffer
③ submit    把 commandBuffer 提交给 graphicsQueue 执行
④ present   告诉呈现引擎"画好了，显示它"
```

中间穿插着两条**同步线**（这是本单元最烧脑、也最核心的部分）：

```
GPU 这边：  ① acquire ──发信号──► imageAvailableSemaphore ──► ③ submit 开始画
            ③ submit 画完 ──发信号──► renderFinishedSemaphore ──► ④ present 开始显示
CPU 这边：  drawFrame 开头 vkWaitForFences(inFlightFence)  ← 等上一帧整个干完
```

一句话：

> **命令缓冲 = 给 GPU 的"剧本"；提交/呈现 = "开演"；信号量/栅栏 = 三个演员之间的"喊卡"。**

---

## 2. 命令池 + 命令缓冲 + 录制命令

### 2.1 命令池 `createCommandPool`

命令缓冲不能凭空 `new`，而是从一个**命令池**里**分配**出来。命令池挂在一个队列族上
（因为命令最终要提交到某个族的队列执行）。

```cpp
void createCommandPool() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("failed to create command pool!");
}
```

- `flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`：允许**单独 reset** 某个命令缓冲。
  我们每帧要 `vkResetCommandBuffer` 再重录，**这一位必须要有**，否则 reset 是非法操作。
- 注意 pool 只绑定 `graphicsFamily`（不是 present —— 命令是"执行"，不是"显示"）。

### 2.2 命令缓冲 `createCommandBuffer`

```cpp
void createCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // 主命令缓冲（能直接提交）
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate command buffers!");
}
```

> ⭐ 一个措辞差别：命令缓冲是 **allocate**（`vkAllocateCommandBuffers`），其他对象都是 **create**。
> 因为它不是独立资源，而是命令池"手下"的东西——销毁命令池会自动释放手下所有命令缓冲，
> 所以 cleanup() 里**不用**（也不能）单独销毁 commandBuffer。

### 2.3 录制命令 `recordCommandBuffer`

```cpp
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = 0;              // Optional
    beginInfo.pInheritanceInfo = nullptr;        // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("failed to begin recording command buffer!");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass      = renderPass;
    renderPassInfo.framebuffer     = swapChainFramebuffers[imageIndex];  // 本帧画到第 imageIndex 张
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};   // ⚠️ 三重花括号
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues    = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);        // 3 个顶点、1 个实例
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("failed to record command buffer!");
}
```

逐行要点：

| 调用 | 干什么 | 对应 OpenGL |
|---|---|---|
| `vkCmdBeginRenderPass` | 开始一次渲染通道，**此刻才真正执行 D9 的 attachment 规则**（含 loadOp=CLEAR 清屏） | `glBindFramebuffer` + `glClear` 的一部分 |
| `VkClearValue` | 给 loadOp=CLEAR 的颜色附件准备"清成什么色" | `glClearColor` |
| `vkCmdBindPipeline` | 绑定 D10 建的图形管线 | `glUseProgram` + 记下当时全部状态 |
| `vkCmdDraw` | 画 3 个顶点 | `glDrawArrays(GL_TRIANGLES, 0, 3)` |
| `vkCmdEndRenderPass` | 结束渲染通道，执行 storeOp=STORE 把结果写回 | （驱动隐式） |

> ⭐ **D9 埋的伏笔全部在这里兑现**：loadOp=CLEAR 借 `pClearValues` 清了屏；
> finalLayout=PRESENT_SRC_KHR 让这张图在呈现前把内存布局转好。renderPass 是"规则书"，
> begin/end render pass 是"照规则书执行的那一段"。
>
> ⭐ **`VkClearValue` 的三重花括号**：它是 union（`color` 又是个 union 里套 `float32[4]`），
> `{{{0,0,0,1}}}` 逐层剥开 = 外层 VkClearValue → 中层 color → 内层 float[4]。少一层就编译报错。

---

## 3. ⭐ 同步对象：信号量 vs 栅栏（本单元核心）

OpenGL 帮你把所有同步偷偷做了；Vulkan 里你必须自己"喊卡"。本单元要三种同步原语：

| 原语 | 方向 | 管什么 | 创建 flag |
|---|---|---|---|
| `imageAvailableSemaphore` | GPU→GPU | "交换链图可用了，可以画" | 无 |
| `renderFinishedSemaphore` | GPU→GPU | "画完了，可以显示" | 无 |
| `inFlightFence` | GPU→CPU | "整帧干完了，CPU 可以复用资源" | `VK_FENCE_CREATE_SIGNALED_BIT` |

**为什么 fence 初始要 SIGNALED？** fence 的固定用法是"提交后等它、再复位"。但程序第一次进
`drawFrame` 时，还没有任何一帧提交过——如果 fence 初始是 unsignaled，第一句
`vkWaitForFences` 会**永远等下去**（死锁）。所以 fence 创建时先置 SIGNALED，让第一帧能通过。

```cpp
void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};    // 信号量只有 sType，没有别的字段
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;    // ← 关键：初始就 signaled

    vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore);
    vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence);
}
```

> 记忆：**信号量管"先后"，栅栏管"等没等完"。** 信号量是 GPU 与 GPU 之间的握手（不阻塞 CPU），
> 栅栏是 GPU 通知 CPU 的通道（CPU 主动 `vkWaitForFences` 阻塞等待）。
> OpenGL 里两者都不存在——`glFinish` 是最接近 fence 的东西（但更粗暴、更慢）。

---

## 4. ⭐ drawFrame：四步串起一整帧

```cpp
void drawFrame() {
    // ① 等上一帧干完 + 复位栅栏（不这样会覆盖还没执行完的命令缓冲）
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    // ② 拿一张可画的图
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                          imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // ③ 重录命令（先 reset 掉上一帧的内容）
    vkResetCommandBuffer(commandBuffer, 0);
    recordCommandBuffer(commandBuffer, imageIndex);

    // ④ 提交执行
    VkSemaphore waitSemaphores[]   = { imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
        throw std::runtime_error("failed to submit draw command buffer!");

    // ⑤ 呈现
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapChain;
    presentInfo.pImageIndices      = &imageIndex;
    presentInfo.pResults           = nullptr;     // Optional

    vkQueuePresentKHR(graphicsQueue, &presentInfo);
}
```

三个最容易被忽略的点：

1. **`pWaitDstStageMask` 是数组指针**——`VkSubmitInfo` 里它是 `const VkPipelineStageFlags*`，
   每个等待信号量对应一个"等到哪个阶段"。我们等 imageAvailableSemaphore 到
   `COLOR_ATTACHMENT_OUTPUT`（开始写颜色附件之前就绪即可）。
2. **Wait/Reset Fence 的顺序**：先 wait 再 reset，且 reset 必须在**复用 commandBuffer /
   重新 acquire 之前**。顺序反了会读/写还没执行完的缓冲区。
3. **`vkQueuePresentKHR` 用的是 `graphicsQueue`**：教程里有个独立的 `presentQueue`
   （因为某些机器 graphics 和 present 是不同队列族）。你的机器两个族是同一个（见
   `docs/your-gpu.md`），所以直接复用 `graphicsQueue`。

> 本单元还**故意不处理** `VK_ERROR_OUT_OF_DATE_KHR`（窗口尺寸变了 / 交换链过期）和
> `VK_SUBOPTIMAL_KHR`（还能用但不再最优）——这些 D12 "交换链重建"才接手。
> 所以本单元请保持窗口固定 800×600 别缩放（initWindow 已经 `GLFW_RESIZABLE = GLFW_FALSE`）。

---

## 5. mainLoop + cleanup 的接线

```cpp
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();                // ← 每帧一遍四步
    }
    vkDeviceWaitIdle(device);       // ← 等 GPU 把最后一帧干完，cleanup 才能安心销毁
}
```

`vkDeviceWaitIdle` 是"等设备完全空闲"。没有它，`cleanup()` 可能在 GPU 还在用 semaphore /
commandPool 时就销毁它们 → 验证层报错甚至崩溃。

cleanup() 新增（在 `vkDestroyDevice` 之前）：

```cpp
vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
vkDestroyFence(device, inFlightFence, nullptr);

vkDestroyCommandPool(device, commandPool, nullptr);   // 自动释放 commandBuffer
```

> 注意：**没有** `vkDestroyCommandBuffer`——命令缓冲归命令池管，池销毁时一起带走。

---

## 6. OpenGL 对照总表（速查）

| OpenGL 调用 / 机制 | Vulkan | 说明 |
|---|---|---|
| `glClearColor` | `VkClearValue` + renderPass 的 loadOp=CLEAR | clear 的动作在 begin render pass 时发生 |
| `glClear` | `vkCmdBeginRenderPass`（触发 loadOp） | Vulkan 把"清屏"折叠进渲染通道 |
| `glUseProgram` | `vkCmdBindPipeline` | 绑定的是打包好的整条管线 |
| `glDrawArrays` | `vkCmdDraw` | 录制进命令缓冲，提交后才执行 |
| `SwapBuffers`（隐式等待） | `vkQueueSubmit` + `vkQueuePresentKHR` | 显式拆分"提交"和"呈现" |
| 驱动自动同步 | semaphore + fence | 你自己"喊卡" |
| `glFinish` | `vkDeviceWaitIdle` / `vkWaitForFences` | 最接近的对应 |

---

## 7. 本单元最容易踩的坑

| # | 现象 | 根因 | 处理 |
|---|---|---|---|
| 1 | 黑屏（连 clear 色都没有） | 忘了 `vkCmdBeginRenderPass` 或没填 `pClearValues` | recordCommandBuffer 四件套缺一不可 |
| 2 | 死锁 / 第一帧就卡住不动 | fence 没加 `VK_FENCE_CREATE_SIGNALED_BIT` | fence 初始 SIGNALED |
| 3 | 编译报 `VkClearValue` 初始化错 | 花括号层次不对 | `{{{0,0,0,1}}}` 三重 |
| 4 | `pWaitDstStageMask` 传单值不是数组 | 类型写错 | 用 `VkPipelineStageFlags waitStages[] = {...}` 数组 |
| 5 | 退出时报 semaphore/fence/pool 泄漏 | cleanup 漏了销毁 | 见第 5 节的 4 个 destroy |
| 6 | 提交后立即销毁导致崩溃 | 没 `vkDeviceWaitIdle` | mainLoop 末尾加一句 |
| 7 | `vkQueuePresentKHR` 报队列不匹配 | 用了错误的队列 | 你的机器 graphics==present，用 graphicsQueue |

---

## 8. 本单元完成标志

- [ ] `createCommandPool()`（含 `RESET_COMMAND_BUFFER_BIT` flag）
- [ ] `createCommandBuffer()`（allocate 不是 create）
- [ ] `createSyncObjects()`（fence 加 `VK_FENCE_CREATE_SIGNALED_BIT`）
- [ ] `recordCommandBuffer()`：begin render pass（含 clear value）→ bind → draw(3) → end render pass → end command buffer
- [ ] `drawFrame()`：wait/reset fence → acquire → reset/record → submit → present
- [ ] `mainLoop()` 每帧 `drawFrame()`，末尾 `vkDeviceWaitIdle`
- [ ] `cleanup()` 销毁 2 个 semaphore + 1 个 fence + commandPool
- [ ] **彩色三角形出现在屏幕上**（黑底 RGB 三角形）
- [ ] 能说清 semaphore 和 fence 的分工（GPU↔GPU vs GPU↔CPU）
- [ ] 能说清 fence 为什么初始 SIGNALED
- [ ] 能说清 `vkCmdDraw` 的 4 个参数
- [ ] 能回答 `docs/ch11-check.md` 全部问题
- [ ] `PROGRESS.md` 里 D11 打勾 + 3 行总结

---

## 9. 前瞻

- **D12** 收尾 M1：飞行帧（`MAX_FRAMES_IN_FLIGHT` —— 现在一次只让 1 帧在飞，CPU 会空等 GPU，
  效率低）、交换链重建（minimize / resize），以及 `vkAcquireNextImageKHR` 返回 `OUT_OF_DATE` /
  `SUBOPTIMAL` 的处理。之后 M1（彩色三角形）正式达标。
- 出图后建议**回头做** D7/D8/D9 自检题的动手实验（改 frontFace、恒等混合、loadOp），
  现在终于有画面可以观察了。