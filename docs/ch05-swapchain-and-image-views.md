# D5 讲义：创建交换链 + 图像视图

> 教程章节：[交换链](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Presentation/Swap_chain)（后半：创建与取图像）·
> [图像视图](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Presentation/Image_views)
>
> 前置：D4 完成（`surface` 已创建，三个 `choose*` 函数已就绪）
> 自检题：`docs/ch05-check.md`
>
> **本单元结束后你仍然看不到画面**，但你会第一次拥有"可以往上画的图像"。

---

## 0. 本单元你要亲手写的

| 函数 / 成员 | 难度 | 说明 |
|---|---|---|
| `createSwapChain()` | ★★★ | 全教程最大的一个创建结构体 |
| 取交换链图像 | ★★ | 又一次两段式枚举（第 4 次） |
| `createImageViews()` | ★★ | 第一次接触 `subresourceRange` |
| 新增成员：`swapChain` / `swapChainImages` / `swapChainImageFormat` / `swapChainExtent` / `swapChainImageViews` | ★ | |
| 更新 `cleanup()` | ★ | 逆序销毁 + 循环销毁 image view |

---

## 1. 全景：D5 在整张图里的位置

```
VkSurfaceKHR            ← D4
     │
VkSwapchainKHR          ← D5（本单元）
     │
     ├── VkImage[]      ← D5 取出句柄（不是我们创建的，不用销毁）
     │        │
     │        └── VkImageView[]   ← D5 创建（我们要销毁）
     │
     └── VkRenderPass + VkFramebuffer   ← D9 / D10
```

---

## 2. ⭐ `VkImage` / `VkImageView` / `VkSampler`：一个被拆成三份的概念

这是本单元最重要的认知，也是 OpenGL 转 Vulkan 最需要"重新布线"的地方之一。

### OpenGL 里它们是一个对象

```cpp
GLuint tex;
glGenTextures(1, &tex);
glBindTexture(GL_TEXTURE_2D, tex);
glTexImage2D(...);                              // 数据 + 格式 + 尺寸
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ...);  // 采样方式
glTexParameteri(..., GL_TEXTURE_WRAP_S, ...);
// 就这么一个 tex，什么都塞在里面
```

### Vulkan 把它们拆成三个独立对象

| 对象 | 负责什么 | 在哪个单元 |
|---|---|---|
| `VkImage` | **内存里的像素数据**（尺寸、格式、tiling、用途） | D5（交换链图像）/ D17（纹理） |
| `VkImageView` | **怎么解释这块数据**（当 2D 还是立方体贴图？看哪一层？哪个 mip？哪个 aspect？） | **D5 本单元** |
| `VkSampler` | **怎么采样它**（过滤、寻址、各向异性） | D18 |

**为什么要拆开？** 因为同一块 `VkImage` 数据，你可能需要多种"看法"：

- 一张 2D 纹理 → 一个 `VIEW_TYPE_2D` 的 view
- 一张 6 面立方体贴图 → **同一块数据**上建一个 `VIEW_TYPE_CUBE` 的 view
- 深度+模板纹理 → 一个只看 `DEPTH_BIT` 的 view 和一个只看 `STENCIL_BIT` 的 view
- 立体渲染 → 每只眼睛一个 array layer 的 view

OpenGL 里这些要复制纹理或者反复改参数；Vulkan 里它们是轻量的、可共存的对象。

> **对你的直接影响**：D5 的交换链图像是驱动创建的（你只拿到 `VkImage` 句柄），
> 但**能不能用它、怎么用它，取决于你建的 `VkImageView`**。没有 view，那张图对渲染管线来说等于不存在。

---

## 3. `createSwapChain()`：全教程最大的创建结构体

### 3.1 先定图像数量

```cpp
uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
if (swapChainSupport.capabilities.maxImageCount > 0 &&
    imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
}
```

三个要点：

