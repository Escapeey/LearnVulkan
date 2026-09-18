# D3 讲义：物理设备 + 队列族 + 逻辑设备

> 教程章节：[物理设备和队列族](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Setup/Physical_devices_and_queue_families) ·
> [逻辑设备和队列](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Setup/Logical_device_and_queues)
>
> 前置：D2 完成（`instance` 和 `debugMessenger` 已经能创建和销毁）
>
> 📊 **你这台机器的实测硬件数据**（交换链能力 / 队列族 / 呈现模式 / 深度格式）：[`your-gpu.md`](your-gpu.md)
> 代码骨架：在 D2 的 `src/hello_triangle.cpp` 上继续加（D2 通过后我会补上 TODO 标记）
> 自检题：`docs/ch03-check.md`

---

## 0. 本单元你要亲手写的

| 函数 | 难度 | 说明 |
|---|---|---|
| `pickPhysicalDevice()` | ★★ | 又见两段式枚举 |
| `isDeviceSuitable()` | ★★ | 现在是 `return true`，但它会一直长大到 D19 |
| `findQueueFamilies()` | ★★★ | 本单元核心，第一次用 `std::optional` |
| `createLogicalDevice()` | ★★★ | 4 个结构体 + 一个生命周期陷阱 |
| 更新 `cleanup()` | ★ | 加一行 `vkDestroyDevice` |

---

## 1. 全景：Vulkan 的三层"设备"模型

```
VkInstance                    ← D2，程序与 Vulkan 的连接
   │
   ├── VkPhysicalDevice       ← 系统里的一块物理 GPU（只读句柄，不可创建/销毁）
   │       │
   │       └── VkDevice       ← 逻辑设备："我与这块 GPU 的会话"（你创建，你销毁）
   │               │
   │               └── VkQueue ← 提交命令的通道（随 device 隐式创建/销毁）
   │
   └── VkSurfaceKHR           ← D4
```

### 四个对象的本质区别（**这是本单元最重要的认知**）

| 对象 | 是什么 | 谁创建 | 谁销毁 |
|---|---|---|---|
| `VkInstance` | 你的程序 ↔ loader ↔ 驱动的连接 | 你（`vkCreateInstance`） | 你（`vkDestroyInstance`） |
| `VkPhysicalDevice` | **系统里的一块物理显卡，只读句柄** | 系统（枚举出来而已） | **随 instance 隐式销毁，你管不着** |
| `VkDevice` | 逻辑设备，你与某块 GPU 的工作会话 | 你（`vkCreateDevice`） | 你（`vkDestroyDevice`） |
| `VkQueue` | 向 GPU 提交命令的通道 | 随 `VkDevice` 自动创建 | **随 device 隐式销毁，你管不着** |

> **反直觉点一**：`VkPhysicalDevice` 不是你"创建"的。它是 `vkEnumeratePhysicalDevices` 从系统里**枚举**出来的、只读的句柄。你既不能创建它，也不能销毁它。
>
> **反直觉点二**：`VkQueue` 你只能"取句柄"（`vkGetDeviceQueue`），不能创建也不能销毁。它在 `vkCreateDevice` 时按你请求的队列族自动生成。

---

## 2. OpenGL 对照

| | OpenGL | Vulkan |
|---|---|---|
| 系统里有几块 GPU | **一个**（驱动替你选的，SLI/CrossFire 也只是合并呈现） | **N 块，全部枚举给你** |
| 怎么选 | 驱动面板 / 电源设置 / 应用配置 | 你写 `isDeviceSuitable()` 或 `rateDeviceSuitability()` |
| 能同时用多块吗 | 不能（除非厂商扩展） | 能，各自创建 `VkDevice` |
| 能创建多个"会话"吗 | 不能（一个 context） | 能，同一 GPU 上可创建多个 `VkDevice` |
| 命令提交 | `glDrawElements` 内部隐含 | 显式查队列族 → `vkGetDeviceQueue` → `vkQueueSubmit` |
| "队列族"概念 | ❌ 完全不可见 | 显式，且是你**必须**掌握的分类体系 |

### 为什么 OpenGL 里你从没关心过这些？

因为 `wglCreateContext` 的时候驱动就替你挑了 GPU。**你在 NVIDIA/Intel 控制面板里改的"高性能 GPU / 省电 GPU"，就是 OpenGL 唯一暴露给你的设备选择机制** —— 而且它根本不算 API 的一部分。

Vulkan 把选择权交给你，也把责任交给你。在只有一块 GPU 的机器上这看起来是多余的；但在双显卡笔记本（Intel 集显 + NVIDIA 独显）上，选错设备意味着性能差一个数量级。

**你的机器**：只有一块 Intel UHD Graphics 630，`vkEnumeratePhysicalDevices` 会返回 1。所以选择逻辑很简单。但**建议你按 `rateDeviceSuitability` 评分版实现**——因为这份代码换到双显卡笔记本上时，评分版不会选错。

---

## 3. `isDeviceSuitable()`：一个会不断长大的函数

教程现在把它写成 `return true`，因为它还没教你怎么判断。但它在后续章节会一路扩展：

