# 你这台机器的实测硬件数据

> 这些是**我在这台机器上实跑探针得到的真实数值**（不是文档抄的，也不是猜的）。
> 后面的单元会反复引用这份数据 —— 你实现完之后可以拿自己的输出对照，一眼看出对错。
>
> 探针程序本身已删除（它包含了 D3/D4/D5 的完整实现，留在仓库里等于给你答案）。
>
> 设备：**Intel(R) UHD Graphics 630**（集成显卡，UMA 统一内存）

---

## ⚠️ 先看这两条：它们会改变你的预期

### 1. 你的显卡**不支持 MAILBOX 呈现模式**

可用的呈现模式只有两个：

```
IMMEDIATE
FIFO        ← 规范保证一定存在
```

**没有 `MAILBOX`。**

教程的 `chooseSwapPresentMode()` 逻辑是"优先 MAILBOX，没有就回退 FIFO"。所以在你这台机器上：

> **无论你怎么写，最终都会拿到 `FIFO`（垂直同步）。这是正常的，不是你写错了。**

如果你照着教程写完发现拿到的是 FIFO，别怀疑自己 —— 这就是这块显卡的能力。想验证的话，把你枚举到的 `availablePresentModes` 打印出来，应该正好是上面两个。

### 2. 交换链尺寸被**固定**成 800×600

```
currentExtent  : 800 x 600      ← 不是 UINT32_MAX
minImageExtent : 800 x 600
maxImageExtent : 800 x 600
```

`currentExtent.width` **不等于** `std::numeric_limits<uint32_t>::max()`，这意味着：

> `chooseSwapExtent()` 会走 **`return capabilities.currentExtent;` 那个分支**，
> 你写的 `else` 分支（`glfwGetFramebufferSize` 那段）**在你这台机器上永远不会被执行**。

**但仍然要把 else 分支写对** —— 它是可移植性代码，换台机器/换个窗口管理器就会走到。而且这是自检题 D4 第 9 题的考点：**`currentExtent` 是不是特殊值，跟你的 DPI 缩放无关，是两件独立的事。**

---

## 1. 设备属性

| 项 | 值 | 相关的单元 |
|---|---|---|
| deviceName | `Intel(R) UHD Graphics 630` | D3 |
| deviceType | `INTEGRATED_GPU`（集显） | D3 的 `rateDeviceSuitability` |
| apiVersion | `1.3.215` | — |
| driverVersion | `0.404.2115` | — |
| maxImageDimension2D | `16384` | D17 纹理 |
| framebufferColorSampleCounts | `0x1f` = 1/2/4/8/16 | D22 MSAA |
| framebufferDepthSampleCounts | `0x1f` = 1/2/4/8/16 | D22 MSAA |

**D22 推论**：`getMaxUsableSampleCount()` 在你的机器上会返回 **16**（颜色和深度都支持到 16x）。

## 2. 关键特性

全部可用：`geometryShader` ✅ · `samplerAnisotropy` ✅ · `fillModeNonSolid` ✅ · `wideLines` ✅ · `depthClamp` ✅

## 3. 内存（UMA 统一内存 —— 集显的特点）

```
heap 0: 16239 MiB  DEVICE_LOCAL        ← 只有一个堆，16 GB
内存类型共 3 种：
  type 0: DEVICE_LOCAL
  type 1: DEVICE_LOCAL | HOST_VISIBLE | HOST_COHERENT   ← findMemoryType 会选中它
  type 2: DEVICE_LOCAL | HOST_VISIBLE | HOST_COHERENT
```

**和独显的关键差异**：集显**没有独立显存**，CPU 和 GPU 共享同一块内存。所以：

- **D14 的 `findMemoryType` 会返回 type 1**（同时满足 `HOST_VISIBLE | HOST_COHERENT`）
- 集显上 staging buffer 的性能收益远不如独显明显（因为内存本来就是同一块），但**教程的流程仍然要照做** —— 你要学的是这套机制，不是它在这块显卡上能省多少
- 你以后在独显机器上跑同样的代码，`findMemoryType` 会返回**另一个**类型（非 DEVICE_LOCAL 的 host-visible 类型），这时 staging buffer 才真正起作用

## 4. ⭐ 队列族（只有 1 个！）

```
族 0: queueCount = 1   flags = [GRAPHICS COMPUTE TRANSFER SPARSE]   可呈现 = 是
```

**这只有一行，但信息量很大**：

| 结论 | 影响的单元 |
|---|---|
| `graphicsFamily = 0`，`presentFamily = 0` —— **同一个族** | **D5 的 `imageSharingMode` 走 `EXCLUSIVE`** |
| 图形 / 计算 / 传输**全在同一族** | D23 计算着色器不用另找队列族 |
| 每族 `queueCount = 1` | **你没法为计算单独创建一个队列** —— D23 会直接复用图形队列 |
| `queueFlags` 含 `SPARSE` | 无关，教程不用稀疏绑定 |