1. **`maxImageCount == 0` 表示"无上限"** —— 又一个"用 0 表示特殊含义"的约定（和 D4 的 `currentExtent == UINT32_MAX` 同理）。所以必须先判断 `> 0` 再比较，否则 `imageCount > 0` 恒真，会把数量压成 0。
2. **为什么 `minImageCount + 1`？** 教程原话：*"仅仅坚持这个最小值意味着我们有时可能需要等待驱动程序完成内部操作，然后才能获取另一个图像进行渲染。"* 多要一张给你留出流水线余量。
3. **注意措辞**：你请求的是**至少** `minImageCount` 张，驱动**可以给你更多**。这就是 D5 后面为什么要先查一次实际数量再 `resize`。

> 在你的 Intel UHD 630 上，`minImageCount` 大概率是 2 或 3。跑起来后把它打印出来看看。

### 3.2 填 `VkSwapchainCreateInfoKHR`

```cpp
VkSwapchainCreateInfoKHR createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
createInfo.surface = surface;

createInfo.minImageCount    = imageCount;
createInfo.imageFormat      = surfaceFormat.format;
createInfo.imageColorSpace  = surfaceFormat.colorSpace;
createInfo.imageExtent      = extent;
createInfo.imageArrayLayers = 1;
createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
```

| 字段 | 值 | 含义 |
|---|---|---|
| `imageArrayLayers` | `1` | 每个图像由几层组成。**除非做立体 3D，否则恒为 1** |
| `imageUsage` | `COLOR_ATTACHMENT_BIT` | 我们直接往它上面渲染 |
| `preTransform` | `capabilities.currentTransform` | 不做额外变换（如 90° 旋转/水平翻转） |
| `compositeAlpha` | `OPAQUE_BIT_KHR` | 忽略 alpha，不与系统里其他窗口混合 |
| `clipped` | `VK_TRUE` | 不关心被遮挡的像素颜色 → 性能更好 |
| `presentMode` | D4 选的 | |
| `oldSwapchain` | `VK_NULL_HANDLE` | **D12（交换链重建）才会用非空值** |

关于 `imageUsage` 教程给了一个前瞻提示：如果你想把画面先渲染到自己的图像做后处理，再传到交换链图像，那时 `imageUsage` 会需要 `VK_IMAGE_USAGE_TRANSFER_DST_BIT` 之类。**现在不用管，知道这个字段是可扩展的就行。**

### 3.3 `imageSharingMode`：`EXCLUSIVE` vs `CONCURRENT`

```cpp
QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamilyIndices;
} else {
    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;        // 可选
    createInfo.pQueueFamilyIndices   = nullptr;  // 可选
}
```

| 模式 | 语义 | 代价 |
|---|---|---|
| `EXCLUSIVE` | 图像一次只属于一个队列族，换族要**显式转移所有权** | **性能最好** |
| `CONCURRENT` | 图像可被多个队列族直接使用，无需转移 | 驱动要做额外同步，稍慢 |

**注意 D4 埋下的伏笔在这里兑现了**：D3 讲义里我特意预警过"`graphicsFamily` 和 `presentFamily` 可能是同一个值"。在你的单 GPU 集显上，**它们几乎必然是同一个索引** → 走 `EXCLUSIVE` 分支。

教程说这么做是为了避免在教程里引入"队列族所有权转移"这个复杂话题（D14 的 `copyBuffer` 会真正遇到它）。

> **`indices.graphicsFamily != indices.presentFamily` 这行比较的是两个 `std::optional<uint32_t>`**，
> 不是 `uint32_t`。`std::optional` 有 `operator!=`，比较的是"是否有值 + 值是否相等"。
> 这里两个都必然有值（否则 `isDeviceSuitable` 早就把它们淘汰了）。

### 3.4 提交并取图像

```cpp
if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
}

// 取出图像句柄（两段式枚举，第 4 次出现）
vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
swapChainImages.resize(imageCount);
vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

// 存下来，后面章节到处要用
swapChainImageFormat = surfaceFormat.format;
swapChainExtent      = extent;
```

