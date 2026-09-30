# D12 自检题

> 合上代码和教程先自己答。全部答得出，再把 D12 打勾。

---

### 1. 全景：本单元解决哪两个问题

1. D11 的三角形虽能出图，但有哪两个毛病？分别在 D12 用什么手段解决？
2. 这两个手段里，哪个顺带把 `pSignalSemaphores-00067` 告警也修掉了？

---

### 2. 为什么同步对象要数组化

1. D11 单套同步对象下，CPU 和 GPU 是怎么"串行空转"的？一句话描述。
2. `MAX_FRAMES_IN_FLIGHT = 2` 意味着什么？CPU 最多领先 GPU 几帧？
3. `currentFrame` 是干嘛的？它怎么轮转？

---

### 3. ⭐ inFlightFences vs imagesInFlight

1. `inFlightFences` 管的是什么？`imagesInFlight` 管的是什么？
2. 为什么 `imagesInFlight` 的 size 是 **交换链图像数**，而不是 `MAX_FRAMES_IN_FLIGHT`？
3. `imagesInFlight[imageIndex] != VK_NULL_HANDLE` 这个判断在问什么？为真时做什么？

<details>
<summary>提示</summary>
第 1 问：前者管"帧"（这一套提交没干完别覆盖），后者管"图"（这张图上一个使用者呈现完才能重写）。第 2 问：因为 acquire 返回哪张图是驱动决定的，每张图都可能被用到，需要给每张图登记一个"正在占用它的 fence"。
</details>

---

### 4. ⭐ imagesInFlight 为什么能修 00067

1. 00067（`vkQueueSubmit-pSignalSemaphores-00067`）的字面意思是什么？
2. 想清楚"提交帧 N+1 时，帧 N 的 `renderFinishedSemaphore` 还没被消费"这个场景，`imagesInFlight`
   的 wait 是在哪一步拦下来的？

<details>
<summary>提示</summary>
场景：3 张图、2 套帧。帧 2 的 acquire 恰好又拿到帧 0 用过的那张图，而帧 0 的 fence 还挂着没呈现完。这时若不 wait，帧 2 会往同一张图上再次 signal renderFinished 信号量 → 00067。imagesInFlight 记录"这张图正在被哪个 fence 用"，遇到就直接 wait。
</details>

---

### 5. drawFrame 里 acquire 的返回值处理

1. `vkAcquireNextImageKHR` 可能返回哪些"非 SUCCESS"？分别怎么处理？
2. 遇到 `VK_ERROR_OUT_OF_DATE_KHR` 为什么是 `recreateSwapChain(); return;`（本帧作废），而不是继续画？

---

### 6. drawFrame 里 present 的返回值处理

1. `vkQueuePresentKHR` 的那三个触发重建的条件是什么？各自是什么意思？
2. 处理完重建后为什么要把 `framebufferResized` 清零？
3. `VK_SUBOPTIMAL_KHR` 和 `VK_ERROR_OUT_OF_DATE_KHR` 的区别是什么？

---

### 7. ⭐ 回调只置标志，不直接重建

1. `framebufferResizeCallback` 为什么只把 `framebufferResized = true`，不直接调 `recreateSwapChain()`？
2. `glfwSetWindowUserPointer(window, this)` 是干嘛的？回调里怎么把 `this` 找回来？
3. 为什么这个回调是 `static`？

<details>
<summary>提示</summary>
第 1 问：回调可能在 CPU 提交到一半时触发，直接拆交换链会和在飞的渲染竞态；置标志让 drawFrame 在"安全的时机"处理。第 3 问：GLFW 是 C 接口，成员函数指针带 this 不兼容 C 函数指针，static 才能当回调传。
</details>

---

### 8. ⭐ recreateSwapChain 的四步

1. 为什么开头要 `while (width == 0 || height == 0) { glfwWaitEvents(); }`？什么情况下尺寸是 0？
2. 为什么重建前必须 `vkDeviceWaitIdle(device)`？
3. 重建的那五个 `create*` 的**顺序**为什么不能乱？
4. `glfwWaitEvents()` 和忙等（`glfwPollEvents`）的区别是什么？

---

### 9. cleanupSwapChain 拆哪些、不拆哪些

1. `cleanupSwapChain()` 拆哪 6 类对象？都是逆序的吗？
2. 为什么连 `graphicsPipeline` 都要拆（重建）？线索在 D7/D8 的 viewport。
3. 同步对象（信号量/栅栏）为**什么不**放进 `cleanupSwapChain`？

---

### 10. 动手题：验证 M1

（**做完 D12 后做**）

1. 运行程序，看验证层日志——还有没有 `VUID-...-00067`？应该是 zero error。
2. 拖动窗口边缘改大小：画面变吗？崩吗？
3. 最小化窗口几秒再还原：崩吗？（这是最容易踩的坑，重点测）
4. 反复快速缩放：CPU 会一直忙等烧满吗，还是能恢复？

```
把现象记下来。有问题时对照 ch12 讲义第 6 节坑表。
```

---

### 11. 延伸思考

1. 我们为"缩放不崩"重建了整条管线。有什么办法能让缩放**不用**重建管线？（提示：`VK_DYNAMIC_STATE_VIEWPORT`）
2. `MAX_FRAMES_IN_FLIGHT` 设成 3 或 4 会怎样？设成 1 呢（是不是就退回 D11 了）？
3. OpenGL 的双缓冲 / 三重缓冲和这里的 `MAX_FRAMES_IN_FLIGHT` 是一回事吗？谁在什么时候替你把同步做了？

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