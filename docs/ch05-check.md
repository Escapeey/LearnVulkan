# D5 自检题

> 合上代码和教程先自己答。全部答得出，再把 D5 打勾。

---

### 1. `VkImage` / `VkImageView` / `VkSampler`（核心考点）

1. 三者各自负责什么？
2. OpenGL 里它们对应什么？为什么 OpenGL 只需要一个对象？
3. **举两个例子**说明"同一块 `VkImage` 数据需要多种 view"的场景。
4. 在 D5 里，这三个对象你各创建了几个？分别是谁创建的？

---

### 2. 交换链图像的所有权

| 对象 | 谁创建 | 你要销毁吗 |
|---|---|---|
| `VkSwapchainKHR` | | |
| `VkImage`（交换链里的） | | |
| `VkImageView` | | |

**追问**：为什么交换链里的 `VkImage` 不需要销毁？如果销毁了会发生什么？

---

### 3. `imageCount` 的三个陷阱

1. 为什么是 `minImageCount + 1` 而不是 `minImageCount`？
2. `maxImageCount == 0` 表示什么？如果漏掉 `> 0` 这个判断会发生什么？
3. 你**请求**了 `imageCount` 张，就一定能拿到这么多吗？代码里怎么体现这一点？

<details>
<summary>提示</summary>
第 3 问想想 <code>vkGetSwapchainImagesKHR</code> 为什么要调用两次，以及 <code>imageCount</code> 这个变量在第二次调用后变成了什么。
</details>

---

### 4. `imageSharingMode`

1. `EXCLUSIVE` 和 `CONCURRENT` 的区别是什么？哪个性能更好？为什么？
2. `CONCURRENT` 分支为什么要设置 `queueFamilyIndexCount = 2` 和 `pQueueFamilyIndices`？
3. **在你这台机器上（单块 Intel UHD 630），会走哪个分支？为什么？**

<details>
<summary>提示</summary>
第 3 问回到 D3 讲义第 4 节关于 graphicsFamily / presentFamily 的讨论。
</details>

---

### 5. ⭐ `subresourceRange`（这个必须记牢）

写出五个字段并解释各自含义：

```
aspectMask     = ?
baseMipLevel   = ?
levelCount     = ?
baseArrayLayer = ?
layerCount     = ?
```

**追问 1**：如果把 `levelCount` 和 `layerCount` 都写成 `0` 会怎样？错误会立刻暴露还是延后暴露？
**追问 2**：列出后续至少 3 个会用到这个结构体的章节，以及各自用来干什么。

---

### 6. `components` 与通道调换

1. `VK_COMPONENT_SWIZZLE_IDENTITY` 是什么意思？
2. 这个功能对应 OpenGL 的什么？举一个实际用它的场景。

---

### 7. 呈现相关字段

解释这三个字段的作用，以及为什么教程选了这些值：

| 字段 | 教程的值 | 为什么 |
|---|---|---|
| `preTransform` | `capabilities.currentTransform` | |
| `compositeAlpha` | `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR` | |
| `clipped` | `VK_TRUE` | |

**追问**：`clipped = VK_TRUE` 换来的是性能。什么情况下你会设成 `VK_FALSE`？

---

### 8. `oldSwapchain`

1. D5 里 `createInfo.oldSwapchain` 是什么值？
2. 这个字段为什么存在？它在哪一章会被真正用到？

---

### 9. 动手题 A：教程要求的报错实验

删掉 `createInfo.imageExtent = extent;`，开着验证层跑一次。把报错原文贴在这里：

```
（你实际看到的验证层输出）
```

**追问**：这个实验说明了验证层的什么能力？对比 OpenGL 里同样错误的表现。

---

### 10. 动手题 B：泄漏实验

把 `cleanup()` 里销毁 `swapChainImageViews` 的循环注释掉，跑一次并把报错贴在这里：

```
（你实际看到的验证层输出）
```

**追问**：报错里有没有告诉你"泄漏了几个对象、是什么类型"？这对排查真实项目的资源泄漏意味着什么？

---

### 11. 动手题 C：记录你的硬件参数

在 `createSwapChain` 末尾打印：

```cpp
std::cout << "交换链图像数量: " << swapChainImages.size() << "\n";
std::cout << "图像格式: " << swapChainImageFormat << "\n";
std::cout << "图像尺寸: " << swapChainExtent.width << " x " << swapChainExtent.height << "\n";
```

把结果贴在这里：

```
交换链图像数量: ___
图像格式: ___
图像尺寸: ___ x ___
```

**追问**：
- 图像数量等于你请求的数量吗？
- 图像尺寸和 D4 里打印的帧缓冲尺寸一致吗？和 `WIDTH`/`HEIGHT` 呢？

---

### 12. 生命周期

画出 D5 结束时的完整销毁顺序（比 D4 多了两个对象）：

```
? → ? → ? → ? → ? → ? → ? → ?
```

并说明为什么 `vkDestroySwapchainKHR` 必须排在 `vkDestroySurfaceKHR` 之前。

---

### 13. 延伸思考

现在你手上有一堆"能画但没画"的图像。

- 如果交换链有 3 张图像，而你的应用每帧只画 1 张，那么第 3 张在干什么？
- 教程在 D12 会讲"飞行帧数"（frames in flight）。猜一猜：**交换链图像数量**和**飞行帧数**是同一个概念吗？

> 这题现在答不完整很正常，先写下你的猜想，D12 学完再回来对照。

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
| 13 | | |