| 章节 | 会加进 `isDeviceSuitable` 的检查 |
|---|---|
| **D3（现在）** | `indices.isComplete()` —— 有没有图形队列族 |
| D5 | 有没有 `VK_KHR_swapchain` 扩展 + 交换链的格式/呈现模式是否非空 |
| D19 | 深度格式 `VK_FORMAT_D32_SFLOAT` 是否被支持 |
| D22 | 多重采样是否支持 |

**所以别小看这个"return true"**。它是设备选择的策略入口，后面会变成这样：

```cpp
bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    // D3:  return indices.isComplete();
    // D5:  + checkDeviceExtensionSupport(device) + swapChainAdequate
    // D19: + findSupportedFormat(device, ...) 可用
    return indices.isComplete();
}
```

### 评分版（教程给的可选做法，建议你实现）

```cpp
int rateDeviceSuitability(VkPhysicalDevice device) {
    int score = 0;
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;      // 独立显卡有显著性能优势
    }
    score += deviceProperties.limits.maxImageDimension2D;   // 最大纹理会影响画质
    if (!deviceFeatures.geometryShader) return 0;           // 硬性要求：没有就淘汰
    return score;
}
```

用 `std::multimap<int, VkPhysicalDevice>` 排序，取 `rbegin()`（最高分）。**注意 `rbegin()->first > 0` 这个判断**——0 分表示"不满足硬性要求"，不是"最好的那个"。

---

## 4. 队列族：Vulkan 的资源分类体系

### 什么是队列族

> Vulkan 中的几乎每个操作，从绘制到上传纹理，都需要将命令提交到队列。队列有不同的类型，这些类型源自不同的**队列族**，并且每个队列族仅允许命令的子集。

关键点：**一个队列族可以同时支持多种能力**（用位掩码表示）：

| 标志 | 含义 |
|---|---|
| `VK_QUEUE_GRAPHICS_BIT` | 图形命令（画三角形） |
| `VK_QUEUE_COMPUTE_BIT` | 计算命令（D23） |
| `VK_QUEUE_TRANSFER_BIT` | 内存传输（D14 的 staging buffer） |
| `VK_QUEUE_SPARSE_BINDING_BIT` | 稀疏内存绑定 |

`queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT` —— 这就是判断方法（位与）。

`queueCount` 字段是该族**最多能创建多少个队列**。教程只创建 1 个：

> *当前可用的驱动程序只允许您为每个队列族创建少量队列，而您实际上不需要超过一个。这是因为您可以在多个线程上创建所有命令缓冲区，然后使用单个低开销调用在主线程上一次性提交它们。*

**这句话是理解 Vulkan 并发的钥匙**：Vulkan 的多线程策略是"**多线程录制命令缓冲，单线程提交**"，而不是"多线程各自提交"。这点在 D11 你会感受更深。

### `std::optional`：为什么不能用魔法值

教程原话：

> 实际上不可能使用魔术值来指示队列族的不存在，因为理论上 `uint32_t` 的任何值都可能是有效的队列族索引，**包括 `0`**。

所以用 C++17 的 `std::optional<uint32_t>`：

```cpp
QueueFamilyIndices indices;
if (!indices.graphicsFamily.has_value()) { /* 这个设备不能用 */ }
uint32_t idx = indices.graphicsFamily.value();   // 没有值时会抛 std::bad_optional_access
```

⚠️ **别忘了 `#include <optional>`**。漏了会报 `std::optional` 不是 `std` 的成员——MSVC 的报错信息有时候不直接指向这里。

### 结构体化的设计（现在就要做对）

教程特意把返回值从 `uint32_t` 改成 `QueueFamilyIndices` 结构体，理由是"接下来的章节里我们还要找另一个队列"。**照做**，而且要理解为什么：

- **D3**：只有 `graphicsFamily`
- **D5**：加 `presentFamily`。⚠️ 在单 GPU 集成显卡上，**这两个几乎肯定是同一个索引值**；在独立显卡上可能是不同的
- **D23**：加 `computeFamily`

`isComplete()` 也会跟着长：

```cpp
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() {
        return graphicsFamily.has_value();
        // D5 变成: graphicsFamily.has_value() && presentFamily.has_value();
    }
};
```

---

## 5. 逻辑设备创建：四个结构体

```
VkDeviceQueueCreateInfo   ← 我要从哪个队列族要几个队列、优先级多少
        ↓ pQueueCreateInfos
VkPhysicalDeviceFeatures  ← 我要用哪些可选特性（现在全 false）
        ↓ pEnabledFeatures
VkDeviceCreateInfo        ← 汇总
        ↓
vkCreateDevice(physicalDevice, &createInfo, nullptr, &device)
```

### 三个必须注意的点

**① 队列优先级是必需的，即使只有一个队列**

```cpp
float queuePriority = 1.0f;
queueCreateInfo.pQueuePriorities = &queuePriority;
```

Vulkan 允许用 `0.0` ~ `1.0` 的浮点数影响命令缓冲的执行调度。即使只有一个队列也必须提供。

