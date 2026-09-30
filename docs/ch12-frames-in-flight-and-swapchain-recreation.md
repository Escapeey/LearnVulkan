# D12 讲义：飞行帧 + 交换链重建（M1 收官）

> 教程章节：[Frames in flight](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Drawing/Frames_in_flight)、[Swap chain recreation](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Swap_chain_recreation)
>
> 前置：D11 完成（彩色三角形出图，但有一个单帧在飞的验证层告警）
> 自检题：`docs/ch12-check.md`

---

## 0. 本单元你要亲手写的

| 对象 / 改动 | 难度 | 对应 OpenGL | 本单元状态 |
|---|---|---|---|
| 同步对象从"单个"改成"数组"（×3 + `imagesInFlight`） | ★★ | （GL 里同步隐式，没有对应物） | **数组化** |
| `createSyncObjects()` 循环创建 | ★ | — | 重写 |
| `drawFrame()` 里 `imagesInFlight` 栅栏护栏 | ★★★ | — | **修 00067 的核心** |
| `cleanupSwapChain()` + `recreateSwapChain()` | ★★★ | — | 新增，缩放不崩 |
| `framebufferResizeCallback` + `glfwSetWindowUserPointer` | ★★ | `glfwSetFramebufferSizeCallback` | GL 里也这么写 |
| `cleanup()` 拆两层 + 同步对象循环销毁 | ★★ | — | 重写 |

> 本单元是 **M1「彩色三角形 + 缩放不崩 + 验证层零 error」的收官**。
> 做完它，D11 那个 `VUID-vkQueueSubmit-pSignalSemaphores-00067` 告警消失，窗口怎么拖都不崩。

---

## 1. 全景：两个问题

D11 的三角形能画出来，但藏着两个毛病：

1. **验证层告警 `pSignalSemaphores-00067`**：我们只用**一套**信号量/栅栏，却让 CPU 一次只提交一帧、
   干等 GPU，于是 `renderFinishedSemaphore` 在上一帧还没呈现完时就被再次 signal。
2. **不能缩放**：`glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE)`，窗口锁死 800×600。

D12 就是用两样东西分别解决它们：

> **飞行帧**（frame in flight）= 让 CPU 和 GPU **重叠干活**（GPU 画第 N 帧，CPU 已经在准备第 N+1 帧），
> 顺带用 `imagesInFlight` 修掉 00067。
>
> **交换链重建**（swap chain recreation）= 窗口尺寸变了就把交换链拆了重建，缩放不崩。

---

## 2. 飞行帧：为什么要把同步对象变成数组

### 2.1 D11 的问题：单帧在飞 = GPU 干完才敢下一帧

`drawFrame()` 开头那句 `vkWaitForFences(inFlightFence)` 的意思是：

```
CPU：提交第 N 帧 → 下一轮立刻就 vkWaitForFences 干等 → GPU 画完第 N 帧 → 才提交第 N+1 帧
```

CPU 大部分时间在**空转**。更糟的是，单套信号量导致 `renderFinishedSemaphore` 语义冲突 → 00067 告警。

### 2.2 解法：MAX_FRAMES_IN_FLIGHT 套同步对象

```cpp
const int MAX_FRAMES_IN_FLIGHT = 2;

std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence>     inFlightFences;
std::vector<VkFence>     imagesInFlight;   // 见 2.4
size_t currentFrame = 0;                   // 0 → 1 → 0 → 1 …
```

每一帧用 `currentFrame` 挑走一套同步对象，用完轮转到下一套：

```
currentFrame = 0:  用第 0 套  →  完了 currentFrame = 1
currentFrame = 1:  用第 1 套  →  完了 currentFrame = 0
```

于是 CPU 最多领先 GPU `MAX_FRAMES_IN_FLIGHT` 帧：

```
CPU:  提交帧0    提交帧1    (等帧0的fence)   提交帧2 …
GPU:        画帧0     画帧1       画帧2 …
```

`vkWaitForFences(inFlightFences[currentFrame])` 在等的是**两帧之前**的那帧，而不是上一帧。

### 2.3 ⭐ createSyncObjects 数组化

```cpp
void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;   // 初始 SIGNALED 的理由，同 D11

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}
```

注意：`imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE)` **不是**按 `MAX_FRAMES_IN_FLIGHT`，
而是按**交换链图像数**——见 2.4。

### 2.4 ⭐ imagesInFlight —— 修 00067 的关键

交换链可能有 3 张图像，而我们只有 2 套栅栏。**"第 N 帧在飞" 和 "第 k 张图被占用" 是两件事**：

- `inFlightFences[currentFrame]` 保证"这一**帧**的提交没干完之前不覆盖它"；
- 但 `vkAcquireNextImageKHR` 返回**哪张图**是驱动的决定，两张图可能撞到同一张。

如果不对"图像"做护栏，就可能出现：acquire 拿到第 2 张图，而第 2 张图**还挂着上一帧的 fence**（那一帧
还没呈现完）——于是 00067：同一张图上的上一个 `renderFinishedSemaphore` 还没被消费，新的提交又来 signal 它。