**注意这里复用了 `imageCount` 变量**：第一次是你请求的数量，第二次被覆盖成**实际拿到的数量**。所以必须重新 `resize`——这正是 3.1 第 3 点说的原因。

### 3.5 🔬 教程要求的必做实验

> 尝试在启用验证层的情况下**删除 `createInfo.imageExtent = extent;` 这一行**，你会看到验证层立即捕获到错误并打印有用的消息。

**这个实验很有价值**，因为它展示了一件 OpenGL 做不到的事：**验证层知道每个字段的合法范围**，能告诉你"`imageExtent` 必须在 `[minImageExtent, maxImageExtent]` 内，你给的是 0×0"。OpenGL 里同样的错误只会给你一个黑屏。

做完记得改回来，并把报错原文记到 `docs/ch05-check.md`。

---

## 4. `createImageViews()`

```cpp
std::vector<VkImageView> swapChainImageViews;
swapChainImageViews.resize(swapChainImages.size());

for (size_t i = 0; i < swapChainImages.size(); i++) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image    = swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format   = swapChainImageFormat;

    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image views!");
    }
}
```

### 4.1 `components`：通道调换

`VK_COMPONENT_SWIZZLE_IDENTITY` = "不调换，原样映射"。

这个功能相当于 OpenGL 的 `glTexParameteri(GL_TEXTURE_SWIZZLE_RGBA, ...)`：比如把 RGB 全映射到 `.r` 得到灰度效果，或者把常量 `0`/`1` 映射到某个通道。**本教程不用，但你以后做单通道贴图（如遮蔽图、粗糙度图）时会用到。**

### 4.2 ⭐ `subresourceRange`：这个结构体你后面会见到几十次

```cpp
createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
createInfo.subresourceRange.baseMipLevel   = 0;
createInfo.subresourceRange.levelCount     = 1;
createInfo.subresourceRange.baseArrayLayer = 0;
createInfo.subresourceRange.layerCount     = 1;
```

它回答的是"**这块图像数据的哪个子区间**"。Vulkan 把一张图像切成了多维子区间：

```
aspect  : COLOR / DEPTH / STENCIL      （同一块数据的不同用途）
mipLevel: 0 .. log2(max(w,h))          （分辨率层级，D21 会用到）
layer   : 0 .. arrayLayers-1           （数组层 / 立方体贴图的 6 个面）
```

**这个结构体在后续章节反复出现**：

| 章节 | 出现场景 |
|---|---|
| D17 | `transitionImageLayout` / `copyBufferToImage` —— 所有 barrier 都要指定 subresource range |
| D19 | 深度图像：`aspectMask` 变成 `VK_IMAGE_ASPECT_DEPTH_BIT` |
| D21 | Mipmap：`levelCount` 不再是 1，而是每级单独转换 |
| D22 | MSAA：`layerCount` 相关 |
| D18 | 纹理的 image view |

**现在花两分钟把这个结构体的五个字段记牢，后面会省很多时间。**

> ⚠️ 一个经典错误：`levelCount` / `layerCount` 写成 0。0 表示"没有子区间"，
> 创建出来的 view 是无效的，后面用它做渲染目标时验证层才会报错——**报错点离错误点很远**。

### 4.3 交换链图像 vs 图像视图：谁该销毁

| 对象 | 谁创建 | 要销毁吗 |
|---|---|---|
| `VkSwapchainKHR` | 你 | ✅ `vkDestroySwapchainKHR` |
| `VkImage`（交换链里的） | 驱动（随交换链） | ❌ **不要**，随交换链自动清理 |
| `VkImageView`（你建的） | 你 | ✅ `vkDestroyImageView`（**每个都要**） |

教程原话：*"图像是由实现为交换链创建的，并且它们将在交换链被销毁后自动清理，因此我们不需要添加任何清理代码。"*