> ⚠️ **生命周期陷阱（和 D2 的 `debugCreateInfo` 一模一样）**：`queuePriority` 必须是**在 `vkCreateDevice` 返回之后仍然存活**的变量。如果你写成 `queueCreateInfo.pQueuePriorities = &1.0f;` —— 编译能过，运行崩溃。

**② 设备的验证层字段：新实现会忽略，但还是要设**

```cpp
if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
} else {
    createInfo.enabledLayerCount = 0;
}
```

教程原话：*"以前的 Vulkan 实现区分了实例和设备特定的验证层，但这**不再是这种情况**……会被最新的实现忽略。但是，为了与旧的实现兼容，最好还是设置它们。"*

**明白这是历史遗留就够了**，别以为是必需的。

**③ 设备扩展现在是 0，D5 会变成 `VK_KHR_swapchain`**

```cpp
createInfo.enabledExtensionCount = 0;   // D5: 换成 {"VK_KHR_swapchain"}
```

`VK_KHR_swapchain` 是**设备扩展**（不是实例扩展！），因为"能不能呈现到窗口"是设备的能力。有些设备只支持计算，没有这个扩展。

---

## 6. 检索队列句柄

```cpp
vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
```

参数：逻辑设备、队列族索引、**族内的队列索引**（我们只要了 1 个，所以是 `0`）、输出变量。

**队列不需要手动销毁。** 它是 `VkDevice` 的一部分。

---

## 7. 生命周期与清理顺序（D3 更新版）

```cpp
void cleanup() {
    vkDestroyDevice(device, nullptr);          // ← D3 新增
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}
```

**需要手动销毁的**：`VkDevice` → `VkDebugUtilsMessengerEXT` → `VkInstance`

**不需要手动销毁的**：
- `VkPhysicalDevice` —— 随 instance 隐式销毁
- `VkQueue` —— 随 device 隐式销毁

> **建议**：在 D3 结束时，故意把 `vkDestroyDevice` 注释掉跑一次，看验证层怎么报资源泄漏。这和 D2 的实验是同一个套路——**验证层会告诉你"哪个类型的对象在什么时候被销毁时还活着"**。

---

## 8. 本单元最容易踩的坑

| # | 现象 | 根因 | 处理 |
|---|---|---|---|
| 1 | `std::optional` 不是 `std` 的成员 | 忘了 `#include <optional>` | 加上 |
| 2 | 崩溃在 `vkCreateDevice` 或之后 | `pQueuePriorities` 指向的 float 已离开作用域 | 用局部变量，且活到 `vkCreateDevice` 之后 |
| 3 | 抛 `std::bad_optional_access` | 没检查 `has_value()` 就调 `.value()` | 先 `isComplete()` 判断 |
| 4 | 验证层报 `queueFamilyIndex` 越界 | 循环里 `i` 的自增位置写错（`break` 在 `i++` 之前） | 教程的写法是 `break` 检查放在末尾，注意抄对 |
| 5 | 设备选错（笔记本双显卡） | 用了"第一个满足条件的"而不是评分 | 实现 `rateDeviceSuitability` |
| 6 | 退出时验证层报 `VkDevice` 泄漏 | `cleanup()` 漏了 `vkDestroyDevice` | 加上 |
| 7 | `queueCreateInfoCount` 与数组长度不匹配 | 手写数字写错 | 用 `1` 或 `static_cast<uint32_t>(...)` |

---

## 9. 建议你做的额外实验（对单 GPU 机器尤其值）

你的机器只有一块 GPU，所以"设备选择"看起来很无趣。**但你可以把它变得很有信息量**——在 `pickPhysicalDevice()` 里加一段打印：

```cpp
// 伪代码，你来补全
for (每个物理设备) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);
    打印 props.deviceName / deviceType / apiVersion / driverVersion

    // 再用一次两段式枚举打印这个设备的所有队列族：
    for (每个队列族) {
        打印 queueFlags（用位与逐个判断 GRAPHICS / COMPUTE / TRANSFER / SPARSE）
        打印 queueCount
    }
}
```

**这项练习的价值**：

1. 又练一遍两段式枚举（`vkGetPhysicalDeviceQueueFamilyProperties`）
2. 你会亲眼看到 Intel UHD 630 上有几个队列族、各自支持什么 —— 后面 D5（present queue）、D14（transfer queue）、D23（compute queue）都要靠这份数据做决策
3. 排查"为什么我的代码在别人机器上崩了"时，这是第一手信息

跑完把你机器上的输出贴给 `PROGRESS.md`，这是很好的学习记录。

---

## 10. 本单元完成标志

- [ ] 程序跑到 D3 结束不发生异常
- [ ] `isDeviceSuitable()` 返回 `indices.isComplete()`
- [ ] 控制台打印出了你的 GPU 信息和队列族信息（第 9 节的实验）
- [ ] 故意注释掉 `vkDestroyDevice` 跑一次，看懂验证层报的泄漏
- [ ] 能回答 `docs/ch03-check.md` 全部问题
- [ ] `PROGRESS.md` 里 D3 打勾 + 3 行总结