所以加一个"每图一个 fence"的登记表：

```cpp
if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
    vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
}
imagesInFlight[imageIndex] = inFlightFences[currentFrame];
```

读法：**acquire 到第 `imageIndex` 张图 → 它若还被某个 fence 占着，先等那个 fence → 再把它登记成
当前这帧的 fence**。这样任何一张图在"上一个使用者"呈现完之前不会被重新写。

> ⭐ 一句话记忆：`inFlightFences` 管**帧**，`imagesInFlight` 管**图**；图数（3）≥ 帧数（2），
> 所以最坏也只是多等一下，不会死锁。

---

## 3. 交换链重建：缩放不崩

### 3.1 为什么要重建整个交换链

窗口尺寸一变，`swapChainExtent` 就变了。而交换链图像、图像视图、帧缓冲、管线里的 viewport/scissor
**全都抄了这个旧尺寸**。与其打一堆补丁，不如**整个拆了重建**。

### 3.2 cleanupSwapChain —— 拆交换链系

```cpp
void cleanupSwapChain() {
    for (auto framebuffer : swapChainFramebuffers)   // ① 帧缓冲
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    for (auto imageView : swapChainImageViews)       // ② 图像视图
        vkDestroyImageView(device, imageView, nullptr);

    vkDestroySwapchainKHR(device, swapChain, nullptr);   // ③ 交换链

    vkDestroyPipeline(device, graphicsPipeline, nullptr);        // ④ 管线
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);    // ⑤ 布局
    vkDestroyRenderPass(device, renderPass, nullptr);            // ⑥ 渲染通道
}
```

为什么连**管线**都要重建？因为我们的 viewport/scissor 是 D7/D8 里**静态烤死**进管线的
（`viewport.width = (float)swapChainExtent.width`）。尺寸变了，管线里的旧尺寸就错了，只能重烤一条。

> ⭐ 这正是 D10 讲到过的"管线是烤死的"的后果：窗口缩放这件事，在 OpenGL 里 `glViewport` 一改就行，
> Vulkan 里静态管线得整个重建（或者改 dynamic state，前瞻见第 7 节）。

### 3.3 recreateSwapChain —— 重建

```cpp
void recreateSwapChain() {
    // ① 最小化时 framebuffer 尺寸是 0——"0 号尺寸"没法建交换链，循环等用户把窗口还原。
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // ② 等 GPU 干完所有在飞命令，才能安全拆正在被用的交换链。
    vkDeviceWaitIdle(device);

    // ③ 拆。
    cleanupSwapChain();

    // ④ 重建（注意顺序 = 创建期的顺序）。
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
}
```

> `glfwWaitEvents()` 不是忙等：它把 CPU 放进去等事件队列，窗口还原了才返回，不烧 CPU。
> `vkDeviceWaitIdle` 比"只等本帧 fence"更重（等所有队列全空），但对"重建交换链"这种低频事件，
> 简单正确优先于性能。

### 3.4 谁触发重建：framebufferResized

窗口缩放时 GLFW 会调一个 C 回调，但**在回调里直接 `recreateSwapChain()` 会和正在跑的渲染帧竞态**。
所以回调只置一个标志：

```cpp
// initWindow() 里：
glfwSetWindowUserPointer(window, this);
glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

static void framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}
```

真正的重建留在 `drawFrame()` 的呈现环节检查。注意三种"该重建"的来源：

| 来源 | 含义 |
|---|---|
| `vkAcquireNextImageKHR` 返回 `VK_ERROR_OUT_OF_DATE_KHR` | 交换链失效（尺寸变了/丢了） |
| `vkQueuePresentKHR` 返回 `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR` | 呈现时发现过期 / 勉强能用 |
| `framebufferResized` 标志为 true | GLFW 告诉我们窗口尺寸变了 |

acquire 阶段就建了，present 阶段又遇上了怎么办？重复重建是**安全的**（`recreateSwapChain` 本身幂等——
先 `vkDeviceWaitIdle` 再拆再建）。所以三个来源各自触发即可，不用防重。

### 3.5 drawFrame 里的对应处理

```cpp
VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;   // 本帧作废
} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
}
// ... imagesInFlight 护栏 + 重录 + submit ...

result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
} else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
}

currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
```

---

## 4. cleanup 拆两层

之前 `cleanup()` 一把梭。现在交换链系对象要能"单独拆"，所以分两层：

| 层 | 什么时候拆 | 拆什么 |
|---|---|---|
| `cleanupSwapChain()` | 每次重建前 + 最终退出 | framebuffers / imageViews / swapchain / pipeline / pipelineLayout / renderPass |
| `cleanup()`（顶层） | 仅最终退出 | 先调 `cleanupSwapChain()`，再拆同步数组、命令池、设备、表面、实例、窗口 |

```cpp
void cleanup() {
    cleanupSwapChain();   // 交换链系都归这里

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    // ... debugMessenger / instance / window / glfwTerminate（不变）
}
```

同步对象**不属于** `cleanupSwapChain`：它们和交换链图像数无关（按帧数），重建时不需要重来，最终退出时才销毁。

---