**但视图是我们显式创建的，所以必须显式销毁。**

---

## 5. 更新后的 `cleanup()` 顺序

```cpp
void cleanup() {
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}
```

**依赖关系决定顺序**：

```
imageView 依赖 swapchain 里的 image
swapchain 依赖 device 和 surface
device    依赖 physicalDevice（不需要手动销毁）
surface   依赖 instance
debugMessenger 依赖 instance
instance  是根
```

> **建议再做一次 D2/D3 那种"故意写错"实验**：把 `vkDestroyImageView` 那个循环注释掉，跑一次，看验证层怎么报。
> 随着对象变多，**验证层的泄漏报告会告诉你"哪个类型的对象、有多少个、在销毁 VkDevice 时还活着"**——
> 这是你排查资源泄漏最直接的工具。

---

## 6. 本单元最容易踩的坑

| # | 现象 | 根因 | 处理 |
|---|---|---|---|
| 1 | `vkCreateSwapchainKHR` 报 `VK_ERROR_INITIALIZATION_FAILED` | `imageCount` 超出范围（见 3.1 的 `maxImageCount > 0` 判断） | 检查那两个 if |
| 2 | 验证层报 `imageExtent` 非法 | 忘了 `createInfo.imageExtent = extent;`（教程让你故意试的） | 补上 |
| 3 | 创建后 `swapChainImages.size() != imageCount` | 忘了重新 `vkGetSwapchainImagesKHR` 覆盖 `imageCount` | 两段式枚举两次都要写对 |
| 4 | 崩溃/验证层报错在 D9 才出现，但根因在 D5 | `subresourceRange` 的 `levelCount` / `layerCount` 写成了 0 | 检查 4.2 |
| 5 | 退出时报 `VkImageView` 泄漏 × N | 忘了循环销毁，或循环写在了 `vkDestroySwapchainKHR` **之后** | 顺序见第 5 节 |
| 6 | `EXCLUSIVE` 分支下 `queueFamilyIndexCount` 没设 0 | 看了 CONCURRENT 分支就照抄 | 两个分支都要写全 |
| 7 | 画面撕裂/卡顿 | 呈现模式选成了 `IMMEDIATE` | 回到 D4 的 `chooseSwapPresentMode` |

---

## 7. 本单元完成标志

- [ ] `createSwapChain()` 成功，验证层无 error
- [ ] 完成"删掉 `imageExtent` 看报错"的实验，并把原文记进自检题
- [ ] `createImageViews()` 为每张交换链图像都建了 view
- [ ] 完成"注释掉 imageView 销毁"的泄漏实验
- [ ] 打印了 `swapChainImages.size()` 和 `swapChainImageFormat` / `swapChainExtent`，并记录
- [ ] 能说清 `VkImage` / `VkImageView` / `VkSampler` 三者的分工
- [ ] 能说清 `subresourceRange` 五个字段的含义
- [ ] 能回答 `docs/ch05-check.md` 全部问题
- [ ] `PROGRESS.md` 里 D5 打勾 + 3 行总结

> **还是看不到画面 —— 这是正常的。** 你现在有了"容器"和"看法"，但还没有渲染通道（D9）、
> 管线（D10）、帧缓冲和命令缓冲（D11）。**D11 是 M1 的分界线**，还有 6 个单元。

---

## 8. 前瞻：D6

D6 进入《图形管线基础》，先做两件事：

1. 读**简介**章节，理解图形管线的完整阶段划分和哪些是"可编程"的、哪些是"固定功能"
2. **着色器模块**：把 GLSL 源码用 `glslc` 编译成 SPIR-V，再用 `vkCreateShaderModule` 加载

本工程的 CMake 已经把 `glslc` 接好了（`cmake/Shaders.cmake` + `shaders/` 目录），
D6 你只需要写 `shader.vert` / `shader.frag`，然后在根 `CMakeLists.txt` 里把它们加进 `add_shaders(...)`。