> **D5 的实测结论**：`indices.graphicsFamily != indices.presentFamily` 为 **false**，
> 所以 `createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE`。
> 我给你的骨架里那两个分支都要写，但**实际执行的是 EXCLUSIVE**。

> **D23 的实测结论**：教程的计算着色器章节会找 `VK_QUEUE_COMPUTE_BIT` 的族。
> 在你这台机器上它就是族 0（和图形同一个），教程会走"复用图形队列"的路径。

## 5. DPI / 高 DPI 陷阱

```
窗口尺寸(屏幕坐标) : 800 x 600
帧缓冲(像素)       : 800 x 600
```

**两者相同 → 你的显示缩放是 100%。**

所以 `glfwGetFramebufferSize` 和 `WIDTH/HEIGHT` 在你的机器上数值一样。**但你仍然必须用前者** —— 这只说明你现在没开缩放。如果你以后把 Windows 显示缩放调到 125%/150%，两者就会分叉，不写对的代码会立刻表现为画面拉伸/只占一角。

## 6. 交换链能力

| 项 | 值 | 说明 |
|---|---|---|
| `minImageCount` | **2** | |
| `maxImageCount` | **64** | 不是 0，所以要做上限判断 |
| `currentExtent` | 800 × 600 | **不是特殊值**，见开头第 2 条 |
| `minImageExtent` | 800 × 600 | |
| `maxImageExtent` | 800 × 600 | 和 min 相同 → 尺寸被完全固定 |
| `currentTransform` | `0x1` = `IDENTITY` | 无旋转/翻转 |
| `supportedCompositeAlpha` | `0x9` = `OPAQUE \| INHERIT` | **含 OPAQUE ✅**，教程的选择可用 |
| `supportedUsageFlags` | `0x1f` | **含 `COLOR_ATTACHMENT ✅`** |

> **D5 的实测推论**：`imageCount = minImageCount + 1 = 3`。
> `maxImageCount = 64 > 0` 且 `3 <= 64`，所以那个上限判断**不会改变结果**。
> **实际会创建 3 张交换链图像。**

## 7. 表面格式（4 个）

| # | format | 值 | colorSpace | 说明 |
|---|---|---|---|---|
| 1 | 44 | `B8G8R8A8_UNORM` | 0 = `SRGB_NONLINEAR` | |
| 2 | **50** | **`B8G8R8A8_SRGB`** | **0 = `SRGB_NONLINEAR`** | **← 教程的首选组合，会命中这个** |
| 3 | 37 | `R8G8B8A8_UNORM` | 0 | |
| 4 | 43 | `R8G8B8A8_SRGB` | 0 | |

> **D4 的实测推论**：你的 `chooseSwapSurfaceFormat()` 会在**第二轮循环**（索引 1）
> 命中 `VK_FORMAT_B8G8R8A8_SRGB` + `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR` 并返回它。
>
> 所以 `swapChainImageFormat` 最终是 `VK_FORMAT_B8G8R8A8_SRGB`（枚举值 50）。
> **注意通道顺序是 B 在前** —— 但这对你无影响，因为 Vulkan 会按格式自动处理。

## 8. 呈现模式（只有 2 个）

```
IMMEDIATE
FIFO
```

见开头第 1 条。

## 9. 设备扩展

- 总数 **109** 个
- **含 `VK_KHR_swapchain` ✅** —— D4 必须把它加进 `deviceExtensions` 并启用

## 10. 深度格式支持（D19）

| format | 值 | `DEPTH_STENCIL_ATTACHMENT` (optimal tiling) |
|---|---|---|
| 126 | `D32_SFLOAT` | ✅ 支持 |
| 130 | `D32_SFLOAT_S8_UINT` | ✅ 支持 |
| 129 | `D24_UNORM_S8_UINT` | ✅ 支持 |

> **D19 推论**：教程的 `findSupportedFormat` 按优先级找
> `D32_SFLOAT` → `D32_SFLOAT_S8_UINT` → `D24_UNORM_S8_UINT`，
> 在你的机器上**第一个就命中 `D32_SFLOAT`**。

---

## 怎么自己复现这些数据

D3 讲义第 9 节有一个"打印所有物理设备和队列族"的练习 —— 做那个就能拿到第 1、4 节的数据。
D4/D5 实现完之后，你可以在 `createSwapChain` 末尾打印 `swapChainImageFormat` 和
`swapChainImages.size()`，对照本文第 6、7 节验证。

**如果你的输出和本文对不上**，先怀疑自己（这是最可能的原因），确认无误后告诉我 ——
那说明环境变了，值得查。
