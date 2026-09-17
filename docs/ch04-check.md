# D4 自检题

> 合上代码和教程先自己答。全部答得出，再把 D4 打勾。

---

### 1. 为什么需要交换链

用一句话说明交换链是什么。然后回答：**OpenGL 里对应这个东西的是什么？为什么你在 OpenGL 里从没"创建"过它？**

---

### 2. 平台无关的代价

`VkSurfaceKHR` 是平台无关的对象，但创建它的过程不是。请说明：

1. 在 Windows 上，`glfwCreateWindowSurface` 背后实际调用了什么函数？需要哪两个句柄？
2. Linux（X11）和 Android 上分别是什么函数？
3. 这说明 WSI 扩展存在的根本原因是什么？

---

### 3. 实例扩展 vs 设备扩展（复习 + 加深）

1. `VK_KHR_surface` 和 `VK_KHR_swapchain` 分别属于哪一类？
2. 为什么 `VK_KHR_swapchain` 必须是**设备**扩展？用"有些设备只能做计算"这个事实来解释。
3. D3 里你写了 `createInfo.enabledExtensionCount = 0;`。D4 要改成什么？

---

### 4. `std::set` 差集技巧

`checkDeviceExtensionSupport` 的做法是：把需要的扩展放进 `std::set`，然后用实际可用的去 `erase`，最后看集合是否为空。

1. 为什么这比 D2 里 `checkValidationLayerSupport()` 的嵌套 `strcmp` 更优雅？
2. 这两种写法的**时间复杂度**分别是什么？为什么教程说"性能差异无关紧要"？

---

### 5. `querySwapChainSupport` 查询的三类属性

填表：

| 属性 | 查询函数 | 返回单个结构体还是列表？ |
|---|---|---|
| 基本能力 | | |
| 表面格式 | | |
| 呈现模式 | | |

**追问**：为什么前两个列表查询要用 `if (count != 0)` 判断？

---

### 6. 顺序的重要性（核心考点）

`isDeviceSuitable` 里为什么必须写成：

```cpp
bool swapChainAdequate = false;
if (extensionsSupported) {                          // ← 为什么要有这个 if
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
}
```

如果去掉那个 `if`，在一个**不支持** `VK_KHR_swapchain` 的设备上会发生什么？

<details>
<summary>提示</summary>
想想验证层会怎么看待"调用了一个其依赖扩展未启用的设备扩展函数"。
把这个结论推广成一条通用原则：<b>什么时候可以查询一项能力的细节？</b>
</details>

---

### 7. 四种呈现模式

填表：

| 模式 | 会撕裂吗 | 会阻塞应用吗 | OpenGL 里的等价物 |
|---|---|---|---|
| `IMMEDIATE` | | | |
| `FIFO` | | | |
| `FIFO_RELAXED` | | | |
| `MAILBOX` | | | |

**追问 1**：规范保证哪一种模式**一定**存在？这为什么决定了 `chooseSwapPresentMode` 的写法？
**追问 2**：教程说 MAILBOX"通常被称为三重缓冲，尽管仅存在三个缓冲区并不一定意味着帧速率已解锁"。这句话什么意思？想想"飞行帧数"（frames in flight）这个概念。

---

### 8. 高 DPI 陷阱（核心考点）

1. `capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()` 表示什么？
2. 为什么不能直接把 `WIDTH` / `HEIGHT` 塞进 `VkExtent2D`？
3. GLFW 的"屏幕坐标"和"像素"有什么区别？哪个是 Vulkan 要的？
4. `std::clamp` 在那段代码里起什么作用？

---

### 9. 动手题：你的显示器缩放

跑讲义第 5.3 节的实验（打印 `glfwGetWindowSize` 和 `glfwGetFramebufferSize`），把结果贴在这里：

```
窗口尺寸(屏幕坐标): ___ x ___
帧缓冲(像素)      : ___ x ___
```

**追问**：两个数字相同吗？如果不相同，说明什么？如果相同，是不是说明 `chooseSwapExtent` 里的 `else` 分支在你的机器上永远不会执行？

<details>
<summary>提示</summary>
第二问要小心。`currentExtent` 是不是特殊值，取决于窗口管理器，<b>不取决于</b>你的 DPI 缩放。
两者是两个独立的判断。
</details>

---

### 10. 生命周期

D4 结束时，画出完整的**销毁顺序**，并说明每一步的理由：

```
? → ? → ? → ? → ? → ?
```

**追问**：`VkSurfaceKHR` 依赖 `VkInstance` 还是 `VkDevice`？那为什么从 D5 起它必须排在 `VkDevice` 之后销毁？

---

### 11. 为什么这个单元看不到画面

D4 结束时程序跑通了，但屏幕上一片漆黑（或者根本没窗口内容）。请解释：**此刻你已经拥有了哪些对象？还缺哪些才能画出第一个像素？**

<details>
<summary>提示</summary>
对照 <code>docs/opengl-to-vulkan.md</code> 里的"图 A：对象依赖图"，看看 D4 完成时你点亮了哪些节点。
</details>

---

### 12. 延伸思考

OpenGL 用 `wglSwapIntervalEXT(0|1)` 一个 bool 就搞定了垂直同步，Vulkan 给了你四个模式和图像数量的控制权。

- 这对**竞技游戏**意味着什么？（想想输入延迟 vs 撕裂的权衡）
- 这对**手机/笔记本**意味着什么？（想想电池）
- 教程说 MAILBOX 是"很好的折衷"。什么情况下你会**故意**选 `FIFO` 而不是 MAILBOX？

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
| 12 | | |