## 5. OpenGL 对照总表

| OpenGL | Vulkan | 说明 |
|---|---|---|
| `glViewport`（随时可改） | 重建整条管线（静态 viewport）或 dynamic state | 缩放时 GL 一行搞定，Vulkan 要拆交换链重建 |
| 驱动隐式做帧流水（双缓冲） | CPU 手写 `MAX_FRAMES_IN_FLIGHT` + 数组 | GL 帮你叠帧，Vulkan 要自己规划 |
| `glfwSetFramebufferSizeCallback` | 同名 API，只多一步 `glfwSetWindowUserPointer` | GL 里回调直接改状态，Vulkan 里只置标志 |
| （无） | `vkDeviceWaitIdle` + 拆链 + 重建 | GL 从不要求你"停住 GPU 重建后台缓冲" |
| 同步全隐式 | `inFlightFences`（帧）+ `imagesInFlight`（图）双层护栏 | Vulkan 把双缓冲的 hazard 显式暴露了 |

---

## 6. 本单元最容易踩的坑

| # | 现象 | 根因 | 处理 |
|---|---|---|---|
| 1 | 编译报原地三件套 `imageAvailableSemaphore` 未声明 | 成员改成数组，旧名没用干净 | 全改成 `...[currentFrame]` / `...[i]` |
| 2 | `imagesInFlight` resize 成 `MAX_FRAMES_IN_FLIGHT` | 想当然按帧数 | 按 `swapChainImages.size()`，初值 `VK_NULL_HANDLE` |
| 3 | 忘了 `imagesInFlight[imageIndex]` 先 wait 再登记 | 栅栏护栏漏了 | 两步都要：`if (!= NULL_HANDLE) wait; imagesInFlight[i] = ...` |
| 4 | `recreateSwapChain` 里忘了 `vkDeviceWaitIdle` | 急着拆 | 拆正在被提交用的交换链 = 未定义行为 |
| 5 | 最小化窗口死循环烧 CPU | 用 `glfwPollEvents` 忙等 | 用 `glfwWaitEvents()`，且条件是 `while(width==0\|\|height==0)` |
| 6 | `cleanupSwapChain` 漏了 pipeline / renderPass | 以为只有 imageView/swapchain | 静态 viewport 的管线也要重建，所以也要拆 |
| 7 | `cleanup()` 里删掉了同步对象循环，也没调 `cleanupSwapChain` | 改了一半 | `cleanup()` 开头调 `cleanupSwapChain()`，同步对象自己循环销毁 |
| 8 | present 结果没处理 `framebufferResized` | 只看了返回值 | 返回值 + `framebufferResized` 三选一都要判，判完把标志清零 |

---

## 7. 前瞻

- **D13 顶点缓冲**：三角形那时候才会从显存缓冲读顶点（现在硬编码在 `shader.vert`）。飞行帧的
  `currentFrame` 循环会在 D14/D15 继续沿用——每帧一套 uniform 缓冲就靠它轮转。
- 本单元重建管线是"静态 viewport"的代价。真正的工程质量做法是**dynamic state**（`VK_DYNAMIC_STATE_VIEWPORT`
  / `SCISSOR` + 命令缓冲里 `vkCmdSetViewport`/`vkCmdSetScissor`），这样缩放**不用重建管线**。M2 之后可以回来改。
- 做完 D12，去跑一次 M1 验收：拖窗口、最小化再还原、观察验证层日志——**应该 zero error**。做不到再回来查第 6 节坑表。

---

## 8. 本单元完成标志

- [ ] `createSyncObjects()` 改成数组化 + 循环创建，`imagesInFlight` 按图像数 resize
- [ ] `drawFrame()` 用 `currentFrame` 索引三套同步对象，加了 `imagesInFlight` 的 wait + 登记
- [ ] `drawFrame()` 处理 acquire 的 `VK_ERROR_OUT_OF_DATE_KHR`（重建 + return）
- [ ] `drawFrame()` 处理 present 的 `OUT_OF_DATE` / `SUBOPTIMAL` / `framebufferResized`（重建），末尾 `currentFrame` 轮转
- [ ] `initWindow()` 改 `GLFW_RESIZABLE=TRUE` + `glfwSetWindowUserPointer` + 尺寸回调
- [ ] 新增 `framebufferResizeCallback`（static + reinterpret_cast 找回 this）
- [ ] 新增 `cleanupSwapChain()`（逆序拆 6 类对象）和 `recreateSwapChain()`（最小化等恢复 → `vkDeviceWaitIdle` → 拆 → 按序重建）
- [ ] `cleanup()` 开头调 `cleanupSwapChain()`，同步对象改成循环销毁
- [ ] 拖窗口 / 最小化还原不崩，验证层日志 **zero error**（00067 消失）
- [ ] 能说清 `inFlightFences` 和 `imagesInFlight` 各管什么、为什么尺寸不同
- [ ] 能说清"为什么缩放要重建管线"
- [ ] 能回答 `docs/ch12-check.md` 全部问题
- [ ] `PROGRESS.md` 里 D12 打勾 + 3 行总结（**M1 完成**）