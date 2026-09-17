# D3 自检题

> 合上代码和教程先自己答。全部答得出，再把 D3 打勾。

---

### 1. 三层设备模型（核心考点）

`VkPhysicalDevice` 和 `VkDevice` 有什么区别？为什么 Vulkan 要把它们分成两个对象？

<details>
<summary>提示</summary>
从"谁创建、谁销毁、能不能有多个"三个角度回答。再想想：如果只有一块 GPU，为什么还需要分两层？
</details>

---

### 2. 生命周期

D3 结束后，你一共有这些对象。哪些需要手动销毁，哪些不需要？为什么？

`VkInstance` / `VkPhysicalDevice` / `VkDevice` / `VkQueue` / `VkDebugUtilsMessengerEXT` / `GLFWwindow`

---

### 3. `std::optional`

为什么教程用 `std::optional<uint32_t>` 表示"队列族索引"，而不是用 `UINT32_MAX` 或 `-1` 之类的魔法值？

<details>
<summary>提示</summary>
教程原话提到 "包括 0"。一个合法的队列族索引可能是 0，那 0 还能当"无效值"用吗？
</details>

---

### 4. 队列族

1. 队列族的"能力"是用什么表示的？给出至少 4 种能力标志。
2. 一个队列族可以**同时**支持图形 + 计算 + 传输吗？怎么验证？
3. `queueFamily.queueCount` 字段的含义是什么？是"系统里有几个队列"还是别的？

---

### 5. 队列优先级

1. 为什么 `queuePriority` 即使只有一个队列也必须提供？
2. `queueCreateInfo.pQueuePriorities = &queuePriority;` 里那个 `float` 有什么**生命周期**要求？

<details>
<summary>提示</summary>
第 2 问和 D2 的 `debugCreateInfo` 是同一类陷阱。如果写成 <code>queueCreateInfo.pQueuePriorities = &1.0f;</code> 会怎样？
</details>

---

### 6. 前瞻题：D5 的 presentFamily

D5 会给 `QueueFamilyIndices` 加一个 `presentFamily`。

1. 在**你的机器**（单块 Intel 集成显卡）上，`graphicsFamily` 和 `presentFamily` 大概率是什么关系？
2. 在双显卡笔记本（Intel 集显 + NVIDIA 独显）上呢？
3. 既然两者**可能相同也可能不同**，创建逻辑设备时应该怎么处理才不会重复请求同一个队列族？

<details>
<summary>提示</summary>
想想 <code>std::set&lt;uint32_t&gt;</code> 的用途：它天然去重。教程在 D5 用一个 <code>uniqueQueueFamilies</code> 集合来构造 <code>VkDeviceQueueCreateInfo</code> 数组。
</details>

---

### 7. 实例扩展 vs 设备扩展

1. 两者有什么区别？
2. `VK_KHR_swapchain` 属于哪一类？为什么？

<details>
<summary>提示</summary>
问题不是"这个扩展在哪个列表里"，而是"这个能力是**整个 Vulkan 会话语境**的属性，还是**某一块具体显卡**的属性"。有些设备只能做计算，不能呈现——这说明了什么？
</details>

---

### 8. 设备的验证层字段

`VkDeviceCreateInfo::enabledLayerCount` / `ppEnabledLayerNames`：

1. 现代 Vulkan 实现会怎么处理这两个字段？
2. 既然会被忽略，教程为什么还要设置它们？

---

### 9. 多个逻辑设备

教程说"甚至可以从同一个物理设备创建多个逻辑设备"。什么场景下这样做有意义？

<details>
<summary>提示</summary>
想想一个应用同时需要：渲染、物理模拟用的计算、以及一个独立的编辑器预览视口。或者：一个库需要自己的隔离状态而不污染主应用的设备。
</details>

---

### 10. 会成长的函数

`isDeviceSuitable()` 现在只是 `return indices.isComplete()`。列出它在后续章节会新增的**至少 3 项**检查，并说明每项对应哪个章节。

<details>
<summary>提示</summary>
和"设备必须具备的能力"有关：能呈现吗？能画深度吗？能多重采样吗？能不能用某种图像格式？
</details>

---

### 11. 动手题：你机器的真实数据

跑第 9 节的实验（打印所有物理设备 + 所有队列族），把输出贴在这里：

```
（设备名称 / 设备类型 / API 版本 / 驱动版本）

（队列族 0：flags = ..., queueCount = ...）
（队列族 1：...）
```

**追问**：你的 Intel UHD 630 有几个队列族？图形和计算在同一个族里吗？传输呢？

> 这份数据后面 D5 / D14 / D23 都要用，别删。

---

### 12. 延伸思考

OpenGL 里你无法选择用哪块 GPU（只能通过驱动面板这类**非 API 手段**）。Vulkan 把它变成 API 的一部分。

- 这对**游戏引擎**意味着什么？（想想 Optimus 笔记本上"游戏跑在集显上"的经典问题）
- 这对**你的代码**意味着什么？（如果 `pickPhysicalDevice` 写错，程序在你自己机器上完全正常，在别人机器上性能腰斩——你打算怎么防？）

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
